#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <ncurses.h>
#include <menu.h>
#include <form.h>
#include <cstring>
#include <unistd.h>

namespace fs = std::filesystem;

class ChrootInstaller {
private:
    struct InstallConfig {
        std::string target_dir;
        bool mount_proc;
        bool mount_sys;
        bool mount_dev;
        bool mount_pts;
        bool enable_network;
        bool copy_resolv_conf;
        bool install_dev_tools;
    };

    InstallConfig config;
    WINDOW *main_win;

    void init_ncurses() {
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
        init_pair(2, COLOR_BLACK, COLOR_WHITE);
        refresh();

        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        main_win = newwin(max_y-2, max_x-2, 1, 1);
        box(main_win, 0, 0);
        wbkgd(main_win, COLOR_PAIR(1));
        wrefresh(main_win);
    }

    bool create_directory_structure() {
        try {
            fs::create_directories(config.target_dir);
            fs::create_directories(config.target_dir + "/proc");
            fs::create_directories(config.target_dir + "/sys");
            fs::create_directories(config.target_dir + "/dev");
            fs::create_directories(config.target_dir + "/dev/pts");
            fs::create_directories(config.target_dir + "/etc");
            fs::create_directories(config.target_dir + "/usr");
            fs::create_directories(config.target_dir + "/var");
            fs::create_directories(config.target_dir + "/tmp");
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }

    bool mount_virtual_filesystems() {
        if (config.mount_proc) {
            if (system(("mount -t proc proc " + config.target_dir + "/proc").c_str()) != 0)
                return false;
        }
        
        if (config.mount_sys) {
            if (system(("mount -t sysfs sys " + config.target_dir + "/sys").c_str()) != 0)
                return false;
        }
        
        if (config.mount_dev) {
            if (system(("mount -o bind /dev " + config.target_dir + "/dev").c_str()) != 0)
                return false;
        }

        if (config.mount_pts) {
            if (system(("mount -o bind /dev/pts " + config.target_dir + "/dev/pts").c_str()) != 0)
                return false;
        }

        return true;
    }

    bool setup_networking() {
        if (config.copy_resolv_conf) {
            if (system(("cp /etc/resolv.conf " + config.target_dir + "/etc/resolv.conf").c_str()) != 0)
                return false;
        }
        return true;
    }

    bool install_base_system() {
        // Extract base system files
        std::string cmd = "tar xf /usr/share/mig_ux/base.tar.xz -C " + config.target_dir;
        if (system(cmd.c_str()) != 0)
            return false;

        // Install development tools if requested
        if (config.install_dev_tools) {
            cmd = "tar xf /usr/share/mig_ux/dev-tools.tar.xz -C " + config.target_dir;
            if (system(cmd.c_str()) != 0)
                return false;
        }

        return true;
    }

    void cleanup() {
        // Unmount in reverse order
        if (config.mount_pts)
            system(("umount " + config.target_dir + "/dev/pts").c_str());
        if (config.mount_dev)
            system(("umount " + config.target_dir + "/dev").c_str());
        if (config.mount_sys)
            system(("umount " + config.target_dir + "/sys").c_str());
        if (config.mount_proc)
            system(("umount " + config.target_dir + "/proc").c_str());
        
        endwin();
    }

public:
    ChrootInstaller() {
        init_ncurses();
        // Set defaults
        config.mount_proc = true;
        config.mount_sys = true;
        config.mount_dev = true;
        config.mount_pts = true;
        config.enable_network = true;
        config.copy_resolv_conf = true;
        config.install_dev_tools = false;
    }

    ~ChrootInstaller() {
        cleanup();
    }

    bool run() {
        // Welcome screen
        mvwprintw(main_win, 2, 2, "Welcome to MIG/UX Chroot Environment Installer");
        mvwprintw(main_win, 4, 2, "This will set up a chroot environment on your system.");
        mvwprintw(main_win, 5, 2, "Enter target directory: ");
        char target_dir[256];
        echo();
        wgetstr(main_win, target_dir);
        noecho();
        config.target_dir = target_dir;

        // Installation steps
        wclear(main_win);
        box(main_win, 0, 0);
        
        int current_line = 2;
        mvwprintw(main_win, current_line++, 2, "Creating directory structure...");
        wrefresh(main_win);
        if (!create_directory_structure()) {
            mvwprintw(main_win, current_line, 2, "Failed to create directories!");
            wrefresh(main_win);
            getch();
            return false;
        }

        mvwprintw(main_win, current_line++, 2, "Mounting virtual filesystems...");
        wrefresh(main_win);
        if (!mount_virtual_filesystems()) {
            mvwprintw(main_win, current_line, 2, "Failed to mount filesystems!");
            wrefresh(main_win);
            getch();
            return false;
        }

        if (config.enable_network) {
            mvwprintw(main_win, current_line++, 2, "Setting up networking...");
            wrefresh(main_win);
            if (!setup_networking()) {
                mvwprintw(main_win, current_line, 2, "Failed to setup networking!");
                wrefresh(main_win);
                getch();
                return false;
            }
        }

        mvwprintw(main_win, current_line++, 2, "Installing base system...");
        wrefresh(main_win);
        if (!install_base_system()) {
            mvwprintw(main_win, current_line, 2, "Failed to install base system!");
            wrefresh(main_win);
            getch();
            return false;
        }

        // Show completion message
        mvwprintw(main_win, current_line++, 2, "Installation completed successfully!");
        mvwprintw(main_win, current_line++, 2, "To enter the chroot environment:");
        mvwprintw(main_win, current_line++, 2, "  chroot %s /bin/sh", config.target_dir.c_str());
        mvwprintw(main_win, current_line++, 2, "Press any key to exit...");
        wrefresh(main_win);
        getch();

        return true;
    }
};

int main() {
    if (geteuid() != 0) {
        std::cerr << "This program must be run as root!\n";
        return 1;
    }

    ChrootInstaller installer;
    return installer.run() ? 0 : 1;
}
