#ifndef FUZZYLIB_HPP
#define FUZZYLIB_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>

namespace FuzzyBox {

class Command {
public:
    virtual ~Command() = default;
    virtual int execute(const std::vector<std::string>& args) = 0;
    virtual std::string help() const = 0;
};

class FuzzyShell {
private:
    std::map<std::string, std::unique_ptr<Command>> commands;
    bool running;
    std::string prompt;

public:
    FuzzyShell(const std::string& p = "$ ") : running(true), prompt(p) {}
    
    void registerCommand(const std::string& name, std::unique_ptr<Command> cmd);
    void run();
    void stop() { running = false; }
    
    // Command execution helpers
    int executeCommand(const std::string& name, const std::vector<std::string>& args);
    std::vector<std::string> parseCommand(const std::string& cmdline);
    void displayHelp(const std::string& command = "");
};

namespace Commands {

class CatCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

class LsCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

class CpCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

class MvCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

class RmCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

class MkdirCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

class PwdCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

class CdCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

class EchoCommand : public Command {
public:
    int execute(const std::vector<std::string>& args) override;
    std::string help() const override;
};

} // namespace Commands

} // namespace FuzzyBox

#endif // FUZZYLIB_HPP