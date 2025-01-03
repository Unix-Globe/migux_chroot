#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

namespace fs = std::filesystem;

class ImageBuilder {
private:
    std::map<std::string, std::string> config;
    fs::path work_dir;
    fs::path output_dir;

    void load_config(const std::string& config_file) {
        std::ifstream conf(config_file);
        std::string line;
        
        while (std::getline(conf, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                config[key] = value;
            }
        }
    }

    void prepare_work_dir() {
        fs::remove_all(work_dir);
        fs::create_directories(work_dir);
        fs::create_directories(work_dir / "boot");
        fs::create_directories(work_dir / "root");
    }

    void copy_system_files() {
        // Copy bootloader files
        if (config["BOOTLOADER"] == "grub2") {
            system(("cp -r /usr/lib/grub/* " + (work_dir / "boot/grub").string()).c_str());
        } else if (config["BOOTLOADER"] == "systemd-boot") {
            system(("cp -r /usr/lib/systemd/boot/* " + (work_dir / "boot").string()).c_str());
        }

        // Copy root filesystem
        system(("cp -r ./bin/* " + (work_dir / "root").string()).c_str());
        
        // Copy documentation if enabled
        if (config["INCLUDE_DOCS"] == "1") {
            fs::create_directories(work_dir / "root/usr/share/doc");
            system(("cp -r ./doc/* " + (work_dir / "root/usr/share/doc").string()).c_str());
        }

        // Copy development tools if enabled
        if (config["INCLUDE_DEV"] == "1") {
            fs::create_directories(work_dir / "root/usr/include");
            fs::create_directories(work_dir / "root/usr/lib");
            system(("cp -r ./dev/* " + (work_dir / "root/usr").string()).c_str());
        }

        // Copy custom boot logo if specified
        if (!config["CUSTOM_LOGO"].empty()) {
            fs::copy_file(config["CUSTOM_LOGO"], work_dir / "boot/logo.png");
        }
    }

    void create_metadata() {
        std::ofstream meta(work_dir / "root/.migux-release");
        meta << "DISTRO_NAME=" << config["DISTRO_NAME"] << "\n"
             << "DISTRO_VERSION=" << config["DISTRO_VERSION"] << "\n"
             << "DISTRO_CODENAME=" << config["DISTRO_CODENAME"] << "\n";
    }

    void create_iso() {
        std::string iso_file = (output_dir / (config["DISTRO_NAME"] + "-" + 
                                            config["DISTRO_VERSION"] + ".iso")).string();
        
        std::string cmd = "mkisofs -o " + iso_file + 
                         " -b boot/grub/i386-pc/eltorito.img" +
                         " -no-emul-boot -boot-load-size 4 -boot-info-table" +
                         " -R -J -v -T " + work_dir.string();
        
        if (!config["ISO_LABEL"].empty()) {
            cmd += " -V " + config["ISO_LABEL"];
        }

        system(cmd.c_str());

        if (config["ENABLE_SECURE_BOOT"] == "1") {
            // Sign the ISO for secure boot
            system(("sbsign --key secureboot.key --cert secureboot.crt " + iso_file).c_str());
        }
    }

    void create_tarball() {
        std::string compression_opt;
        if (config["COMPRESSION"] == "gz") compression_opt = "z";
        else if (config["COMPRESSION"] == "bz2") compression_opt = "j";
        else if (config["COMPRESSION"] == "xz") compression_opt = "J";
        else if (config["COMPRESSION"] == "zstd") compression_opt = "--zstd";
        
        std::string tar_file = (output_dir / (config["DISTRO_NAME"] + "-" + 
                                            config["DISTRO_VERSION"] + ".tar." + 
                                            config["COMPRESSION"])).string();
        
        std::string cmd = "tar -c" + compression_opt + "f " + tar_file + " -C " + 
                         work_dir.string() + " .";
        
        system(cmd.c_str());

        if (config["SIGN_PACKAGES"] == "1") {
            // Sign the tarball
            system(("gpg --sign " + tar_file).c_str());
        }
    }

public:
    ImageBuilder(const std::string& config_file) 
        : work_dir(fs::temp_directory_path() / "migux_build")
        , output_dir(fs::current_path() / "images") {
        load_config(config_file);
        fs::create_directories(output_dir);
    }

    bool build() {
        try {
            std::cout << "Preparing work directory...\n";
            prepare_work_dir();

            std::cout << "Copying system files...\n";
            copy_system_files();

            std::cout << "Creating metadata...\n";
            create_metadata();

            if (config["MEDIA_TYPE"] == "iso" || config["MEDIA_TYPE"] == "both") {
                std::cout << "Creating ISO image...\n";
                create_iso();
            }

            if (config["MEDIA_TYPE"] == "tarball" || config["MEDIA_TYPE"] == "both") {
                std::cout << "Creating tarball...\n";
                create_tarball();
            }

            std::cout << "Cleaning up...\n";
            fs::remove_all(work_dir);

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config-file>\n";
        return 1;
    }

    ImageBuilder builder(argv[1]);
    return builder.build() ? 0 : 1;
}
