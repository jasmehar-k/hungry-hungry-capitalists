#include "game.h"

#include <cassert>
#include <cctype>
#include <istream>
#include <ostream>
#include <sstream>
#include <utility>

#include "auction.h"
#include "boarddata.h"
#include "district.h"
#include "player.h"
#include "turn.h"
#include "workshop.h"

Game::Game(bool testingMode, unsigned seed, bool shuffledDeck, std::istream &in,
           std::ostream &out)
    : testingMode_{testingMode}, in_{in}, out_{out}, rng_{seed}, dice_{rng_} {
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
    // Only the current Artificer's own debt blocks their turn; one parked
    // against someone else is settled when that Artificer's turn comes round.
    if (owes(currentPlayer())) {
        out_ << "settle the debt or declare insolvency before ending your turn\n";
        return;
    }
    if (isOver()) return;

    do {
        current_ = (current_ + 1) % players_.size();
    } while (players_[current_]->isInsolvent());

    turn_->beginTurn();
    out_ << "it is now " << currentPlayer().name() << "'s turn\n";
}

void Game::movePlayer(Player &player, int steps) {
    const int from = player.position();
    const int to = Board::advance(from, steps);
    player.setPosition(to);

    if (const int laps = Board::grandEntrancePasses(from, steps); laps > 0) {
        player.addReagents(kGrandEntranceBonus * laps);
        out_ << player.name() << " passes the Grand Entrance and collects "
                  << (kGrandEntranceBonus * laps) << " Reagents\n";
    }

    out_ << player.name() << " lands on " << board_.at(to).name() << '\n';
    board_.at(to).landOn(*this, player);
}

void Game::sendToLockup(Player &player) {
    player.sendToLockup();
    out_ << player.name() << " is sent to The Lockup\n";
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
            out_ << player.name() << " leaves The Lockup\n";
            return true;
        }
        if (answer == "token" && hasToken) {
            player.setFreePasses(player.freePasses() - 1);
            player.releaseFromLockup();
            out_ << player.name() << " spends a Free-Pass Token to leave The Lockup\n";
            return true;
        }
        out_ << "please answer roll";
        if (canPay) out_ << ", pay";
        if (hasToken) out_ << " or token";
        out_ << '\n';
    }
}

bool Game::forceLockupExit(Player &player) {
    if (player.freePasses() > 0) {
        player.setFreePasses(player.freePasses() - 1);
        player.releaseFromLockup();
        out_ << player.name() << " spends a Free-Pass Token to leave The Lockup\n";
        return true;
    }
    // payTreasury parks a debt when the fine is unaffordable, which is what
    // stops an Artificer who cannot pay from being detained forever: the debt
    // blocks the turn until they raise funds or declare insolvency.
    const int moveOncePaid = dice_.total();
    payTreasury(player, kLockupFine);
    if (owes(player)) {
        // The fine is owed but unpayable. Settling it later must still release
        // them and move this roll, so tag the debt with both.
        Debt &debt = debts_[&player];
        debt.fromLockupFine = true;
        debt.lockupMove = moveOncePaid;
        return false;
    }

    player.releaseFromLockup();
    return true;
}

bool Game::owes(const Player &player) const {
    return debts_.find(const_cast<Player *>(&player)) != debts_.end();
}

int Game::amountOwed(const Player &player) const {
    const auto found = debts_.find(const_cast<Player *>(&player));
    return found == debts_.end() ? 0 : found->second.amount;
}

void Game::addDebt(Player &debtor, Player *creditor, int amount) {
    Debt &debt = debts_[&debtor];
    if (debt.amount == 0) {
        debt.creditor = creditor;
    }
    // A second debt for the same Artificer can only arise from repeated
    // insolvency fees, which are always owed to the Treasury, because an
    // Artificer who owes anything is barred from rolling. Amounts therefore
    // accumulate and the original creditor stands.
    debt.amount += amount;
}

void Game::payTreasury(Player &player, int amount) {
    if (amount <= 0) return;
    if (player.reagents() >= amount) {
        player.addReagents(-amount);
        treasury_ += amount;
        out_ << player.name() << " pays " << amount << " Reagents to the Guild Treasury\n";
        return;
    }
    addDebt(player, nullptr, amount);
    out_ << player.name() << " owes " << amountOwed(player)
              << " Reagents to the Guild Treasury but holds only " << player.reagents()
              << "\nraise funds by mortgaging, or use the insolvent command\n";
}

