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

struct OEMOption {
    std::string name;
    std::string description;
    std::string type;
    std::string value;
    std::vector<std::string> choices;
    std::string category;
};

class OEMConfig {
private:
    std::string config_file;
    WINDOW *menu_win;
    MENU *menu;
    std::vector<ITEM*> items;
    std::vector<OEMOption> options;
    std::vector<std::string> item_names;
    std::vector<std::string> item_descriptions;

    void init_curses() {
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_BLUE, COLOR_BLACK);
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
            // Basic Information
            {"DISTRO_NAME", "Distribution Name", "string", "Migux", {}, "Basic"},
            {"DISTRO_VERSION", "Distribution Version", "string", "1.0", {}, "Basic"},
            {"DISTRO_CODENAME", "Distribution Codename", "string", "initial", {}, "Basic"},
            
            // Media Options
            {"MEDIA_TYPE", "Installation Media Type", "choice", "iso", 
             {"iso", "tarball", "both"}, "Media"},
            {"ISO_LABEL", "ISO Label", "string", "MIGUX_INSTALL", {}, "Media"},
            {"COMPRESSION", "Tarball Compression", "choice", "xz",
             {"gz", "bz2", "xz", "zstd"}, "Media"},
            
            // Installation Options
            {"BOOTLOADER", "Bootloader Type", "choice", "grub2",
             {"grub2", "systemd-boot", "none"}, "Installation"},
            {"PARTITION_LAYOUT", "Default Partition Layout", "choice", "standard",
             {"standard", "lvm", "btrfs"}, "Installation"},
            {"DEFAULT_FS", "Default Root Filesystem", "choice", "ext4",
             {"ext4", "btrfs", "xfs"}, "Installation"},
            
            // Customization
            {"INCLUDE_DOCS", "Include Documentation", "bool", "1", {}, "Customization"},
            {"INCLUDE_DEV", "Include Development Tools", "bool", "0", {}, "Customization"},
            {"CUSTOM_LOGO", "Custom Boot Logo Path", "string", "", {}, "Customization"},
            
            // Security
            {"DEFAULT_ROOT_PASS", "Default Root Password", "string", "migux", {}, "Security"},
            {"ENABLE_SECURE_BOOT", "Enable Secure Boot Support", "bool", "0", {}, "Security"},
            {"SIGN_PACKAGES", "Sign Packages", "bool", "1", {}, "Security"}
        };
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
        mvprintw(1, (col - 30) / 2, "Migux OEM Configuration");
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
            
            std::string status = opt.type == "bool" ? (opt.value == "1" ? "[*]" : "[ ]") : "";
            item_names.push_back("  " + status + opt.name + ": " + opt.value);
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

    void edit_option(OEMOption& opt) {
        WINDOW* edit_win = newwin(6, 50, 10, 15);
        box(edit_win, 0, 0);
        wattron(edit_win, A_BOLD);
        mvwprintw(edit_win, 1, 2, "%s", opt.name.c_str());
        wattroff(edit_win, A_BOLD);
        mvwprintw(edit_win, 2, 2, "%s", opt.description.c_str());

        if (opt.type == "choice") {
            size_t choice = 0;
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
    OEMConfig(const std::string& config_path) 
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

    ~OEMConfig() {
        cleanup();
    }
};

int main() {
    fs::path config_dir = fs::current_path() / "config";
    fs::create_directories(config_dir);
    
    OEMConfig config((config_dir / "oem.conf").string());
    config.run();
    return 0;
}
