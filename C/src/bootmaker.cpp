#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <system_error>
#include <cstdlib>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include "../system/root.cpp"

namespace fs = std::filesystem;

class BootMaker {
private:
    std::string rootfs_path;
    std::vector<std::string> required_dirs = {
        "bin", "sbin", "lib", "lib64", "usr", "etc", 
        "var", "tmp", "proc", "sys", "dev", "run", 
        "home", "root", "opt"
    };

    std::vector<std::string> required_files = {
        "/etc/passwd",
        "/etc/group",
        "/etc/hosts",
        "/etc/resolv.conf",
        "/etc/fstab"
    };

    bool createDirectoryStructure() {
        try {
            for (const auto& dir : required_dirs) {
                fs::path dirPath = fs::path(rootfs_path) / dir;
                fs::create_directories(dirPath);
                chmod(dirPath.c_str(), 0755);
            }
            
            // Set special permissions for sensitive directories
            chmod((fs::path(rootfs_path) / "tmp").c_str(), 01777);
            chmod((fs::path(rootfs_path) / "root").c_str(), 0700);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error creating directory structure: " << e.what() << std::endl;
            return false;
        }
    }

    bool copySystemFiles() {
        try {
            for (const auto& file : required_files) {
                fs::path destPath = fs::path(rootfs_path) / file.substr(1);
                fs::create_directories(destPath.parent_path());
                fs::copy_file(file, destPath, fs::copy_options::overwrite_existing);
            }
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error copying system files: " << e.what() << std::endl;
            return false;
        }
    }

    bool setupBasicSystem() {
        std::vector<std::string> setup_commands = {
            "cp /bin/bash " + rootfs_path + "/bin/",
            "cp /bin/ls " + rootfs_path + "/bin/",
            "cp /bin/cat " + rootfs_path + "/bin/",
            "cp /bin/echo " + rootfs_path + "/bin/",
            "cp /bin/mkdir " + rootfs_path + "/bin/",
            "cp /bin/chmod " + rootfs_path + "/bin/",
            "cp /bin/chown " + rootfs_path + "/bin/"
        };

        for (const auto& cmd : setup_commands) {
            if (system(cmd.c_str()) != 0) {
                std::cerr << "Failed to execute: " << cmd << std::endl;
                return false;
            }
        }
        return true;
    }

    bool copySharedLibraries() {
        std::string ldd_cmd = "ldd /bin/bash | grep -o '/lib[^ ]*' | sort | uniq";
        FILE* pipe = popen(ldd_cmd.c_str(), "r");
        if (!pipe) return false;

        char buffer[256];
        std::vector<std::string> libraries;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string lib(buffer);
            lib = lib.substr(0, lib.find('\n'));
            libraries.push_back(lib);
        }
        pclose(pipe);

        for (const auto& lib : libraries) {
            fs::path destPath = fs::path(rootfs_path) / lib.substr(1);
            try {
                fs::create_directories(destPath.parent_path());
                fs::copy_file(lib, destPath, fs::copy_options::overwrite_existing);
            } catch (const std::exception& e) {
                std::cerr << "Error copying library " << lib << ": " << e.what() << std::endl;
                return false;
            }
        }
        return true;
    }

    bool setupNetwork() {
        try {
            // Create network configuration
            fs::path etc_path = fs::path(rootfs_path) / "etc";
            
            // Setup hosts file
            std::ofstream hosts(etc_path / "hosts");
            hosts << "127.0.0.1 localhost\n";
            hosts << "::1 localhost ip6-localhost ip6-loopback\n";
            hosts.close();

            // Setup resolv.conf
            std::ofstream resolv(etc_path / "resolv.conf");
            resolv << "nameserver 8.8.8.8\n";
            resolv << "nameserver 8.8.4.4\n";
            resolv.close();

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error setting up network: " << e.what() << std::endl;
            return false;
        }
    }

public:
    BootMaker(const std::string& path) : rootfs_path(path) {}

    bool initialize() {
        std::cout << "Initializing chroot environment at " << rootfs_path << std::endl;
        
        if (!createDirectoryStructure()) {
            std::cerr << "Failed to create directory structure" << std::endl;
            return false;
        }

        if (!copySystemFiles()) {
            std::cerr << "Failed to copy system files" << std::endl;
            return false;
        }

        if (!setupBasicSystem()) {
            std::cerr << "Failed to setup basic system" << std::endl;
            return false;
        }

        if (!copySharedLibraries()) {
            std::cerr << "Failed to copy shared libraries" << std::endl;
            return false;
        }

        if (!setupNetwork()) {
            std::cerr << "Failed to setup network configuration" << std::endl;
            return false;
        }

        std::cout << "Chroot environment initialized successfully" << std::endl;
        return true;
    }

    bool start() {
        try {
            RootManager rootMgr;
            if (!rootMgr.checkRootAccess()) {
                throw std::runtime_error("Root privileges required");
            }

            // Enter chroot environment
            if (!rootMgr.enterChroot(rootfs_path)) {
                throw std::runtime_error("Failed to enter chroot environment");
            }

            // Execute the shell
            rootMgr.executeSecurely("/bin/bash");

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error starting chroot environment: " << e.what() << std::endl;
            return false;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <rootfs-path>\n";
        return 1;
    }

    if (getuid() != 0) {
        std::cerr << "This program must be run as root\n";
        return 1;
    }

    BootMaker bootmaker(argv[1]);
    
    if (!bootmaker.initialize()) {
        std::cerr << "Failed to initialize chroot environment\n";
        return 1;
    }

    if (!bootmaker.start()) {
        std::cerr << "Failed to start chroot environment\n";
        return 1;
    }

    return 0;
}
