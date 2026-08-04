#include "game.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <utility>

#include "boarddata.h"
#include "district.h"
#include "player.h"
#include "turn.h"
#include "workshop.h"

Game::Game(bool testingMode, unsigned seed)
    : testingMode_{testingMode}, rng_{seed}, dice_{rng_} {
    turn_ = std::make_unique<Turn>(*this);
}

Game::~Game() = default;

bool Game::tokenTaken(char token) const {
    for (const auto &player : players_) {
        if (player->token() == token) return true;
    }
    return false;
}

bool Game::addPlayer(std::string name, char token) {
    if (players_.size() >= kMaxPlayers || tokenTaken(token)) return false;
    players_.push_back(std::make_unique<Player>(std::move(name), token));
    return true;
}

const Player &Game::player(std::size_t index) const {
    assert(index < players_.size());
    return *players_[index];
}

Player &Game::player(std::size_t index) {
    assert(index < players_.size());
    return *players_[index];
}

const Player &Game::currentPlayer() const { return player(current_); }

Player &Game::currentPlayer() { return player(current_); }

void Game::setCurrentPlayer(std::size_t index) {
    assert(index < players_.size());
    current_ = index;
}

void Game::endTurn() {
    if (players_.empty() || solventCount() == 0) return;
    if (hasPendingDebt()) {
        std::cout << "settle the debt or declare insolvency before ending your turn\n";
        return;
    }
    if (isOver()) return;

    do {
        current_ = (current_ + 1) % players_.size();
    } while (players_[current_]->isInsolvent());

    turn_->beginTurn();
    std::cout << "it is now " << currentPlayer().name() << "'s turn\n";
}

void Game::movePlayer(Player &player, int steps) {
    const int from = player.position();
    const int to = Board::advance(from, steps);
    player.setPosition(to);

    if (Board::passedGrandEntrance(from, steps)) {
        player.addReagents(kGrandEntranceBonus);
        std::cout << player.name() << " passes the Grand Entrance and collects "
                  << kGrandEntranceBonus << " Reagents\n";
    }

    std::cout << player.name() << " lands on " << board_.at(to).name() << '\n';
    board_.at(to).landOn(*this, player);
}

void Game::sendToLockup(Player &player) {
    player.sendToLockup();
    std::cout << player.name() << " is sent to The Lockup\n";
}

bool Game::forceLockupExit(Player &player) {
    if (player.freePasses() > 0) {
        player.setFreePasses(player.freePasses() - 1);
        player.releaseFromLockup();
        std::cout << player.name() << " spends a Free-Pass Token to leave The Lockup\n";
        return true;
    }
    if (player.reagents() >= kLockupFine) {
        payTreasury(player, kLockupFine);
        player.releaseFromLockup();
        std::cout << player.name() << " pays the " << kLockupFine << "-Reagent fine\n";
        return true;
    }
    std::cout << player.name() << " cannot pay the " << kLockupFine << "-Reagent fine\n";
    return false;
}

void Game::payTreasury(Player &player, int amount) {
    if (amount <= 0) return;
    if (player.reagents() >= amount) {
        player.addReagents(-amount);
        treasury_ += amount;
        std::cout << player.name() << " pays " << amount << " Reagents to the Guild Treasury\n";
        return;
    }
    debtor_ = &player;
    creditor_ = nullptr;
    debtAmount_ = amount;
    std::cout << player.name() << " owes " << amount
              << " Reagents to the Guild Treasury but holds only " << player.reagents()
              << "\nraise funds by mortgaging, or use the insolvent command\n";
}

void Game::payPlayer(Player &from, Player &to, int amount) {
    if (amount <= 0) return;
    if (from.reagents() >= amount) {
        from.addReagents(-amount);
        to.addReagents(amount);
        std::cout << from.name() << " pays " << amount << " Reagents to " << to.name() << '\n';
        return;
    }
    debtor_ = &from;
    creditor_ = &to;
    debtAmount_ = amount;
    std::cout << from.name() << " owes " << amount << " Reagents to " << to.name()
              << " but holds only " << from.reagents()
              << "\nraise funds by mortgaging, or use the insolvent command\n";
}

void Game::settleDebtIfPossible() {
    if (!hasPendingDebt()) return;
    if (debtor_->reagents() < debtAmount_) return;

    if (creditor_ != nullptr) {
        debtor_->addReagents(-debtAmount_);
        creditor_->addReagents(debtAmount_);
        std::cout << debtor_->name() << " settles " << debtAmount_ << " Reagents with "
                  << creditor_->name() << '\n';
    } else {
        debtor_->addReagents(-debtAmount_);
        treasury_ += debtAmount_;
        std::cout << debtor_->name() << " settles " << debtAmount_
                  << " Reagents with the Guild Treasury\n";
    }
    debtor_ = nullptr;
    creditor_ = nullptr;
    debtAmount_ = 0;
}

