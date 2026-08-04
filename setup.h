#ifndef SETUP_H
#define SETUP_H

#include <iosfwd>

class Game;

namespace Setup {

// Asks for the number of Artificers, then a name and token for each, retrying
// on bad input. Returns false only if the input stream ends first.
bool runNewGame(Game &game, std::istream &in, std::ostream &out);

}  // namespace Setup

#endif  // SETUP_H
