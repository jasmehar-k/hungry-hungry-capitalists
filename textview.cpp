#include "textview.h"

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "board.h"
#include "controller.h"
#include "district.h"
#include "game.h"
#include "player.h"

// The View. It renders the whole board on every update rather than tracking
// what changed, because the spec redraws after every command; that is also why
// it needs no knowledge of which command ran.
//
// It reads the Model directly through Game's const accessors instead of being
// handed a copy of the data, which is the arrangement Chapter 17 describes for
// MVC. Its output stream is injected, so the same View could render to a file.

namespace {

// A full row is 11 cells plus the separators around them.
constexpr std::size_t kRowWidth = kGridSize * (kCellWidth + 1) + 1;
// What is left between the left and right columns on the nine middle rows.
constexpr std::size_t kInteriorWidth = kRowWidth - 2 * (kCellWidth + 1) - 2;

// Truncate before padding: the subtraction below is unsigned, so a string
// wider than the cell would otherwise wrap around to an enormous length.
std::string pad(std::string text, std::size_t width) {
    if (text.size() > width) text.resize(width);
    text.append(width - text.size(), ' ');
    return text;
}

std::string leftRight(std::string left, std::string right, std::size_t width) {
    // Clamp the right side first: without this, a right side wider than the
    // cell makes the padding length underflow and std::string::append throws.
    if (right.size() > width) right.resize(width);
    if (left.size() + right.size() > width) left.resize(width - right.size());
    left.append(width - left.size() - right.size(), ' ');
    left += right;
    return left;
}

std::string centre(const std::string &text, std::size_t width) {
    if (text.size() >= width) return pad(text, width);
    const std::size_t lead = (width - text.size()) / 2;
    return pad(std::string(lead, ' ') + text, width);
}

// Split a district name across the two name lines of a cell, breaking on a
// space where one exists and hard-cutting only when a single word is too long.
std::pair<std::string, std::string> wrapName(const std::string &name, std::size_t width) {
    if (name.size() <= width) return {name, ""};
    const std::size_t cut = name.rfind(' ', width);
    if (cut == std::string::npos) return {name.substr(0, width), name.substr(width)};
    return {name.substr(0, cut), name.substr(cut + 1)};
}

// The figure colours a Workshop's Guild; a text display marks it with a letter.
std::string guildTag(Guild guild) {
    switch (guild) {
        case Guild::Ember: return "E ";
        case Guild::Smoke: return "S ";
        case Guild::Arcane: return "A ";
        case Guild::Gilded: return "G ";
        case Guild::None: break;
    }
    return "";
}

std::string fullBorder() {
    std::string border{'+'};
    for (std::size_t i = 0; i < kGridSize; ++i) {
        border.append(kCellWidth, '-');
        border += '+';
    }
    return border;
}

std::string interiorBorder() {
    std::string border{'+'};
    border.append(kCellWidth, '-');
    border += '+';
    border.append(kInteriorWidth, ' ');
    border += '+';
    border.append(kCellWidth, '-');
    border += '+';
    return border;
}

// Squares are laid out clockwise with Grand Entrance at the bottom right.
// Squares are numbered clockwise but the board is drawn top-down, so each
// edge maps its own column or row index onto the circuit's numbering. The
// bottom and left edges therefore count downward.
int topSquare(std::size_t col) { return static_cast<int>(20 + col); }        // 20..30
int bottomSquare(std::size_t col) { return static_cast<int>(10 - col); }     // 10..0
int leftSquare(std::size_t row) { return static_cast<int>(20 - row); }       // 19..11
int rightSquare(std::size_t row) { return static_cast<int>(30 + row); }      // 31..39

// The open middle of the board, used for the title and a layout legend.
std::vector<std::string> interiorLines() {
    std::vector<std::string> lines((kGridSize - 2) * kCellLines, std::string(kInteriorWidth, ' '));
    const auto put = [&lines](std::size_t index, const std::string &text) {
        if (index < lines.size()) lines[index] = centre(text, kInteriorWidth);
    };
    put(3, "ArcaneMarket");
    put(5, "Circuit Board - 40 Districts");
    put(9, "Ember 1 2 6 8      Smoke 11 13 16      Arcane 21 22 25 27      Gilded 31 33 35 37 39");
    put(11, "Forges 4 18 24 29      Depots 9 14");
    put(15, "Play runs clockwise from Grand Entrance (sq 0)");
    put(19, "cell:  upgrades / owner       name       buy price / tokens here");
    return lines;
}

}  // namespace

