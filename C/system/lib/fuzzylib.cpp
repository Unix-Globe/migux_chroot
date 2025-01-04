#include "fuzzylib.hpp"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace fs = std::filesystem;

namespace FuzzyBox {

void FuzzyShell::registerCommand(const std::string& name, std::unique_ptr<Command> cmd) {
    commands[name] = std::move(cmd);
}

std::vector<std::string> FuzzyShell::parseCommand(const std::string& cmdline) {
    std::vector<std::string> args;
    std::istringstream iss(cmdline);
    std::string arg;
    
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    return args;
}

int FuzzyShell::executeCommand(const std::string& name, const std::vector<std::string>& args) {
    auto it = commands.find(name);
    if (it == commands.end()) {
        std::cerr << "Command not found: " << name << std::endl;
        return 1;
    }
    return it->second->execute(args);
}

void FuzzyShell::displayHelp(const std::string& command) {
    if (command.empty()) {
        std::cout << "Available commands:\n";
        for (const auto& cmd : commands) {
            std::cout << "  " << cmd.first << " - " << cmd.second->help() << "\n";
        }
    } else {
        auto it = commands.find(command);
        if (it != commands.end()) {
            std::cout << command << " - " << it->second->help() << "\n";
        } else {
            std::cerr << "No help available for: " << command << "\n";
        }
    }
}

void FuzzyShell::run() {
    std::string line;
    
    while (running) {
        std::cout << prompt;
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        auto args = parseCommand(line);
        if (args.empty()) continue;
        
        std::string cmd = args[0];
        args.erase(args.begin());
        
        if (cmd == "exit" || cmd == "quit") {
            break;
        } else if (cmd == "help") {
            displayHelp(args.empty() ? "" : args[0]);
        } else {
            executeCommand(cmd, args);
        }
    }
}

namespace Commands {

int CatCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: cat <file>" << std::endl;
        return 1;
    }
    
    std::ifstream file(args[0]);
    if (!file) {
        std::cerr << "Cannot open file: " << args[0] << std::endl;
        return 1;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }
    
    return 0;
}

std::string CatCommand::help() const {
    return "Display file contents";
}

int LsCommand::execute(const std::vector<std::string>& args) {
    std::string path = args.empty() ? "." : args[0];
    
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            std::cout << entry.path().filename().string() << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

std::string LsCommand::help() const {
    return "List directory contents";
}

int CpCommand::execute(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cerr << "Usage: cp <source> <destination>" << std::endl;
        return 1;
    }
    
    try {
        fs::copy(args[0], args[1], fs::copy_options::overwrite_existing);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

std::string CpCommand::help() const {
    return "Copy files";
}

int MvCommand::execute(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cerr << "Usage: mv <source> <destination>" << std::endl;
        return 1;
    }
    
    try {
        fs::rename(args[0], args[1]);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

std::string MvCommand::help() const {
    return "Move/rename files";
}

int RmCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: rm <file>" << std::endl;
        return 1;
    }
    
    try {
        fs::remove(args[0]);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

std::string RmCommand::help() const {
    return "Remove files";
}

int MkdirCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: mkdir <directory>" << std::endl;
        return 1;
    }
    
    try {
        fs::create_directories(args[0]);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

std::string MkdirCommand::help() const {
    return "Create directories";
}

int PwdCommand::execute(const std::vector<std::string>& args) {
    (void)args; // Unused
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << cwd << std::endl;
    } else {
        std::cerr << "Error getting current directory" << std::endl;
        return 1;
    }
    return 0;
}

std::string PwdCommand::help() const {
    return "Print working directory";
}

int CdCommand::execute(const std::vector<std::string>& args) {
    std::string path = args.empty() ? getenv("HOME") : args[0];
    
    if (chdir(path.c_str()) != 0) {
        std::cerr << "Error changing directory to " << path << std::endl;
        return 1;
    }
    
    return 0;
}

std::string CdCommand::help() const {
    return "Change directory";
}

int EchoCommand::execute(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        std::cout << args[i];
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    return 0;
}

std::string EchoCommand::help() const {
    return "Display a line of text";
}

} // namespace Commands
} // namespace FuzzyBox
