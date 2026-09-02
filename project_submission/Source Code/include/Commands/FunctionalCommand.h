#pragma once
#include "ICommand.h"
#include <functional>
#include <string>

class FunctionalCommand : public ICommand {
private:
    std::string _name;
    std::function<void()> _action;
    CommandType _type;

public:
    FunctionalCommand(const std::string& name, 
                      std::function<void()> action,
                      CommandType type = CommandType::NON_STATE)
        : _name(name), _action(std::move(action)), _type(type) {}

    void execute() override {
        if (_action) {
            _action();
        }
    }

    std::string getName() const override {
        return _name;
    }

    CommandType getType() const override {
        return _type;
    }
};