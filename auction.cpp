#include "auction.h"

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "district.h"
#include "game.h"
#include "player.h"

void Auction::run(OwnableDistrict &district) {
    std::vector<Player *> active;
    for (std::size_t i = 0; i < game_.playerCount(); ++i) {
        Player &player = game_.player(i);
        if (!player.isInsolvent()) active.push_back(&player);
    }
    if (active.size() < 2) return;

    game_.out() << "the Guild auctions " << district.name() << '\n';
    int highest = 0;
    Player *leader = nullptr;
    std::size_t index = 0;

    while (active.size() > 1) {
        Player *bidder = active[index];
        // Worded as an instruction rather than a question: "raise or withdraw?"
        // invites typing the word "raise", but a bare number is what is wanted.
        std::ostringstream question;
        question << bidder->name() << " holds " << bidder->reagents() << " - enter an amount "
                 << "above " << highest << ", or \"withdraw\": ";
        const std::string answer = game_.prompt(question.str());

        // A blank answer, including end of input, counts as withdrawing, so a
        // closed stream can never leave the auction spinning.
        if (answer == "withdraw" || answer == "w" || answer.empty()) {
            game_.out() << bidder->name() << " withdraws\n";
            active.erase(active.begin() + static_cast<std::ptrdiff_t>(index));
            if (index >= active.size()) index = 0;
            continue;
        }

        int bid = 0;
        std::istringstream parsed{answer};
        if (!(parsed >> bid) || bid <= highest || bid > bidder->reagents()) {
            game_.out() << "type just the number: more than " << highest << " and no more than "
                        << bidder->reagents() << ", or \"withdraw\"\n";
            continue;  // the same Artificer tries again
        }
        highest = bid;
        leader = bidder;
        index = (index + 1) % active.size();
    }

    if (leader == nullptr) {
        // Everyone withdrew without bidding, so nothing changes hands.
        game_.out() << "no bids were made; " << district.name() << " stays with the Guild\n";
        return;
    }
    leader->addReagents(-highest);
    district.setOwner(leader);
    game_.out() << leader->name() << " wins " << district.name() << " for " << highest
                << " Reagents\n";
}