void Game::transferEverything(Player &from, Player &to) {
    to.addReagents(from.reagents());
    from.setReagents(0);
    to.setFreePasses(to.freePasses() + from.freePasses());
    from.setFreePasses(0);

    for (int square = 0; square < Board::size(); ++square) {
        auto *ownable = dynamic_cast<OwnableDistrict *>(&board_.at(square));
        if (!ownable || ownable->owner() != &from) continue;
        ownable->setOwner(&to);
        if (ownable->isMortgaged()) {
            // Receiving a mortgaged district costs 10% of its printed value now.
            const int fee = ownable->buyPrice() / 10;
            to.addReagents(-fee);
            treasury_ += fee;
            std::cout << to.name() << " pays " << fee << " Reagents on the mortgaged "
                      << ownable->name() << '\n';
        }
    }
    std::cout << to.name() << " receives everything " << from.name() << " held\n";
}

void Game::releaseToMarket(Player &from) {
    treasury_ += from.reagents();
    from.setReagents(0);
    // Tokens held by an Artificer insolvent to the Treasury are destroyed.
    from.setFreePasses(0);

    std::vector<OwnableDistrict *> released;
    for (int square = 0; square < Board::size(); ++square) {
        auto *ownable = dynamic_cast<OwnableDistrict *>(&board_.at(square));
        if (!ownable || ownable->owner() != &from) continue;
        ownable->setOwner(nullptr);
        ownable->setMortgaged(false);  // they return to the open market unmortgaged
        released.push_back(ownable);
    }

    for (OwnableDistrict *district : released) runAuction(*district);
}

void Game::declareInsolvency() {
    if (!hasPendingDebt()) {
        std::cout << "you may only declare insolvency when you owe more than you hold\n";
        return;
    }
    Player &debtor = *debtor_;
    if (debtor.reagents() >= debtAmount_) {
        std::cout << "you can cover this debt, so insolvency is not available\n";
        return;
    }

    std::cout << debtor.name() << " declares insolvency\n";
    Player *creditor = creditor_;
    debtor_ = nullptr;
    creditor_ = nullptr;
    debtAmount_ = 0;
    debtor.declareInsolvent();

    if (creditor != nullptr) {
        transferEverything(debtor, *creditor);
    } else {
        releaseToMarket(debtor);
    }

    if (const Player *champion = winner()) {
        std::cout << champion->name() << " is the last solvent Artificer and wins\n";
    }
}

bool Game::mortgage(const std::string &districtName) {
    Player &player = currentPlayer();
    OwnableDistrict *district = board_.findOwnable(districtName);
    if (district == nullptr) {
        std::cout << "there is no ownable district called " << districtName << '\n';
        return false;
    }
    if (district->owner() != &player) {
        std::cout << district->name() << " is not yours to mortgage\n";
        return false;
    }
    if (district->isMortgaged()) {
        std::cout << district->name() << " is already mortgaged\n";
        return false;
    }
    if (district->upgradeCount() > 0) {
        std::cout << "sell every upgrade on " << district->name() << " first\n";
        return false;
    }

    const int payout = district->buyPrice() / 2;
    district->setMortgaged(true);
    player.addReagents(payout);
    std::cout << player.name() << " mortgages " << district->name() << " for " << payout
              << " Reagents\n";
    settleDebtIfPossible();
    return true;
}

bool Game::unmortgage(const std::string &districtName) {
    Player &player = currentPlayer();
    OwnableDistrict *district = board_.findOwnable(districtName);
    if (district == nullptr) {
        std::cout << "there is no ownable district called " << districtName << '\n';
        return false;
    }
    if (district->owner() != &player) {
        std::cout << district->name() << " is not yours\n";
        return false;
    }
    if (!district->isMortgaged()) {
        std::cout << district->name() << " is not mortgaged\n";
        return false;
    }

    // Half the printed value plus a further 10%.
    const int cost = district->buyPrice() * 6 / 10;
    if (player.reagents() < cost) {
        std::cout << "unmortgaging " << district->name() << " costs " << cost
                  << " Reagents\n";
        return false;
    }
    player.addReagents(-cost);
    treasury_ += cost;
    district->setMortgaged(false);
    std::cout << player.name() << " unmortgages " << district->name() << " for " << cost
              << " Reagents\n";
    return true;
}

std::size_t Game::solventCount() const {
    std::size_t total = 0;
    for (const auto &player : players_) {
        if (!player->isInsolvent()) ++total;
    }
    return total;
}

const Player *Game::winner() const {
    if (players_.size() < kMinPlayers || solventCount() != 1) return nullptr;
    for (const auto &player : players_) {
        if (!player->isInsolvent()) return player.get();
    }
    return nullptr;
}

