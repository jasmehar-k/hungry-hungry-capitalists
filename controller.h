#ifndef CS247_CONTROLLER_H
#define CS247_CONTROLLER_H

#include <string>

class Game;

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

#endif  // CS247_CONTROLLER_H
