#pragma once
#include <stdexcept>
#include <string>

class CompilerException : public std::runtime_error {
public:
    explicit CompilerException(const std::string& msg)
        : std::runtime_error(msg) {}
};
