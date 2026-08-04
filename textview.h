#ifndef CS247_TEXTVIEW_H
#define CS247_TEXTVIEW_H

#include <array>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>

#include "observer.h"

class ControllerInterface;
class District;
class Game;

inline constexpr std::size_t kCellWidth = 12;
inline constexpr std::size_t kCellLines = 4;
inline constexpr std::size_t kGridSize = 11;

// The View. Observes the Model and redraws on notification, queries the Model
// directly for the state it needs, and delegates user input to its Controller.
class TextView final : public Observer {
public:
    // Registers itself as an observer of the Model and deregisters on destruction.
    TextView(Game &game, std::shared_ptr<ControllerInterface> controller);
    ~TextView() override;

    void update() override;
    void render(std::ostream &out) const;

    // User input reaches the View and is passed to its Strategy.
    void handleInput(const std::string &line);

private:
    std::array<std::string, kCellLines> renderCell(const District &district) const;
    std::string tokensOn(int square) const;

    Game &game_;
    std::shared_ptr<ControllerInterface> controller_;
};

#endif  // CS247_TEXTVIEW_H
