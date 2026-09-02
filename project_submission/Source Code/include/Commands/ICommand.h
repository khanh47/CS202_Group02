#pragma once
#include <memory>
#include <string>

enum class CommandType {
    STATE_CHANGING,
    NON_STATE,
    IMMEDIATE
};

class ICommand {
public:
    virtual ~ICommand() = default;

    virtual void execute() = 0;
    virtual std::string getName() const = 0;
    virtual CommandType getType() const = 0;
};
