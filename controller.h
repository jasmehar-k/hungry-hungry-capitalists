#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>

class Game;

// The Invoker of the Command pattern and, at the same time, the Strategy the
// View holds for behaviour. Keeping it behind an interface is what lets the
// View be constructed with any controller, and it is the seam the system tests
// drive instead of a terminal.

// The View's Strategy for behaviour (Ch 17). Swapping the Controller changes
// what user input does without touching the View or the Model.
class ControllerInterface {
public:
    virtual ~ControllerInterface() = default;
    virtual void handleCommand(const std::string &line) = 0;

protected:
    ControllerInterface() = default;
};

class GameController final : public ControllerInterface {
public:
    explicit GameController(Game &game);
    void handleCommand(const std::string &line) override;

private:
    Game &game_;
};

#endif  // CONTROLLER_H