void Game::payPlayer(Player &from, Player &to, int amount) {
    if (amount <= 0) return;
    if (from.reagents() >= amount) {
        from.addReagents(-amount);
        to.addReagents(amount);
        out_ << from.name() << " pays " << amount << " Reagents to " << to.name() << '\n';
        return;
    }
    addDebt(from, &to, amount);
    out_ << from.name() << " owes " << amountOwed(from) << " Reagents to " << to.name()
              << " but holds only " << from.reagents()
              << "\nraise funds by mortgaging, or use the insolvent command\n";
}

void Game::settleDebtIfPossible() {
    // Settling one debt can hand Reagents to a creditor who is themselves in
    // debt, so this repeats until no further debt can be covered. The entry is
    // erased before anything that could touch the map runs, because completing
    // a Lockup exit moves an Artificer and may create a fresh debt.
    while (true) {
        Player *ready = nullptr;
        for (const auto &entry : debts_) {
            if (entry.first->reagents() >= entry.second.amount) {
                ready = entry.first;
                break;
            }
        }
        if (ready == nullptr) return;

        Player &settled = *ready;
        const Debt debt = debts_[ready];
        debts_.erase(ready);

        settled.addReagents(-debt.amount);
        if (debt.creditor != nullptr) {
            debt.creditor->addReagents(debt.amount);
            out_ << settled.name() << " settles " << debt.amount << " Reagents with "
                      << debt.creditor->name() << '\n';
        } else {
            treasury_ += debt.amount;
            out_ << settled.name() << " settles " << debt.amount
                      << " Reagents with the Guild Treasury\n";
        }

        // A settled Lockup fine has bought the exit it was owed for.
        if (debt.fromLockupFine) {
            settled.releaseFromLockup();
            out_ << settled.name() << " leaves The Lockup\n";
            movePlayer(settled, debt.lockupMove);
        }
    }
}

int Game::mortgageTransferFee(const OwnableDistrict &district) {
    // Receiving a mortgaged district costs 10% of its printed value now; the
    // later unmortgage still costs the full 60%.
    return district.isMortgaged() ? district.buyPrice() / 10 : 0;
}

int Game::transferDistrict(OwnableDistrict &district, Player &to) {
    const int fee = mortgageTransferFee(district);
    district.setOwner(&to);
    return fee;
}

void Game::transferEverything(Player &from, Player &to) {
    to.addReagents(from.reagents());
    from.setReagents(0);
    to.setFreePasses(to.freePasses() + from.freePasses());
    from.setFreePasses(0);

    int fees = 0;
    for (OwnableDistrict *ownable : board_.ownables()) {
        if (ownable->owner() != &from) continue;
        fees += transferDistrict(*ownable, to);
    }
    out_ << to.name() << " receives everything " << from.name() << " held\n";

    if (fees > 0) {
        out_ << to.name() << " owes " << fees
                  << " Reagents on the mortgaged districts received\n";
        // Through payTreasury so an unaffordable total becomes a debt rather
        // than a negative balance.
        payTreasury(to, fees);
    }
}

