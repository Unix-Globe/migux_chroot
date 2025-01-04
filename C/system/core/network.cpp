#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>

class NetworkManager {
private:
    bool network_enabled;
    bool ipv6_enabled;
    std::vector<std::string> interfaces;

    void detect_interfaces() {
        struct if_nameindex *if_ni, *i;

        if_ni = if_nameindex();
        if (if_ni == NULL) {
            std::cerr << "Error getting network interfaces" << std::endl;
            return;
        }

        for (i = if_ni; i->if_index != 0 && i->if_name != NULL; i++) {
            interfaces.push_back(i->if_name);
        }

        if_freenameindex(if_ni);
    }

    bool configure_interface(const std::string& interface) {
        struct ifreq ifr;
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "Error creating socket" << std::endl;
            return false;
        }

        strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ);
        
        // Get interface flags
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
            std::cerr << "Error getting interface flags for " << interface << std::endl;
            close(sock);
            return false;
        }

        // Set interface up
        ifr.ifr_flags |= IFF_UP;
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
            std::cerr << "Error setting interface " << interface << " up" << std::endl;
            close(sock);
            return false;
        }

        close(sock);
        return true;
    }

    void setup_ipv6() {
        #ifdef ENABLE_IPV6
        if (ENABLE_IPV6) {
            std::ofstream ipv6_conf("/proc/sys/net/ipv6/conf/all/disable_ipv6");
            ipv6_conf << "0" << std::endl;
        }
        #endif
    }

public:
    NetworkManager() {
        #ifdef ENABLE_NETWORK
        network_enabled = ENABLE_NETWORK;
        #else
        network_enabled = false;
        #endif

        #ifdef ENABLE_IPV6
        ipv6_enabled = ENABLE_IPV6;
        #else
        ipv6_enabled = false;
        #endif
    }

    bool start() {
        if (!network_enabled) {
            std::cout << "Network is disabled in configuration" << std::endl;
            return false;
        }

        std::cout << "Starting network..." << std::endl;

        // Detect network interfaces
        detect_interfaces();

        if (interfaces.empty()) {
            std::cerr << "No network interfaces found" << std::endl;
            return false;
        }

        // Configure IPv6 if enabled
        if (ipv6_enabled) {
            setup_ipv6();
        }

        // Configure each interface
        bool success = true;
        for (const auto& interface : interfaces) {
            if (!configure_interface(interface)) {
                success = false;
            }
        }

        #ifdef ENABLE_DHCP
        if (ENABLE_DHCP) {
            std::cout << "DHCP client will be started separately" << std::endl;
        }
        #endif

        return success;
    }

    bool stop() {
        if (!network_enabled) return true;

        std::cout << "Stopping network..." << std::endl;
        
        bool success = true;
        for (const auto& interface : interfaces) {
            struct ifreq ifr;
            int sock = socket(AF_INET, SOCK_DGRAM, 0);
            
            if (sock < 0) {
                std::cerr << "Error creating socket" << std::endl;
                success = false;
                continue;
            }

            strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ);
            
            if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
                std::cerr << "Error getting interface flags for " << interface << std::endl;
                close(sock);
                success = false;
                continue;
            }

            ifr.ifr_flags &= ~IFF_UP;
            if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
                std::cerr << "Error setting interface " << interface << " down" << std::endl;
                success = false;
            }

            close(sock);
        }

        return success;
    }

    void status() {
        if (!network_enabled) {
            std::cout << "Network is disabled" << std::endl;
            return;
        }

        std::cout << "Network status:\n";
        std::cout << "IPv6: " << (ipv6_enabled ? "enabled" : "disabled") << "\n\n";
        
        std::cout << "Interfaces:\n";
        for (const auto& interface : interfaces) {
            struct ifreq ifr;
            int sock = socket(AF_INET, SOCK_DGRAM, 0);
            
            if (sock < 0) {
                std::cerr << "Error creating socket" << std::endl;
                continue;
            }

            strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ);
            
            if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
                std::cerr << "Error getting interface flags for " << interface << std::endl;
                close(sock);
                continue;
            }

            std::cout << interface << ": " 
                     << ((ifr.ifr_flags & IFF_UP) ? "UP" : "DOWN")
                     << ((ifr.ifr_flags & IFF_RUNNING) ? " RUNNING" : "")
                     << std::endl;

            close(sock);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [start|stop|status]\n";
        return 1;
    }

    NetworkManager manager;
    std::string command = argv[1];

    if (command == "start") {
        return manager.start() ? 0 : 1;
    } else if (command == "stop") {
        return manager.stop() ? 0 : 1;
    } else if (command == "status") {
        manager.status();
        return 0;
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }
}
