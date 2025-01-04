#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "lib/fuzzylib.hpp"

#define MAX_ARGS 64
#define MAX_LINE 1024
#define MAX_PATH 256
#define HISTORY_FILE ".ash_history"

std::map<std::string, std::string> aliases;
char current_dir[MAX_PATH];

void initialize_shell() {
    using_history();
    read_history(HISTORY_FILE);
    
    // Set up default aliases
    aliases["ll"] = "ls -la";
    aliases["cls"] = "clear";
    
    // Initialize readline
    rl_bind_key('\t', rl_complete);
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(line);
    
    while (std::getline(tokenStream, token, ' ')) {
        if (!token.empty()) {
            // Handle environment variables
            if (token[0] == '$') {
                const char* env_val = getenv(token.substr(1).c_str());
                if (env_val) token = env_val;
            }
            tokens.push_back(token);
        }
    }
    return tokens;
}

bool handle_builtin(const std::vector<std::string>& args) {
    if (args.empty()) return true;
    
    if (args[0] == "exit") return false;
    
    if (args[0] == "cd") {
        const char* dir = args.size() > 1 ? args[1].c_str() : getenv("HOME");
        if (chdir(dir) != 0) {
            perror("cd");
        }
        return true;
    }
    
    if (args[0] == "alias") {
        if (args.size() == 1) {
            for (const auto& pair : aliases) {
                printf("%s='%s'\n", pair.first.c_str(), pair.second.c_str());
            }
        } else if (args.size() == 3) {
            aliases[args[1]] = args[2];
        }
        return true;
    }
    
    return false;
}

int execute_pipeline(std::vector<std::vector<std::string>>& commands) {
    int num_cmds = commands.size();
    std::vector<int> pipes((num_cmds - 1) * 2);
    
    // Create all pipes
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(&pipes[i * 2]) < 0) {
            perror("pipe");
            return 1;
        }
    }
    
    // Execute all commands
    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            
            // Set up input pipe
            if (i > 0) {
                dup2(pipes[(i-1)*2], STDIN_FILENO);
            }
            
            // Set up output pipe
            if (i < num_cmds - 1) {
                dup2(pipes[i*2 + 1], STDOUT_FILENO);
            }
            
            // Close all pipes
            for (size_t j = 0; j < pipes.size(); j++) {
                close(pipes[j]);
            }
            
            // Convert vector<string> to char*[]
            std::vector<char*> c_args;
            for (auto& arg : commands[i]) {
                c_args.push_back(const_cast<char*>(arg.c_str()));
            }
            c_args.push_back(nullptr);
            
            execvp(c_args[0], c_args.data());
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    
    // Parent closes all pipes
    for (int pipe : pipes) {
        close(pipe);
    }
    
    // Wait for all children
    for (int i = 0; i < num_cmds; i++) {
        wait(nullptr);
    }
    
    return 1;
}

int main() {
    initialize_shell();
    char* input;
    int status = 1;
    
    while (status && (input = readline("# "))) {
        if (input[0] != '\0') {
            add_history(input);
            write_history(HISTORY_FILE);
            
            std::string line(input);
            std::vector<std::vector<std::string>> pipeline;
            std::istringstream pipe_stream(line);
            std::string command;
            
            // Split into pipeline segments
            while (std::getline(pipe_stream, command, '|')) {
                auto tokens = tokenize(command);
                if (!tokens.empty()) {
                    // Check for aliases
                    if (aliases.count(tokens[0])) {
                        auto alias_tokens = tokenize(aliases[tokens[0]]);
                        tokens.erase(tokens.begin());
                        tokens.insert(tokens.begin(), alias_tokens.begin(), alias_tokens.end());
                    }
                    pipeline.push_back(tokens);
                }
            }
            
            if (!pipeline.empty()) {
                if (pipeline.size() == 1 && !handle_builtin(pipeline[0])) {
                    status = 0;
                } else {
                    status = execute_pipeline(pipeline);
                }
            }
        }
        free(input);
    }
    
    printf("\n");
    return EXIT_SUCCESS;
}
