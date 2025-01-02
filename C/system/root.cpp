#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>

class RootManager {
private:
    bool isRoot;
    uid_t originalUid;

public:
    RootManager() {
        originalUid = getuid();
        isRoot = (originalUid == 0);
    }

    bool checkRootAccess() {
        return isRoot;
    }

    bool elevatePrivileges() {
        if (setuid(0) != 0) {
            std::cerr << "Failed to elevate privileges" << std::endl;
            return false;
        }
        isRoot = true;
        return true;
    }

    bool dropPrivileges() {
        if (setuid(originalUid) != 0) {
            std::cerr << "Failed to drop privileges" << std::endl;
            return false;
        }
        isRoot = false;
        return true;
    }

    bool executeAsRoot(const std::string& command) {
        if (!isRoot) {
            std::cerr << "Root privileges required" << std::endl;
            return false;
        }
        return system(command.c_str()) == 0;
    }
};

int main() {
    RootManager rootMgr;

    if (!rootMgr.checkRootAccess()) {
        std::cout << "Warning: Program not running as root" << std::endl;
        return 1;
    }

    // Example usage
    if (rootMgr.executeAsRoot("whoami")) {
        std::cout << "Command executed successfully" << std::endl;
    }

    return 0;
}