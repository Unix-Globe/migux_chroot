#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sys/mount.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>

namespace fs = std::filesystem;

class AutoBoot {
private:
    std::string chroot_path;
    bool mount_success = false;

    bool mount_virtual_filesystems() {
        const std::string mounts[][2] = {
            {"proc", "/proc"},
            {"sysfs", "/sys"},
            {"devtmpfs", "/dev"},
            {"devpts", "/dev/pts"}
        };

        for (const auto& mount_point : mounts) {
            std::string target = chroot_path + mount_point[1];
            
            // Create mount point if it doesn't exist
            fs::create_directories(target);

            if (mount(mount_point[0].c_str(), target.c_str(), mount_point[0].c_str(), 0, nullptr) != 0) {
                std::cerr << "Failed to mount " << mount_point[1] << ": " << strerror(errno) << std::endl;
                return false;
            }
        }
        mount_success = true;
        return true;
    }

    void unmount_virtual_filesystems() {
        if (!mount_success) return;

        const std::string unmounts[] = {
            "/dev/pts",
            "/dev",
            "/sys",
            "/proc"
        };

        for (const auto& point : unmounts) {
            std::string target = chroot_path + point;
            umount(target.c_str());
        }
    }

    bool setup_motd() {
        std::string motd_path = chroot_path + "/etc/motd";
        std::ofstream motd(motd_path);
        if (!motd) {
            std::cerr << "Failed to create MOTD file" << std::endl;
            return false;
        }

        motd << "\033[1;32m" // Bright green
             << "Welcome to MIG/UX Chroot Environment!\n"
             << "\033[0m"     // Reset color
             << "===============================\n\n"
             << "System Information:\n"
             << "- Distribution: MIG/UX\n"
             << "- Version: 1.0 (initial)\n"
             << "- Architecture: " << (sizeof(void*) == 8 ? "x86_64" : "x86") << "\n\n"
             << "\033[1;34m" // Bright blue
             << "Quick Tips:\n"
             << "\033[0m"     // Reset color
             << "- Use 'ash' for shell access\n"
             << "- System configuration is in /etc/migux\n"
             << "- Type 'help' for available commands\n\n"
             << "\033[1;33m" // Bright yellow
             << "Note: You are in a chroot environment\n"
             << "\033[0m";    // Reset color

        return true;
    }

public:
    AutoBoot(const std::string& path) : chroot_path(path) {
        // Remove trailing slash if present
        if (chroot_path.back() == '/') {
            chroot_path.pop_back();
        }
    }

    ~AutoBoot() {
        unmount_virtual_filesystems();
    }

    bool start() {
        // Check if directory exists and is accessible
        if (!fs::exists(chroot_path) || !fs::is_directory(chroot_path)) {
            std::cerr << "Error: " << chroot_path << " is not a valid directory" << std::endl;
            return false;
        }

        // Check if running as root
        if (getuid() != 0) {
            std::cerr << "Error: This program must be run as root" << std::endl;
            return false;
        }

        // Mount virtual filesystems
        if (!mount_virtual_filesystems()) {
            std::cerr << "Failed to mount virtual filesystems" << std::endl;
            return false;
        }

        // Setup MOTD
        if (!setup_motd()) {
            std::cerr << "Warning: Failed to setup MOTD" << std::endl;
            // Continue anyway, not critical
        }

        // Create necessary directories
        fs::create_directories(chroot_path + "/etc");
        fs::create_directories(chroot_path + "/bin");
        fs::create_directories(chroot_path + "/lib");
        fs::create_directories(chroot_path + "/usr/bin");
        fs::create_directories(chroot_path + "/usr/lib");

        // Change root
        if (chdir(chroot_path.c_str()) != 0) {
            std::cerr << "Failed to change directory to " << chroot_path << std::endl;
            return false;
        }

        if (chroot(chroot_path.c_str()) != 0) {
            std::cerr << "Failed to chroot to " << chroot_path << std::endl;
            return false;
        }

        // Execute shell
        execl("/bin/ash", "ash", nullptr);
        std::cerr << "Failed to execute shell" << std::endl;
        return false;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <chroot-path>" << std::endl;
        return 1;
    }

    AutoBoot autoboot(argv[1]);
    return autoboot.start() ? 0 : 1;
}