// Registration is tied to the object's lifetime, which is RAII applied to a
// subscription rather than to memory: the Subject cannot end up holding a
// pointer to a View that no longer exists.
TextView::TextView(Game &game, std::shared_ptr<ControllerInterface> controller,
                   std::ostream &out)
    : game_{game}, controller_{std::move(controller)}, out_{out} {
    game_.addObserver(this);
}

TextView::~TextView() { game_.removeObserver(this); }

void TextView::update() { render(out_); }

// The View owns its Controller as a Strategy and forwards input to it without
// interpreting the line, so swapping the Controller changes what input does
// without touching either the View or the Model.
void TextView::handleInput(const std::string &line) {
    if (controller_) controller_->handleCommand(line);
}

// Several Artificers may share a square, so a cell shows all of their tokens.
// Six is the roster cap, which is what keeps this inside the cell width.
std::string TextView::tokensOn(int square) const {
    std::string tokens;
    for (std::size_t i = 0; i < game_.playerCount(); ++i) {
        const Player &player = game_.player(i);
        if (player.position() == square) tokens += player.token();
    }
    return tokens;
}

std::array<std::string, kCellLines> TextView::renderCell(const District &district) const {
    std::string flags = guildTag(district.guild());
    if (district.isMortgaged()) {
        flags += "MORT";
    } else {
        flags.append(static_cast<std::size_t>(district.upgradeCount()), 'U');
    }

    std::string price;
    if (district.buyPrice() > 0) {
        price = std::to_string(district.buyPrice());
    }

    const auto [firstLine, secondLine] = wrapName(district.name(), kCellWidth);

    return {leftRight(flags, std::string(1, district.ownerInitial()), kCellWidth),
            pad(firstLine, kCellWidth), pad(secondLine, kCellWidth),
            leftRight(price, tokensOn(district.square()), kCellWidth)};
}

void TextView::render(std::ostream &out) const {
    const Board &board = game_.board();
    const std::string border = fullBorder();
    const std::string openBorder = interiorBorder();
    const std::vector<std::string> interior = interiorLines();

    // A row of eleven cells, used for the top and bottom edges of the circuit.
    const auto renderFullRow = [&](int (*squareAt)(std::size_t)) {
        std::vector<std::array<std::string, kCellLines>> cells;
        cells.reserve(kGridSize);
        for (std::size_t col = 0; col < kGridSize; ++col) {
            cells.push_back(renderCell(board.at(squareAt(col))));
        }
        for (std::size_t line = 0; line < kCellLines; ++line) {
            out << '|';
            for (const auto &cell : cells) out << cell[line] << '|';
            out << '\n';
        }
    };

    out << border << '\n';
    renderFullRow(topSquare);
    out << border << '\n';

    for (std::size_t row = 1; row < kGridSize - 1; ++row) {
        const auto left = renderCell(board.at(leftSquare(row)));
        const auto right = renderCell(board.at(rightSquare(row)));
        for (std::size_t line = 0; line < kCellLines; ++line) {
            out << '|' << left[line] << '|' << interior[(row - 1) * kCellLines + line] << '|'
                << right[line] << "|\n";
        }
        out << (row == kGridSize - 2 ? border : openBorder) << '\n';
    }

    renderFullRow(bottomSquare);
    out << border << '\n';

    if (game_.playerCount() > 0) {
        const Player &player = game_.currentPlayer();
        out << player.name() << " (" << player.token() << ")  " << player.reagents()
            << " Reagents  on " << board.at(player.position()).name() << "  ["
            << game_.turnStateName() << ']';
        if (player.inLockup()) {
            out << "  detained, " << player.lockupTurns() << " turns served";
        }
        if (player.freePasses() > 0) {
            out << "  Free-Pass x" << player.freePasses();
        }
        out << '\n';
    }
}
