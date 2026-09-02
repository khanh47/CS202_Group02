#pragma once

#include "Commands/ICommand.h"

class ToggleGridCommand : public ICommand {
public:
    void execute() override;
    std::string getName() const override { return "ToggleGridCommand"; }
    CommandType getType() const override { return CommandType::IMMEDIATE; }
};

class ToggleCoordinatesCommand : public ICommand {
public:
    void execute() override;
    std::string getName() const override { return "ToggleCoordinatesCommand"; }
    CommandType getType() const override { return CommandType::IMMEDIATE; }
};

class ToggleHitboxCommand : public ICommand {
public:
    void execute() override;
    std::string getName() const override { return "ToggleHitboxCommand"; }
    CommandType getType() const override { return CommandType::IMMEDIATE; }
};

class ToggleFreeCameraCommand : public ICommand {
public:
    void execute() override;
    std::string getName() const override { return "ToggleFreeCameraCommand"; }
    CommandType getType() const override { return CommandType::IMMEDIATE; }
};
