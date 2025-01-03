#include <ncurses.h>
#include <menu.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <memory>
#include <cstring>
#include <map>

namespace fs = std::filesystem;

struct ConfigOption {
    std::string name;
    std::string description;
    std::string type;
    std::string value;
    std::vector<std::string> choices;
    std::string category;
    bool depends_on;
    std::string dependency;
};

class MenuConfig {
private:
    std::string config_file;
    WINDOW *menu_win;
    MENU *menu;
    std::vector<ITEM*> items;
    std::vector<ConfigOption> options;
    std::vector<std::string> item_names;
    std::vector<std::string> item_descriptions;
    std::string current_category;

    void init_curses() {
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_BLUE, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    }

    void cleanup_menu() {
        if (menu) {
            unpost_menu(menu);
            free_menu(menu);
            menu = nullptr;
        }
        for (auto item : items) {
            free_item(item);
        }
        items.clear();
    }

    void cleanup() {
        cleanup_menu();
        endwin();
    }

    void load_default_config() {
        options = {
            // System Size
            {"SYSTEM_SIZE", "System Size", "choice", "medium", 
             {"minimal", "small", "medium", "large"}, "General", false, ""},

            // Core System
            {"INIT_SYSTEM", "Init System Type", "choice", "simple", 
             {"simple", "systemd-like", "custom"}, "Core System", false, ""},
            {"ENABLE_MODULES", "Enable Loadable Kernel Modules", "bool", "1", 
             {}, "Core System", false, ""},
            {"MAX_USERS", "Maximum Number of Users", "int", "10", 
             {}, "Core System", false, ""},

            // Filesystem
            {"ROOT_FS_TYPE", "Root Filesystem Type", "choice", "ext4", 
             {"ext4", "btrfs", "xfs"}, "Filesystem", false, ""},
            {"ENABLE_TMPFS", "Enable tmpfs", "bool", "1", 
             {}, "Filesystem", false, ""},
            {"MOUNT_PROC", "Mount /proc filesystem", "bool", "1", 
             {}, "Filesystem", false, ""},
            {"MOUNT_SYS", "Mount /sys filesystem", "bool", "1", 
             {}, "Filesystem", false, ""},
            {"MOUNT_DEV", "Mount /dev filesystem", "bool", "1", 
             {}, "Filesystem", false, ""},

            // Networking
            {"ENABLE_NETWORK", "Enable Networking", "bool", "1", 
             {}, "Networking", false, ""},
            {"ENABLE_IPV6", "Enable IPv6 Support", "bool", "1", 
             {}, "Networking", true, "ENABLE_NETWORK"},
            {"ENABLE_DHCP", "Enable DHCP Client", "bool", "1", 
             {}, "Networking", true, "ENABLE_NETWORK"},
            {"ENABLE_DNS", "Enable DNS Resolution", "bool", "1", 
             {}, "Networking", true, "ENABLE_NETWORK"},

            // Security
            {"ENABLE_PAM", "Enable PAM Authentication", "bool", "1", 
             {}, "Security", false, ""},
            {"ENABLE_SELINUX", "Enable SELinux Support", "bool", "0", 
             {}, "Security", false, ""},
            {"ENABLE_CAPABILITIES", "Enable POSIX Capabilities", "bool", "1", 
             {}, "Security", false, ""},

            // System Tools
            {"SYSTEM_PACKAGES", "System Packages", "string", 
             "bash,coreutils,net-tools", {}, "System Tools", false, ""},
            {"ENABLE_PACKAGE_MANAGER", "Enable Package Manager", "bool", "0", 
             {}, "System Tools", false, ""},

            // Device Support
            {"ENABLE_DEVICES", "Device Support Level", "choice", "full",
             {"minimal", "basic", "full"}, "Devices", false, ""},
            {"ENABLE_UDEV", "Enable udev Device Manager", "bool", "1", 
             {}, "Devices", true, "ENABLE_DEVICES"},
            {"ENABLE_INPUT", "Enable Input Devices", "bool", "1", 
             {}, "Devices", true, "ENABLE_DEVICES"},
            {"ENABLE_SOUND", "Enable Sound Support", "bool", "1", 
             {}, "Devices", true, "ENABLE_DEVICES"},

            // Development
            {"ENABLE_DEVELOPMENT", "Enable Development Tools", "bool", "0", 
             {}, "Development", false, ""},
            {"ENABLE_GDB", "Enable GDB Debugger", "bool", "0", 
             {}, "Development", true, "ENABLE_DEVELOPMENT"},
            {"ENABLE_STRACE", "Enable strace", "bool", "0", 
             {}, "Development", true, "ENABLE_DEVELOPMENT"}
        };
    }

    bool check_dependency(const ConfigOption& opt) {
        if (!opt.depends_on) return true;
        for (const auto& dep : options) {
            if (dep.name == opt.dependency) {
                return dep.value == "1";
            }
        }
        return false;
    }

    void save_config() {
        std::ofstream config(config_file);
        std::string current_category;
        
        for (const auto& opt : options) {
            if (opt.category != current_category) {
                config << "\n# " << opt.category << "\n";
                current_category = opt.category;
            }
            config << opt.name << "=" << opt.value << "\n";
        }
    }

    void load_config() {
        if (!fs::exists(config_file)) {
            load_default_config();
            return;
        }

        std::ifstream config(config_file);
        std::string line;
        while (std::getline(config, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string name = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                for (auto& opt : options) {
                    if (opt.name == name) {
                        opt.value = value;
                        break;
                    }
                }
            }
        }
    }

