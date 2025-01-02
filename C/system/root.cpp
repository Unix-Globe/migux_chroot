#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/capability.h>
#include <linux/capability.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <memory>
#include <errno.h>
#include <fcntl.h>

namespace fs = std::filesystem;

class SecurityException : public std::runtime_error {
public:
    explicit SecurityException(const std::string& msg) : std::runtime_error(msg) {}
};

class RootManager {
private:
    bool isRoot;
    uid_t originalUid;
    gid_t originalGid;
    std::vector<gid_t> supplementaryGroups;
    std::string chrootPath;
    bool inChroot;

    void saveIdentity() {
        originalUid = getuid();
        originalGid = getgid();
        
        int ngroups = getgroups(0, nullptr);
        if (ngroups > 0) {
            supplementaryGroups.resize(ngroups);
            if (getgroups(ngroups, supplementaryGroups.data()) == -1) {
                throw SecurityException("Failed to get supplementary groups");
            }
        }
    }

    bool setupMountPoints(const std::string& newRoot) {
        std::vector<std::string> criticalDirs = {"/proc", "/sys", "/dev", "/dev/pts", "/run"};
        
        for (const auto& dir : criticalDirs) {
            std::string mountPoint = newRoot + dir;
            if (!fs::exists(mountPoint)) {
                fs::create_directories(mountPoint);
            }
            
            // Mount with appropriate flags
            if (dir == "/proc") {
                if (mount("proc", mountPoint.c_str(), "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr) != 0) {
                    return false;
                }
            } else if (dir == "/sys") {
                if (mount("sysfs", mountPoint.c_str(), "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV, nullptr) != 0) {
                    return false;
                }
            } else if (dir == "/dev") {
                if (mount("devtmpfs", mountPoint.c_str(), "devtmpfs", MS_NOSUID, nullptr) != 0) {
                    return false;
                }
            } else if (dir == "/dev/pts") {
                if (mount("devpts", mountPoint.c_str(), "devpts", MS_NOSUID | MS_NOEXEC, nullptr) != 0) {
                    return false;
                }
            }
        }
        return true;
    }

    bool dropCapabilities() {
        cap_t caps = cap_init();
        if (!caps) {
            return false;
        }
        
        if (cap_set_proc(caps) == -1) {
            cap_free(caps);
            return false;
        }
        
        cap_free(caps);
        return true;
    }

    bool setupSecurityBoundaries() {
        // Disable core dumps
        if (prctl(PR_SET_DUMPABLE, 0) == -1) {
            return false;
        }

        // Set no new privileges flag
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
            return false;
        }

        return true;
    }

public:
    RootManager() : isRoot(false), inChroot(false) {
        try {
            saveIdentity();
            isRoot = (originalUid == 0);
        } catch (const std::exception& e) {
            throw SecurityException("Failed to initialize RootManager: " + std::string(e.what()));
        }
    }

    bool checkRootAccess() const {
        return isRoot;
    }

    bool elevatePrivileges() {
        if (seteuid(0) != 0) {
            throw SecurityException("Failed to elevate privileges");
        }
        isRoot = true;
        return true;
    }

    bool dropPrivileges() {
        if (!isRoot) return true;

        // First switch to original GID
        if (setgid(originalGid) != 0) {
            throw SecurityException("Failed to drop group privileges");
        }

        // Set supplementary groups
        if (!supplementaryGroups.empty()) {
            if (setgroups(supplementaryGroups.size(), supplementaryGroups.data()) != 0) {
                throw SecurityException("Failed to set supplementary groups");
            }
        }

        // Finally switch to original UID
        if (setuid(originalUid) != 0) {
            throw SecurityException("Failed to drop user privileges");
        }

        isRoot = false;
        return true;
    }

    bool enterChroot(const std::string& newRoot) {
        if (!isRoot) {
            throw SecurityException("Root privileges required for chroot");
        }

        if (!fs::exists(newRoot)) {
            throw SecurityException("Chroot directory does not exist");
        }

        // Save the chroot path for later use
        chrootPath = newRoot;

        // Setup mount points
        if (!setupMountPoints(newRoot)) {
            throw SecurityException("Failed to setup mount points");
        }

        // Change to the new root directory
        if (chdir(newRoot.c_str()) != 0) {
            throw SecurityException("Failed to change to new root directory");
        }

        // Perform the chroot
        if (chroot(newRoot.c_str()) != 0) {
            throw SecurityException("Failed to chroot");
        }

        // Additional security measures
        if (!setupSecurityBoundaries()) {
            throw SecurityException("Failed to setup security boundaries");
        }

        inChroot = true;
        return true;
    }

    bool executeSecurely(const std::string& command, uid_t targetUid = 0, gid_t targetGid = 0) {
        if (!isRoot) {
            throw SecurityException("Root privileges required for secure execution");
        }

        pid_t pid = fork();
        if (pid == -1) {
            throw SecurityException("Fork failed");
        }

        if (pid == 0) {  // Child process
            // Drop capabilities
            if (!dropCapabilities()) {
                _exit(EXIT_FAILURE);
            }

            // Change to target user if specified
            if (targetUid != 0) {
                if (setuid(targetUid) != 0) {
                    _exit(EXIT_FAILURE);
                }
            }

            // Execute the command
            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            _exit(EXIT_FAILURE);
        }

        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    ~RootManager() {
        try {
            if (isRoot) {
                dropPrivileges();
            }
        } catch (...) {
            // Log error but don't throw from destructor
        }
    }
};

// Example usage
int main(int argc, char* argv[]) {
    try {
        RootManager rootMgr;

        if (!rootMgr.checkRootAccess()) {
            std::cerr << "Error: This program must be run as root\n";
            return 1;
        }

        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <chroot-path>\n";
            return 1;
        }

        // Enter chroot environment
        rootMgr.enterChroot(argv[1]);

        // Execute some commands in chroot
        rootMgr.executeSecurely("id");
        rootMgr.executeSecurely("mount");

        // Drop privileges for safety
        rootMgr.dropPrivileges();

    } catch (const SecurityException& e) {
        std::cerr << "Security error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}