bool Game::guildIsUnimproved(Guild guild) const {
    if (guild == Guild::None) return true;  // Forges and Depots take no upgrades
    for (const OwnableDistrict *ownable : board_.ownables()) {
        if (ownable->kind() != DistrictKind::Workshop || ownable->guild() != guild) continue;
        if (ownable->upgradeCount() > 0) return false;
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
    for (OwnableDistrict *ownable : board_.ownables()) {
        if (ownable->owner() != &from) continue;
        ownable->setOwner(nullptr);
        ownable->setMortgaged(false);  // they return to the open market unmortgaged
        released.push_back(ownable);
    }

    Auction auction{*this};
    for (OwnableDistrict *district : released) auction.run(*district);
}

void Game::declareInsolvency() {
    // Guarded on the current Artificer so a debt parked against someone else
    // cannot be declared away on their behalf.
    Player &debtor = currentPlayer();
    const auto owed = debts_.find(&debtor);
    if (owed == debts_.end()) {
        out_ << "you may only declare insolvency when you owe more than you hold\n";
        return;
    }
    if (debtor.reagents() >= owed->second.amount) {
        out_ << "you can cover this debt, so insolvency is not available\n";
        return;
    }

    out_ << debtor.name() << " declares insolvency\n";
    // Erasing the entry also discards any half-finished Lockup exit, which
    // dies with the Artificer who owed for it.
    Player *creditor = owed->second.creditor;
    debts_.erase(owed);
    debtor.declareInsolvent();

    if (creditor != nullptr) {
        transferEverything(debtor, *creditor);
    } else {
        releaseToMarket(debtor);
    }

    if (const Player *champion = winner()) {
        out_ << champion->name() << " is the last solvent Artificer and wins\n";
    }
}

bool Game::mortgage(const std::string &districtName) {
    Player &player = currentPlayer();
    OwnableDistrict *district = board_.findOwnable(districtName);
    if (district == nullptr) {
        out_ << "there is no ownable district called " << districtName << '\n';
        return false;
    }
    if (district->owner() != &player) {
        out_ << district->name() << " is not yours to mortgage\n";
        return false;
    }
    if (district->isMortgaged()) {
        out_ << district->name() << " is already mortgaged\n";
        return false;
    }
    if (district->upgradeCount() > 0) {
        out_ << "sell every upgrade on " << district->name() << " first\n";
        return false;
    }

    const int payout = district->buyPrice() / 2;
    district->setMortgaged(true);
    player.addReagents(payout);
    out_ << player.name() << " mortgages " << district->name() << " for " << payout
              << " Reagents\n";
    settleDebtIfPossible();
    return true;
}

bool Game::unmortgage(const std::string &districtName) {
    Player &player = currentPlayer();
    OwnableDistrict *district = board_.findOwnable(districtName);
    if (district == nullptr) {
        out_ << "there is no ownable district called " << districtName << '\n';
        return false;
    }
    if (district->owner() != &player) {
        out_ << district->name() << " is not yours\n";
        return false;
    }
    if (!district->isMortgaged()) {
        out_ << district->name() << " is not mortgaged\n";
        return false;
    }

    // Half the printed value plus a further 10%.
    const int cost = district->buyPrice() * 6 / 10;
    if (player.reagents() < cost) {
        out_ << "unmortgaging " << district->name() << " costs " << cost
                  << " Reagents\n";
        return false;
    }
    player.addReagents(-cost);
    treasury_ += cost;
    district->setMortgaged(false);
    out_ << player.name() << " unmortgages " << district->name() << " for " << cost
              << " Reagents\n";
    return true;
}

namespace {

// Shared lookup for the two upgrade commands: everything both must reject
// before the level itself is considered.
Workshop *findUpgradableWorkshop(Board &board, Player &player, const std::string &name,
                                 std::ostream &out) {
    OwnableDistrict *district = board.findOwnable(name);
    if (district == nullptr) {
        out << "there is no ownable district called " << name << '\n';
        return nullptr;
    }
    Workshop *workshop = board.findWorkshop(name);
    if (workshop == nullptr) {
        out << district->name() << " is not a Workshop, so it takes no upgrades\n";
        return nullptr;
    }
    if (workshop->owner() != &player) {
        out << workshop->name() << " is not yours\n";
        return nullptr;
    }
    if (workshop->isMortgaged()) {
        out << workshop->name() << " is mortgaged\n";
        return nullptr;
    }
    return workshop;
}

}  // namespace

bool Game::buyUpgrade(const std::string &districtName) {
    Player &player = currentPlayer();
    Workshop *workshop = findUpgradableWorkshop(board_, player, districtName, out_);
    if (workshop == nullptr) return false;

    if (!guildComplete(workshop->guild(), player)) {
        out_ << "you must own every Workshop in that Crafting Guild first\n";
        return false;
    }
    if (workshop->upgradeCount() >= kMaxUpgrades) {
        out_ << workshop->name() << " already has a Master Atelier\n";
        return false;
    }

    const int cost = workshop->upgradeCost();
    if (player.reagents() < cost) {
        out_ << "an upgrade on " << workshop->name() << " costs " << cost
                  << " Reagents\n";
        return false;
    }

    player.addReagents(-cost);
    treasury_ += cost;
    workshop->setUpgrades(workshop->upgradeCount() + 1);
    out_ << player.name() << " upgrades " << workshop->name() << " to level "
              << workshop->upgradeCount() << " for " << cost << " Reagents\n";
    return true;
}

bool Game::sellUpgrade(const std::string &districtName) {
    Player &player = currentPlayer();
    Workshop *workshop = findUpgradableWorkshop(board_, player, districtName, out_);
    if (workshop == nullptr) return false;

    if (workshop->upgradeCount() <= 0) {
        out_ << workshop->name() << " has no upgrades to sell\n";
        return false;
    }

    const int refund = workshop->upgradeCost() / 2;
    workshop->setUpgrades(workshop->upgradeCount() - 1);
    player.addReagents(refund);
    out_ << player.name() << " sells an upgrade on " << workshop->name() << " for "
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
        out_ << "a trade offer is already awaiting an answer\n";
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
        out_ << "there is no solvent Artificer called " << targetName << '\n';
        return false;
    }
    if (to == &from) {
        out_ << "you cannot trade with yourself\n";
        return false;
    }

    TradeOffer offer;
    offer.from = &from;
    offer.to = to;

    const bool giveIsReagents = parseReagents(give, offer.giveReagents);
    const bool receiveIsReagents = parseReagents(receive, offer.receiveReagents);
    if (giveIsReagents && receiveIsReagents) {
        out_ << "a Reagent-for-Reagent trade is not allowed\n";
        return false;
    }

    // Each side is validated against whoever is parting with it.
    if (giveIsReagents) {
        if (from.reagents() < offer.giveReagents) {
            out_ << from.name() << " does not hold " << offer.giveReagents
                      << " Reagents\n";
            return false;
        }
    } else {
        offer.giveDistrict = board_.findOwnable(give);
        if (offer.giveDistrict == nullptr) {
            out_ << "there is no ownable district called " << give << '\n';
            return false;
        }
        if (offer.giveDistrict->owner() != &from) {
            out_ << offer.giveDistrict->name() << " is not yours to trade\n";
            return false;
        }
        if (!guildIsUnimproved(offer.giveDistrict->guild())) {
            out_ << offer.giveDistrict->name()
                      << " cannot be traded while its Guild holds upgrades\n";
            return false;
        }
    }

    if (receiveIsReagents) {
        if (to->reagents() < offer.receiveReagents) {
            out_ << to->name() << " does not hold " << offer.receiveReagents
                      << " Reagents\n";
            return false;
        }
    } else {
        offer.receiveDistrict = board_.findOwnable(receive);
        if (offer.receiveDistrict == nullptr) {
            out_ << "there is no ownable district called " << receive << '\n';
            return false;
        }
        if (offer.receiveDistrict->owner() != to) {
            out_ << offer.receiveDistrict->name() << " does not belong to " << to->name()
                      << '\n';
            return false;
        }
        if (!guildIsUnimproved(offer.receiveDistrict->guild())) {
            out_ << offer.receiveDistrict->name()
                      << " cannot be traded while its Guild holds upgrades\n";
            return false;
        }
    }

    // Both sides must be able to cover the 10% due on any mortgaged district
    // they are about to receive. Refusing here is better than completing a
    // trade that drives someone negative: a trade is voluntary, so an
    // unaffordable one should simply not happen.
    const int feeOnGiven =
        offer.giveDistrict != nullptr ? mortgageTransferFee(*offer.giveDistrict) : 0;
    const int feeOnAsked =
        offer.receiveDistrict != nullptr ? mortgageTransferFee(*offer.receiveDistrict) : 0;

    const int fromAfter =
        from.reagents() - offer.giveReagents + offer.receiveReagents - feeOnAsked;
    const int toAfter =
        to->reagents() + offer.giveReagents - offer.receiveReagents - feeOnGiven;
    if (fromAfter < 0) {
        out_ << from.name() << " could not cover the " << feeOnAsked
                  << "-Reagent charge on the mortgaged district\n";
        return false;
    }
    if (toAfter < 0) {
        out_ << to->name() << " could not cover the " << feeOnGiven
                  << "-Reagent charge on the mortgaged district\n";
        return false;
    }

    offer.active = true;
    offer_ = offer;
    turn_->enterTradeResponse();
    out_ << from.name() << " offers " << sideText(offer_.giveDistrict, offer_.giveReagents)
              << " to " << to->name() << " for "
              << sideText(offer_.receiveDistrict, offer_.receiveReagents) << '\n'
              << to->name() << ": accept or reject\n";
    return true;
}