    void draw_title() {
        int col;
        getmaxyx(stdscr, std::ignore, col);
        attron(A_BOLD | COLOR_PAIR(1));
        mvprintw(1, (col - 30) / 2, "Migux Unix System Configuration");
        attroff(A_BOLD | COLOR_PAIR(1));
        mvhline(2, 1, ACS_HLINE, col - 2);
    }

    void draw_help() {
        int row, col;
        getmaxyx(stdscr, row, col);
        mvprintw(row-2, 2, "Arrow keys: Navigate | Enter: Edit | Q: Save and Exit");
    }

    void recreate_menu() {
        cleanup_menu();
        
        item_names.clear();
        item_descriptions.clear();
        
        std::string current_category;
        for (const auto& opt : options) {
            if (opt.category != current_category) {
                item_names.push_back("=== " + opt.category + " ===");
                item_descriptions.push_back("");
                current_category = opt.category;
            }
            
            std::string prefix = opt.depends_on && !check_dependency(opt) ? "  [D] " : "  ";
            std::string status = opt.type == "bool" ? (opt.value == "1" ? "[*]" : "[ ]") : "";
            item_names.push_back(prefix + status + opt.name + ": " + opt.value);
            item_descriptions.push_back(opt.description);
        }

        item_names.push_back("Save and Exit");
        item_descriptions.push_back("Save configuration and exit");

        for (size_t i = 0; i < item_names.size(); ++i) {
            items.push_back(new_item(item_names[i].c_str(), item_descriptions[i].c_str()));
        }
        items.push_back(nullptr);

        menu = new_menu(items.data());
        set_menu_win(menu, menu_win);
        set_menu_sub(menu, derwin(menu_win, 15, 68, 3, 1));
        set_menu_mark(menu, " * ");
        post_menu(menu);
        
        box(menu_win, 0, 0);
        refresh();
        wrefresh(menu_win);
    }

    bool is_category_header(const std::string& item_name) {
        return item_name.substr(0, 4) == "=== ";
    }

    void edit_option(ConfigOption& opt) {
        if (opt.depends_on && !check_dependency(opt)) {
            return;
        }

        WINDOW* edit_win = newwin(6, 50, 10, 15);
        box(edit_win, 0, 0);
        wattron(edit_win, A_BOLD);
        mvwprintw(edit_win, 1, 2, "%s", opt.name.c_str());
        wattroff(edit_win, A_BOLD);
        mvwprintw(edit_win, 2, 2, "%s", opt.description.c_str());

        if (opt.type == "choice") {
            long unsigned int choice = 0;
            for (size_t i = 0; i < opt.choices.size(); i++) {
                if (opt.choices[i] == opt.value) {
                    choice = i;
                    break;
                }
            }

            while (true) {
                for (size_t i = 0; i < opt.choices.size(); i++) {
                    mvwprintw(edit_win, 3, 2 + i * 12, "%-10s", opt.choices[i].c_str());
                    if (i == choice) {
                        mvwchgat(edit_win, 3, 2 + i * 12, 10, A_REVERSE, 0, NULL);
                    }
                }
                wrefresh(edit_win);

                int ch = getch();
                if (ch == KEY_LEFT && choice > 0) choice--;
                else if (ch == KEY_RIGHT && choice < opt.choices.size() - 1) choice++;
                else if (ch == 10) { // Enter
                    opt.value = opt.choices[choice];
                    break;
                }
                else if (ch == 27) break; // Escape
            }
        } else if (opt.type == "bool") {
            opt.value = (opt.value == "1") ? "0" : "1";
        } else {
            echo();
            char value[256];
            mvwprintw(edit_win, 3, 2, "New value: ");
            wgetstr(edit_win, value);
            opt.value = value;
            noecho();
        }

        delwin(edit_win);
        touchwin(stdscr);
        refresh();
    }

public:
    MenuConfig(const std::string& config_path) 
        : config_file(config_path)
        , menu_win(nullptr)
        , menu(nullptr) {
        load_default_config();
        load_config();
    }

    void run() {
        init_curses();

        menu_win = newwin(20, 70, 3, 5);
        keypad(menu_win, TRUE);

        recreate_menu();
        draw_title();
        draw_help();

        while (true) {
            int c = wgetch(menu_win);
            switch (c) {
                case KEY_DOWN:
                    menu_driver(menu, REQ_DOWN_ITEM);
                    break;
                case KEY_UP:
                    menu_driver(menu, REQ_UP_ITEM);
                    break;
                case 10: { // Enter
                    ITEM* cur = current_item(menu);
                    std::string item_name = item_name(cur);
                    
                    if (item_name == "Save and Exit") {
                        save_config();
                        cleanup();
                        return;
                    }
                    
                    if (!is_category_header(item_name)) {
                        int index = item_index(cur);
                        size_t opt_index = 0;
                        for (size_t i = 0; i < options.size(); ++i) {
                            if (!is_category_header(item_names[i])) {
                                if (opt_index == index) {
                                    edit_option(options[i]);
                                    recreate_menu();
                                    break;
                                }
                                opt_index++;
                            }
                        }
                    }
                    break;
                }
                case 'q':
                case 'Q':
                    save_config();
                    cleanup();
                    return;
            }
        }
    }

    ~MenuConfig() {
        cleanup();
    }
};

int main() {
    fs::path config_dir = fs::current_path() / "config";
    fs::create_directories(config_dir);
    
    MenuConfig config((config_dir / "migux.conf").string());
    config.run();
    return 0;
}
