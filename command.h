#ifndef COMMAND_H
#define COMMAND_H

#include <iosfwd>
#include <string>
#include <utility>

class Game;

// The Command hierarchy. Every line the user types that the parser recognises
// becomes one of these objects, holding both its receiver and its already
// parsed arguments, so executing it needs no further interpretation.
//
// That is what separates what was asked for from who asked and who carries it
// out: GameController fires execute() through this interface without knowing
// which command it holds, and adding a command changes neither the Controller
// nor any existing command.

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

// Reports why an input was refused. Having a command carry the diagnostic
// keeps console output in the Command layer instead of pushing it into the
// parser, and lets a specific complaint replace a generic "I don't understand".
class MessageCommand final : public Command {
public:
    MessageCommand(std::ostream &out, std::string message)
        : out_{out}, message_{std::move(message)} {}
    void execute() override;

private:
    std::ostream &out_;
    std::string message_;
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

class TradeCommand final : public Command {
public:
    TradeCommand(Game &game, std::string target, std::string give, std::string receive)
        : game_{game},
          target_{std::move(target)},
          give_{std::move(give)},
          receive_{std::move(receive)} {}
    void execute() override;

private:
    Game &game_;
    std::string target_;
    std::string give_;
    std::string receive_;
};

class AcceptCommand final : public Command {
public:
    explicit AcceptCommand(Game &game) : game_{game} {}
    void execute() override;

private:
    Game &game_;
};

class RejectCommand final : public Command {
public:
    explicit RejectCommand(Game &game) : game_{game} {}
    void execute() override;

private:
    Game &game_;
};

class UpgradeCommand final : public Command {
public:
    UpgradeCommand(Game &game, std::string district, bool buying)
        : game_{game}, district_{std::move(district)}, buying_{buying} {}
    void execute() override;

private:
    Game &game_;
    std::string district_;
    bool buying_;
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
