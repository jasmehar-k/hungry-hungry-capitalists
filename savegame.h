#ifndef SAVEGAME_H
#define SAVEGAME_H

#include <string>

class Game;

// Load and save live together so the two halves of the format cannot drift.
// The spec warns that a game which fails to load costs significant marks, so
// load validates hard and reports why rather than half-restoring a game.
namespace SaveGame {

// Restores `game` from `filename`. On failure returns false, sets `error`, and
// leaves the game untouched.
bool load(Game &game, const std::string &filename, std::string &error);

bool save(const Game &game, const std::string &filename);

}  // namespace SaveGame

#endif  // SAVEGAME_H
