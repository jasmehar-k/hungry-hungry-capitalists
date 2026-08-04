#include "game.h"

#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>
#include <utility>

#include "boarddata.h"
#include "district.h"
#include "player.h"
#include "turn.h"
#include "workshop.h"

Game::Game(bool testingMode, unsigned seed, bool shuffledDeck)
    : testingMode_{testingMode}, rng_{seed}, dice_{rng_} {
    if (shuffledDeck) deck_ = std::make_unique<CardDeck>(rng_);
    turn_ = std::make_unique<Turn>(*this);
}

int Game::drawEnchantmentValue() {
    if (deck_ != nullptr) return deck_->draw();
    return drawEnchantment(rng_).change;
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

    if (const int laps = Board::grandEntrancePasses(from, steps); laps > 0) {
        player.addReagents(kGrandEntranceBonus * laps);
        std::cout << player.name() << " passes the Grand Entrance and collects "
                  << (kGrandEntranceBonus * laps) << " Reagents\n";
    }

    std::cout << player.name() << " lands on " << board_.at(to).name() << '\n';
    board_.at(to).landOn(*this, player);
}

void Game::sendToLockup(Player &player) {
    player.sendToLockup();
    std::cout << player.name() << " is sent to The Lockup\n";
}

bool Game::offerLockupExit(Player &player) {
    const bool canPay = player.reagents() >= kLockupFine;
    const bool hasToken = player.freePasses() > 0;
    // With neither route affordable, rolling for doubles is the only option
    // and there is nothing to ask about.
    if (!canPay && !hasToken) return false;

    while (true) {
        std::ostringstream question;
        question << "leave The Lockup how? [roll";
        if (canPay) question << "/pay";
        if (hasToken) question << "/token";
        question << "] ";
        const std::string answer = prompt(question.str());

        // A blank answer, including end of input, rolls for doubles.
        if (answer == "roll" || answer.empty()) return false;

        if (answer == "pay" && canPay) {
            payTreasury(player, kLockupFine);
            player.releaseFromLockup();
            std::cout << player.name() << " leaves The Lockup\n";
            return true;
        }
        if (answer == "token" && hasToken) {
            player.setFreePasses(player.freePasses() - 1);
            player.releaseFromLockup();
            std::cout << player.name() << " spends a Free-Pass Token to leave The Lockup\n";
            return true;
        }
        std::cout << "please answer roll";
        if (canPay) std::cout << ", pay";
        if (hasToken) std::cout << " or token";
        std::cout << '\n';
    }
}