void Game::acceptTrade() {
    if (!offer_.active) {
        out_ << "there is no trade offer to answer\n";
        return;
    }
    const TradeOffer offer = offer_;
    offer_ = TradeOffer{};

    // offerTrade already proved both sides can cover any mortgage charge, so
    // these are charged directly rather than parked as debts.
    if (offer.giveDistrict != nullptr) {
        const int fee = transferDistrict(*offer.giveDistrict, *offer.to);
        if (fee > 0) {
            offer.to->addReagents(-fee);
            treasury_ += fee;
            out_ << offer.to->name() << " pays " << fee << " Reagents on the mortgaged "
                      << offer.giveDistrict->name() << '\n';
        }
    } else {
        offer.from->addReagents(-offer.giveReagents);
        offer.to->addReagents(offer.giveReagents);
    }

    if (offer.receiveDistrict != nullptr) {
        const int fee = transferDistrict(*offer.receiveDistrict, *offer.from);
        if (fee > 0) {
            offer.from->addReagents(-fee);
            treasury_ += fee;
            out_ << offer.from->name() << " pays " << fee << " Reagents on the mortgaged "
                      << offer.receiveDistrict->name() << '\n';
        }
    } else {
        offer.to->addReagents(-offer.receiveReagents);
        offer.from->addReagents(offer.receiveReagents);
    }

    out_ << offer.to->name() << " accepts the trade\n";
    turn_->resumeAfterTrade();
    // Trading is one of the ways the spec lets an Artificer raise funds.
    settleDebtIfPossible();
}

