#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <utility>

class Game;

// Ch 20 declares the Command interface as execute() "and, when undo is
// required, its inverse, undo()". ArcaneMarket has no undo requirement, so the
// interface carries execute() alone rather than an undo() every command would
// have to stub out.
//
// Each concrete command binds its receiver and its arguments at construction,
// per the chapter, so commands are built fresh from each input line.
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;

protected:
    Command() = default;
};

class RollCommand final : public Command {
public:
    explicit RollCommand(Game &game) : game_{game} {}
    RollCommand(Game &game, int die1, int die2) : game_{game}, die1_{die1}, die2_{die2} {}
    void execute() override;

private:
    Game &game_;
    int die1_ = -1;  // negative means roll randomly
    int die2_ = -1;
};

class NextCommand final : public Command {
public:
    explicit NextCommand(Game &game) : game_{game} {}
    void execute() override;

private:
    Game &game_;
};

class MortgageCommand final : public Command {
public:
    MortgageCommand(Game &game, std::string district)
        : game_{game}, district_{std::move(district)} {}
    void execute() override;

private:
    Game &game_;
    std::string district_;
};

class UnmortgageCommand final : public Command {
public:
    UnmortgageCommand(Game &game, std::string district)
        : game_{game}, district_{std::move(district)} {}
    void execute() override;

private:
    Game &game_;
    std::string district_;
};

class InsolventCommand final : public Command {
public:
    explicit InsolventCommand(Game &game) : game_{game} {}
    void execute() override;

private:
    Game &game_;
};

class AssetsCommand final : public Command {
public:
    explicit AssetsCommand(Game &game) : game_{game} {}
    void execute() override;

private:
    Game &game_;
};

class AllCommand final : public Command {
public:
    explicit AllCommand(Game &game) : game_{game} {}
    void execute() override;

private:
    Game &game_;
};

class SaveCommand final : public Command {
public:
    SaveCommand(Game &game, std::string filename)
        : game_{game}, filename_{std::move(filename)} {}
    void execute() override;

private:
    Game &game_;
    std::string filename_;
};

#endif  // COMMAND_H