bool Game::forceLockupExit(Player &player) {
    if (player.freePasses() > 0) {
        player.setFreePasses(player.freePasses() - 1);
        player.releaseFromLockup();
        std::cout << player.name() << " spends a Free-Pass Token to leave The Lockup\n";
        return true;
    }
    // payTreasury parks a debt when the fine is unaffordable, which is what
    // stops an Artificer who cannot pay from being detained forever: the debt
    // blocks the turn until they raise funds or declare insolvency.
    payTreasury(player, kLockupFine);
    if (hasPendingDebt()) return false;

    player.releaseFromLockup();
    return true;
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

void Game::transferDistrict(OwnableDistrict &district, Player &to) {
    district.setOwner(&to);
    if (!district.isMortgaged()) return;
    // Receiving a mortgaged district costs 10% of its printed value now; the
    // later unmortgage still costs the full 60%.
    const int fee = district.buyPrice() / 10;
    to.addReagents(-fee);
    treasury_ += fee;
    std::cout << to.name() << " pays " << fee << " Reagents on the mortgaged "
              << district.name() << '\n';
}

void Game::transferEverything(Player &from, Player &to) {
    to.addReagents(from.reagents());
    from.setReagents(0);
    to.setFreePasses(to.freePasses() + from.freePasses());
    from.setFreePasses(0);

    for (int square = 0; square < Board::size(); ++square) {
        auto *ownable = dynamic_cast<OwnableDistrict *>(&board_.at(square));
        if (!ownable || ownable->owner() != &from) continue;
        transferDistrict(*ownable, to);
    }
    std::cout << to.name() << " receives everything " << from.name() << " held\n";
}

bool Game::guildIsUnimproved(Guild guild) const {
    if (guild == Guild::None) return true;  // Forges and Depots take no upgrades
    for (int square = 0; square < Board::size(); ++square) {
        const District &district = board_.at(square);
        if (district.kind() != DistrictKind::Workshop || district.guild() != guild) continue;
        if (district.upgradeCount() > 0) return false;
    }
    return true;
}

std::string Game::tradeTargetName() const {
    return offer_.active && offer_.to != nullptr ? offer_.to->name() : std::string{};
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

namespace {

// Shared lookup for the two upgrade commands: everything both must reject
// before the level itself is considered.
Workshop *findUpgradableWorkshop(Board &board, Player &player, const std::string &name) {
    OwnableDistrict *district = board.findOwnable(name);
    if (district == nullptr) {
        std::cout << "there is no ownable district called " << name << '\n';
        return nullptr;
    }
    auto *workshop = dynamic_cast<Workshop *>(district);
    if (workshop == nullptr) {
        std::cout << district->name() << " is not a Workshop, so it takes no upgrades\n";
        return nullptr;
    }
    if (workshop->owner() != &player) {
        std::cout << workshop->name() << " is not yours\n";
        return nullptr;
    }
    if (workshop->isMortgaged()) {
        std::cout << workshop->name() << " is mortgaged\n";
        return nullptr;
    }
    return workshop;
}

}  // namespace

bool Game::buyUpgrade(const std::string &districtName) {
    Player &player = currentPlayer();
    Workshop *workshop = findUpgradableWorkshop(board_, player, districtName);
    if (workshop == nullptr) return false;

    if (!guildComplete(workshop->guild(), player)) {
        std::cout << "you must own every Workshop in that Crafting Guild first\n";
        return false;
    }
    if (workshop->upgradeCount() >= kMaxUpgrades) {
        std::cout << workshop->name() << " already has a Master Atelier\n";
        return false;
    }

    const int cost = workshop->upgradeCost();
    if (player.reagents() < cost) {
        std::cout << "an upgrade on " << workshop->name() << " costs " << cost
                  << " Reagents\n";
        return false;
    }

    player.addReagents(-cost);
    treasury_ += cost;
    workshop->setUpgrades(workshop->upgradeCount() + 1);
    std::cout << player.name() << " upgrades " << workshop->name() << " to level "
              << workshop->upgradeCount() << " for " << cost << " Reagents\n";
    return true;
}

bool Game::sellUpgrade(const std::string &districtName) {
    Player &player = currentPlayer();
    Workshop *workshop = findUpgradableWorkshop(board_, player, districtName);
    if (workshop == nullptr) return false;

    if (workshop->upgradeCount() <= 0) {
        std::cout << workshop->name() << " has no upgrades to sell\n";
        return false;
    }

    const int refund = workshop->upgradeCost() / 2;
    workshop->setUpgrades(workshop->upgradeCount() - 1);
    player.addReagents(refund);
    std::cout << player.name() << " sells an upgrade on " << workshop->name() << " for "
              << refund << " Reagents, leaving level " << workshop->upgradeCount() << '\n';
    // Selling upgrades is one of the ways the spec lets an Artificer raise
    // funds against an outstanding debt.
    settleDebtIfPossible();
    return true;
}

namespace {

// A trade side is Reagents when it is all digits, and a district name
// otherwise. The range check keeps a very long digit string from overflowing.
bool parseReagents(const std::string &text, int &out) {
    if (text.empty()) return false;
    for (char character : text) {
        if (!std::isdigit(static_cast<unsigned char>(character))) return false;
    }
    std::istringstream parsed{text};
    long long value = 0;
    if (!(parsed >> value) || value <= 0 || value > 1000000000LL) return false;
    out = static_cast<int>(value);
    return true;
}

std::string sideText(const OwnableDistrict *district, int reagents) {
    return district != nullptr ? district->name() : std::to_string(reagents) + " Reagents";
}

}  // namespace

bool Game::offerTrade(const std::string &targetName, const std::string &give,
                      const std::string &receive) {
    if (offer_.active) {
        std::cout << "a trade offer is already awaiting an answer\n";
        return false;
    }

    Player &from = currentPlayer();
    Player *to = nullptr;
    for (const auto &candidate : players_) {
        if (candidate->name() == targetName && !candidate->isInsolvent()) {
            to = candidate.get();
        }
    }
    if (to == nullptr) {
        std::cout << "there is no solvent Artificer called " << targetName << '\n';
        return false;
    }
    if (to == &from) {
        std::cout << "you cannot trade with yourself\n";
        return false;
    }

    TradeOffer offer;
    offer.from = &from;
    offer.to = to;

    const bool giveIsReagents = parseReagents(give, offer.giveReagents);
    const bool receiveIsReagents = parseReagents(receive, offer.receiveReagents);
    if (giveIsReagents && receiveIsReagents) {
        std::cout << "a Reagent-for-Reagent trade is not allowed\n";
        return false;
    }

    // Each side is validated against whoever is parting with it.
    if (giveIsReagents) {
        if (from.reagents() < offer.giveReagents) {
            std::cout << from.name() << " does not hold " << offer.giveReagents
                      << " Reagents\n";
            return false;
        }
    } else {
        offer.giveDistrict = board_.findOwnable(give);
        if (offer.giveDistrict == nullptr) {
            std::cout << "there is no ownable district called " << give << '\n';
            return false;
        }
        if (offer.giveDistrict->owner() != &from) {
            std::cout << offer.giveDistrict->name() << " is not yours to trade\n";
            return false;
        }
        if (!guildIsUnimproved(offer.giveDistrict->guild())) {
            std::cout << offer.giveDistrict->name()
                      << " cannot be traded while its Guild holds upgrades\n";
            return false;
        }
    }

    if (receiveIsReagents) {
        if (to->reagents() < offer.receiveReagents) {
            std::cout << to->name() << " does not hold " << offer.receiveReagents
                      << " Reagents\n";
            return false;
        }
    } else {
        offer.receiveDistrict = board_.findOwnable(receive);
        if (offer.receiveDistrict == nullptr) {
            std::cout << "there is no ownable district called " << receive << '\n';
            return false;
        }
        if (offer.receiveDistrict->owner() != to) {
            std::cout << offer.receiveDistrict->name() << " does not belong to " << to->name()
                      << '\n';
            return false;
        }
        if (!guildIsUnimproved(offer.receiveDistrict->guild())) {
            std::cout << offer.receiveDistrict->name()
                      << " cannot be traded while its Guild holds upgrades\n";
            return false;
        }
    }

    offer.active = true;
    offer_ = offer;
    turn_->enterTradeResponse();
    std::cout << from.name() << " offers " << sideText(offer_.giveDistrict, offer_.giveReagents)
              << " to " << to->name() << " for "
              << sideText(offer_.receiveDistrict, offer_.receiveReagents) << '\n'
              << to->name() << ": accept or reject\n";
    return true;
}

void Game::acceptTrade() {
    if (!offer_.active) {
        std::cout << "there is no trade offer to answer\n";
        return;
    }
    const TradeOffer offer = offer_;
    offer_ = TradeOffer{};

    if (offer.giveDistrict != nullptr) {
        transferDistrict(*offer.giveDistrict, *offer.to);
    } else {
        offer.from->addReagents(-offer.giveReagents);
        offer.to->addReagents(offer.giveReagents);
    }

    if (offer.receiveDistrict != nullptr) {
        transferDistrict(*offer.receiveDistrict, *offer.from);
    } else {
        offer.to->addReagents(-offer.receiveReagents);
        offer.from->addReagents(offer.receiveReagents);
    }

    std::cout << offer.to->name() << " accepts the trade\n";
    turn_->resumeAfterTrade();
    // Trading is one of the ways the spec lets an Artificer raise funds.
    settleDebtIfPossible();
}

void Game::rejectTrade() {
    if (!offer_.active) {
        std::cout << "there is no trade offer to answer\n";
        return;
    }
    std::cout << offer_.to->name() << " rejects the trade\n";
    offer_ = TradeOffer{};
    turn_->resumeAfterTrade();
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

int Game::totalFreePasses() const {
    int total = 0;
    for (const auto &player : players_) total += player->freePasses();
    return total;
}

bool Game::tryGrantFreePass(Player &player) {
    if (totalFreePasses() >= kMaxFreePassesInPlay) return false;

    std::uniform_int_distribution<int> chance{1, 100};
    if (chance(rng_) > kFreePassPercent) return false;

    player.setFreePasses(player.freePasses() + 1);
    std::cout << player.name() << " receives a Free-Pass Token\n";
    return true;
}

std::string Game::prompt(const std::string &question) const {
    std::cout << question;
    std::cout.flush();
    std::string line;
    std::getline(std::cin, line);
    return line;
}