void Game::rejectTrade() {
    if (!offer_.active) {
        out_ << "there is no trade offer to answer\n";
        return;
    }
    out_ << offer_.to->name() << " rejects the trade\n";
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
    out_ << player.name() << " (" << player.token() << ") holds " << player.reagents()
              << " Reagents, " << player.freePasses() << " Free-Pass Tokens\n";
    for (const OwnableDistrict *ownable : board_.ownables()) {
        if (ownable->owner() != &player) continue;
        out_ << "  " << ownable->name();
        if (ownable->isMortgaged()) {
            out_ << " (mortgaged)";
        } else if (ownable->upgradeCount() > 0) {
            out_ << " (" << ownable->upgradeCount() << " upgrades)";
        }
        out_ << '\n';
    }
}

void Game::printAllAssets() const {
    for (const auto &player : players_) {
        if (player->isInsolvent()) {
            out_ << player->name() << " is insolvent\n";
            continue;
        }
        printAssets(*player);
    }
}

bool Game::guildComplete(Guild guild, const Player &owner) const {
    int inGuild = 0;
    int held = 0;
    for (const OwnableDistrict *ownable : board_.ownables()) {
        if (ownable->kind() != DistrictKind::Workshop || ownable->guild() != guild) continue;
        ++inGuild;
        if (ownable->owner() == &owner) ++held;
    }
    return inGuild > 0 && inGuild == held;
}

int Game::countOwned(DistrictKind kind, const Player &owner) const {
    int total = 0;
    for (const OwnableDistrict *ownable : board_.ownables()) {
        if (ownable->kind() == kind && ownable->owner() == &owner) ++total;
    }
    return total;
}

int Game::totalWorth(const Player &player) const {
    int worth = player.reagents();
    for (const OwnableDistrict *ownable : board_.ownables()) {
        if (ownable->owner() != &player) continue;
        worth += ownable->buyPrice() + ownable->upgradeInvestment();
    }
    return worth;
}

void Game::offerPurchase(OwnableDistrict &district, Player &buyer) {
    const int price = district.buyPrice();
    out_ << district.name() << " is unowned, " << price << " Reagents\n";

    if (buyer.reagents() >= price) {
        const std::string answer = prompt("buy it? [yes/no] ");
        if (answer == "yes" || answer == "y") {
            buyer.addReagents(-price);
            district.setOwner(&buyer);
            out_ << buyer.name() << " buys " << district.name() << " for " << price
                      << " Reagents\n";
            return;
        }
    } else {
        out_ << buyer.name() << " cannot afford " << district.name() << '\n';
    }

    Auction{*this}.run(district);
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
    out_ << player.name() << " receives a Free-Pass Token\n";
    return true;
}

std::string Game::prompt(const std::string &question) const {
    out_ << question;
    out_.flush();
    std::string line;
    std::getline(in_, line);
    return line;
}
