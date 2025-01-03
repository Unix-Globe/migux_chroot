#ifndef FUZZYLIB_H
#define FUZZYLIB_H

#include <string>
#include <vector>
#include <functional>
#include <map>

namespace FuzzyBox {

class Command {
public:
    virtual int execute(const std::vector<std::string>& args) = 0;
    virtual ~Command() = default;
};

class FuzzyLib {
private:
    std::map<std::string, std::unique_ptr<Command>> commands;

public:
    FuzzyLib();
    
    // Register a new command
    void registerCommand(const std::string& name, std::unique_ptr<Command> cmd);
    
    // Execute a command
    int runCommand(const std::string& name, const std::vector<std::string>& args);
    
    // Get list of available commands
    std::vector<std::string> getCommandList() const;
};

// Common utility functions
namespace Utils {
    std::string getWorkingDirectory();
    bool changeDirectory(const std::string& path);
    bool copyFile(const std::string& src, const std::string& dest);
    bool removeFile(const std::string& path);
    bool makeDirectory(const std::string& path);
}

// Common command implementations
namespace Commands {

class CatCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

class LsCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

class CpCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

class MvCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

class RmCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

class MkdirCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

class PwdCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

class CdCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

class EchoCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
};

} // namespace Commands} // namespace Commands

} // namespace FuzzyBox

#endif // FUZZYLIB_H