void Game::printAssets(const Player &player) const {
    std::cout << player.name() << " (" << player.token() << ") holds " << player.reagents()
              << " Reagents, " << player.freePasses() << " Free-Pass Tokens\n";
    for (int square = 0; square < Board::size(); ++square) {
        const auto *ownable = dynamic_cast<const OwnableDistrict *>(&board_.at(square));
        if (!ownable || ownable->owner() != &player) continue;
        std::cout << "  " << ownable->name();
        if (ownable->isMortgaged()) {
            std::cout << " (mortgaged)";
        } else if (ownable->upgradeCount() > 0) {
            std::cout << " (" << ownable->upgradeCount() << " upgrades)";
        }
        std::cout << '\n';
    }
}

void Game::printAllAssets() const {
    for (const auto &player : players_) {
        if (player->isInsolvent()) {
            std::cout << player->name() << " is insolvent\n";
            continue;
        }
        printAssets(*player);
    }
}

bool Game::guildComplete(Guild guild, const Player &owner) const {
    int inGuild = 0;
    int held = 0;
    for (int square = 0; square < Board::size(); ++square) {
        const District &district = board_.at(square);
        if (district.kind() != DistrictKind::Workshop || district.guild() != guild) continue;
        ++inGuild;
        const auto *ownable = static_cast<const OwnableDistrict *>(&district);
        if (ownable->owner() == &owner) ++held;
    }
    return inGuild > 0 && inGuild == held;
}

int Game::countOwned(DistrictKind kind, const Player &owner) const {
    int total = 0;
    for (int square = 0; square < Board::size(); ++square) {
        const District &district = board_.at(square);
        if (district.kind() != kind) continue;
        const auto *ownable = dynamic_cast<const OwnableDistrict *>(&district);
        if (ownable && ownable->owner() == &owner) ++total;
    }
    return total;
}

int Game::totalWorth(const Player &player) const {
    int worth = player.reagents();
    for (int square = 0; square < Board::size(); ++square) {
        const auto *ownable = dynamic_cast<const OwnableDistrict *>(&board_.at(square));
        if (!ownable || ownable->owner() != &player) continue;
        worth += ownable->buyPrice();
        if (const auto *workshop = dynamic_cast<const Workshop *>(ownable)) {
            worth += workshop->upgradeCount() * workshop->upgradeCost();
        }
    }
    return worth;
}

void Game::offerPurchase(OwnableDistrict &district, Player &buyer) {
    const int price = district.buyPrice();
    std::cout << district.name() << " is unowned, " << price << " Reagents\n";

    if (buyer.reagents() >= price) {
        const std::string answer = prompt("buy it? [yes/no] ");
        if (answer == "yes" || answer == "y") {
            buyer.addReagents(-price);
            district.setOwner(&buyer);
            std::cout << buyer.name() << " buys " << district.name() << " for " << price
                      << " Reagents\n";
            return;
        }
    } else {
        std::cout << buyer.name() << " cannot afford " << district.name() << '\n';
    }

    runAuction(district);
}

void Game::runAuction(OwnableDistrict &district) {
    std::vector<Player *> active;
    for (const auto &player : players_) {
        if (!player->isInsolvent()) active.push_back(player.get());
    }
    if (active.size() < 2) return;

    std::cout << "the Guild auctions " << district.name() << '\n';
    int highest = 0;
    Player *leader = nullptr;
    std::size_t index = 0;

    while (active.size() > 1) {
        Player *bidder = active[index];
        std::ostringstream question;
        question << bidder->name() << " holds " << bidder->reagents() << ", current bid "
                 << highest << " - raise or withdraw? [amount/withdraw] ";
        const std::string answer = prompt(question.str());

        // A blank answer, including end of input, counts as withdrawing, so a
        // closed stream can never leave the auction spinning.
        if (answer == "withdraw" || answer == "w" || answer.empty()) {
            std::cout << bidder->name() << " withdraws\n";
            active.erase(active.begin() + static_cast<std::ptrdiff_t>(index));
            if (index >= active.size()) index = 0;
            continue;
        }

        int bid = 0;
        std::istringstream parsed{answer};
        if (!(parsed >> bid) || bid <= highest || bid > bidder->reagents()) {
            std::cout << "a raise must beat " << highest << " and be within your Reagents\n";
            continue;  // the same Artificer tries again
        }
        highest = bid;
        leader = bidder;
        index = (index + 1) % active.size();
    }

    if (leader == nullptr) {
        // Everyone withdrew without bidding, so nothing changes hands.
        std::cout << "no bids were made; " << district.name() << " stays with the Guild\n";
        return;
    }
    leader->addReagents(-highest);
    district.setOwner(leader);
    std::cout << leader->name() << " wins " << district.name() << " for " << highest
              << " Reagents\n";
}

std::string Game::prompt(const std::string &question) const {
    std::cout << question;
    std::cout.flush();
    std::string line;
    std::getline(std::cin, line);
    return line;
}
