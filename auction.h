#ifndef AUCTION_H
#define AUCTION_H

class Game;
class OwnableDistrict;

// The Guild's open auction: each solvent Artificer in turn raises or
// withdraws until one remains. A separate class rather than a Game method
// because the bidding loop is its own responsibility — a self-contained
// interactive dialogue — while Game holds the rules; folding it in gave Game
// one more reason to change than it should carry.
class Auction {
public:
    explicit Auction(Game &game) : game_{game} {}

    // Sells `district` to the last remaining bidder at their final bid. If
    // every Artificer withdraws without a bid, nothing changes hands.
    void run(OwnableDistrict &district);

private:
    Game &game_;
};

#endif  // AUCTION_H
