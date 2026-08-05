// Checks the layers gametest.cpp does not reach, built and run with `make test`.
//
// gametest drives the Model's rule methods directly. This harness goes in
// through the front door instead: it builds a GameController over string
// streams and feeds it command lines, so the parser, the Command objects, the
// turn state machine and the Model's mid-turn dialogues are all exercised the
// way a player would exercise them. It also validates the half of the save
// format that only fails on bad input, which is the part a demo cannot afford
// to get wrong.

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "board.h"
#include "boarddata.h"
#include "controller.h"
#include "district.h"
#include "game.h"
#include "player.h"
#include "savegame.h"
#include "workshop.h"

namespace {

int checks = 0;
int failures = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        ++checks;                                                      \
        if (!(cond)) {                                                 \
            std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; \
            ++failures;                                                \
        }                                                              \
    } while (0)

bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
}

// A whole application stack over strings: input is scripted, output is
// captured, and no console is involved anywhere.
struct Harness {
    std::istringstream in;
    std::ostringstream out;
    Game game;
    GameController controller;

    explicit Harness(const std::string &input = "", bool testing = true, unsigned seed = 1,
                     bool deck = false)
        : in{input}, game{testing, seed, deck, in, out}, controller{game} {}

    // Two Artificers and a turn begun: the state every test but the roster
    // ones starts from.
    void seat(int count = 2) {
        const char tokens[] = {'G', 'O', 'X', 'W', 'A', 'P'};
        const char *names[] = {"alice", "bob", "carol", "dave", "erin", "finn"};
        for (int i = 0; i < count; ++i) game.addPlayer(names[i], tokens[i]);
        game.beginTurn();
    }

    void run(const std::string &line) { controller.handleCommand(line); }
    std::string text() const { return out.str(); }
    void clear() { out.str(""); }

    Player &alice() { return game.player(0); }
    Player &bob() { return game.player(1); }
    OwnableDistrict &district(const std::string &name) {
        return *game.board().findOwnable(name);
    }
};

// ---------------------------------------------------------------------------
// The command layer: parsing, dispatch, and refusing to crash on nonsense.
// ---------------------------------------------------------------------------

void testCommandParsing() {
    Harness h;
    h.seat();

    // A misspelled verb is reported, not obeyed and not fatal. The spec calls
    // this out explicitly, so it is checked explicitly.
    h.run("frobnicate");
    CHECK(contains(h.text(), "I don't understand"));
    CHECK(h.game.turnStateName() == "may roll");

    // Blank input is ignored rather than reported.
    h.clear();
    h.run("");
    h.run("   \t ");
    CHECK(h.text().empty());

    // Commands whose argument is missing are as unrecognizable as a typo.
    for (const char *line : {"mortgage", "unmortgage", "save", "upgrade Tinder",
                             "upgrade", "trade bob", "trade bob Tinder"}) {
        h.clear();
        h.run(line);
        CHECK(contains(h.text(), "I don't understand"));
    }

    // "upgrade <district> <verb>" only takes buy or sell.
    h.clear();
    h.run("upgrade Tinder demolish");
    CHECK(contains(h.text(), "I don't understand"));

    // Dice arguments are validated rather than half-read: a bad one is
    // reported instead of silently becoming a random roll.
    for (const char *line : {"roll 1", "roll abc def", "roll 4x 2", "roll 2 4x", "roll -1 2"}) {
        h.clear();
        h.run(line);
        CHECK(contains(h.text(), "roll takes either no arguments or two"));
        CHECK(h.game.turnStateName() == "may roll");  // nothing was rolled
    }

    // Explicit dice outside -testing are refused, not obeyed and not ignored.
    Harness live{"", /*testing=*/false};
    live.seat();
    live.run("roll 3 4");
    CHECK(contains(live.text(), "explicit dice values need -testing"));
    CHECK(live.game.turnStateName() == "may roll");

    // The informational commands run without a receiver mishap.
    h.clear();
    h.run("assets");
    CHECK(!h.text().empty());
    h.clear();
    h.run("all");
    CHECK(!h.text().empty());
}

// ---------------------------------------------------------------------------
// The turn state machine.
// ---------------------------------------------------------------------------

void testTurnStates() {
    Harness h;
    h.seat();
    CHECK(h.game.turnStateName() == "may roll");

    // Rolling out of turn order, in both directions.
    h.clear();
    h.run("next");
    CHECK(contains(h.text(), "you still have a roll pending"));
    CHECK(h.game.currentPlayerIndex() == 0);

    h.run("roll 4 8");  // square 12, Market Festival: no dialogue to script
    CHECK(h.game.turnStateName() == "may end turn");
    CHECK(h.alice().position() == 12);

    h.clear();
    h.run("roll 4 8");
    CHECK(contains(h.text(), "you have already rolled this turn"));
    CHECK(h.alice().position() == 12);  // and it really did not move

    h.clear();
    h.run("next");
    CHECK(h.game.currentPlayerIndex() == 1);
    CHECK(h.game.turnStateName() == "may roll");

    // Doubles grant another roll rather than ending the turn.
    Harness d;
    d.seat();
    d.run("roll 6 6");
    CHECK(d.game.turnStateName() == "may roll");

    // Three consecutive doubles: detained, and the third total is not moved.
    Harness triple;
    triple.seat();
    triple.run("roll 2 2");
    triple.run("roll 3 3");
    CHECK(triple.alice().position() == 10);  // 0 -> 4 -> 10, just visiting
    triple.clear();
    triple.run("roll 1 1");
    CHECK(contains(triple.text(), "three doubles in a row"));
    CHECK(triple.alice().inLockup());
    CHECK(triple.alice().position() == kLockupSquare);
    CHECK(triple.game.turnStateName() == "may end turn");

    // The count is per turn, so it does not carry across turns.
    Harness reset;
    reset.seat();
    reset.run("roll 2 2");
    reset.run("roll 4 8");  // non-doubles ends the roll
    reset.run("next");
    reset.run("roll 2 2");
    reset.run("roll 3 3");
    reset.clear();
    reset.run("roll 1 1");  // bob's third of the turn, not the game
    CHECK(contains(reset.text(), "three doubles in a row"));
    CHECK(reset.bob().inLockup());
}

// ---------------------------------------------------------------------------
// The Lockup: three exits on every turn, and the compulsory third.
// ---------------------------------------------------------------------------

void testLockupExits() {
    // Paying the fine, then rolling and moving in the same turn.
    Harness pay{"pay\n"};
    pay.seat();
    pay.game.sendToLockup(pay.alice());
    pay.game.beginTurn();
    CHECK(pay.game.turnStateName() == "in The Lockup");
    pay.run("roll 2 8");  // 10 -> 20, Free Rest
    CHECK(!pay.alice().inLockup());
    CHECK(pay.alice().reagents() == kStartingReagents - kLockupFine);
    CHECK(pay.alice().position() == 20);

    // Spending a token instead.
    Harness token{"token\n"};
    token.seat();
    token.alice().setFreePasses(1);
    token.game.sendToLockup(token.alice());
    token.game.beginTurn();
    token.run("roll 2 8");
    CHECK(!token.alice().inLockup());
    CHECK(token.alice().freePasses() == 0);
    CHECK(token.alice().reagents() == kStartingReagents);
    CHECK(token.alice().position() == 20);

    // Declining both and rolling doubles.
    Harness lucky{"roll\n"};
    lucky.seat();
    lucky.game.sendToLockup(lucky.alice());
    lucky.game.beginTurn();
    lucky.clear();
    lucky.run("roll 5 5");
    CHECK(contains(lucky.text(), "leaves The Lockup"));
    CHECK(!lucky.alice().inLockup());
    CHECK(lucky.alice().reagents() == kStartingReagents);
    CHECK(lucky.alice().position() == 20);

    // Declining both and failing: a turn is served, not an exit bought.
    Harness stay{"roll\n"};
    stay.seat();
    stay.game.sendToLockup(stay.alice());
    stay.game.beginTurn();
    stay.clear();
    stay.run("roll 2 3");
    CHECK(contains(stay.text(), "stays in The Lockup"));
    CHECK(stay.alice().inLockup());
    CHECK(stay.alice().lockupTurns() == 1);
    CHECK(stay.alice().position() == kLockupSquare);

    // The third turn with neither route affordable: the fine is compulsory, so
    // it becomes a debt rather than detaining the Artificer forever.
    Harness broke;
    broke.seat();
    broke.alice().setReagents(10);
    broke.game.sendToLockup(broke.alice());
    broke.alice().serveLockupTurn();
    broke.alice().serveLockupTurn();
    broke.game.beginTurn();
    broke.run("roll 2 3");
    CHECK(broke.game.owes(broke.alice()));
    CHECK(broke.game.amountOwed(broke.alice()) == kLockupFine);
    CHECK(broke.alice().inLockup());
    CHECK(broke.alice().reagents() == 10);  // never driven negative

    // The turn cannot advance either way: the roll is already spent, and the
    // debt refuses to let the turn end.
    broke.clear();
    broke.run("roll 2 3");
    CHECK(contains(broke.text(), "you have already rolled"));
    broke.clear();
    broke.run("next");
    CHECK(contains(broke.text(), "settle the debt or declare insolvency"));
    CHECK(broke.game.currentPlayerIndex() == 0);

    // Raising the funds settles it, and the settlement still buys the exit it
    // was owed for.
    broke.district("Nexus").setOwner(&broke.alice());
    broke.clear();
    broke.run("mortgage Nexus");
    CHECK(!broke.game.owes(broke.alice()));
    CHECK(!broke.alice().inLockup());
    CHECK(contains(broke.text(), "leaves The Lockup"));

    // A debt carried into a fresh turn blocks the roll itself, which is what
    // stops one Artificer stacking up two separate obligations.
    Harness owing;
    owing.seat();
    owing.alice().setReagents(10);
    owing.game.payTreasury(owing.alice(), 500);
    owing.game.beginTurn();
    owing.clear();
    owing.run("roll 2 3");
    CHECK(contains(owing.text(), "settle your debt"));
    CHECK(owing.alice().position() == 0);
}

// ---------------------------------------------------------------------------
// Trading, including the phase a pending offer interrupts.
// ---------------------------------------------------------------------------

void testTrading() {
    Harness h;
    h.seat();
    h.district("Tinder").setOwner(&h.alice());
    h.run("roll 4 8");  // reach the post-roll phase first
    CHECK(h.game.turnStateName() == "may end turn");

    h.run("trade bob Tinder 100");
    CHECK(h.game.hasTradeOffer());
    CHECK(h.game.turnStateName() == "awaiting a trade response");

    // Neither action of the interrupted turn is available yet.
    h.clear();
    h.run("roll 1 1");
    CHECK(contains(h.text(), "must answer the trade offer first"));
    h.clear();
    h.run("next");
    CHECK(contains(h.text(), "must answer the trade offer first"));
    CHECK(h.game.currentPlayerIndex() == 0);

    h.run("accept");
    CHECK(!h.game.hasTradeOffer());
    CHECK(h.game.turnStateName() == "may end turn");  // the phase is restored
    CHECK(h.district("Tinder").owner() == &h.bob());
    CHECK(h.alice().reagents() == kStartingReagents + 100);
    CHECK(h.bob().reagents() == kStartingReagents - 100);

    // Rejecting restores the phase and moves nothing.
    Harness r;
    r.seat();
    r.district("Tinder").setOwner(&r.alice());
    r.run("trade bob Tinder 100");
    CHECK(r.game.turnStateName() == "awaiting a trade response");
    r.run("reject");
    CHECK(r.game.turnStateName() == "may roll");  // interrupted before rolling
    CHECK(r.district("Tinder").owner() == &r.alice());
    CHECK(r.bob().reagents() == kStartingReagents);

    // Answering when nothing was offered.
    r.clear();
    r.run("accept");
    CHECK(contains(r.text(), "no trade offer"));

    // The offers the rules refuse.
    Harness bad;
    bad.seat();
    bad.district("Tinder").setOwner(&bad.alice());
    CHECK(!bad.game.offerTrade("bob", "100", "200"));   // Reagents for Reagents
    CHECK(!bad.game.offerTrade("alice", "Tinder", "100"));
    CHECK(!bad.game.offerTrade("nobody", "Tinder", "100"));
    CHECK(!bad.game.offerTrade("bob", "Nowhere", "100"));
    CHECK(!bad.game.offerTrade("bob", "Flint", "100"));   // not alice's to give
    CHECK(!bad.game.offerTrade("bob", "Tinder", "Flint"));  // not bob's to give
    CHECK(!bad.game.offerTrade("bob", "999999", "Tinder"));  // alice lacks the funds
    CHECK(!bad.game.hasTradeOffer());

    // A district cannot move while its Guild holds upgrades.
    for (const char *name : {"Tinder", "Flint", "Cinder-stone", "Slagpit"}) {
        bad.district(name).setOwner(&bad.alice());
    }
    bad.game.board().findWorkshop("Tinder")->setUpgrades(1);
    CHECK(!bad.game.offerTrade("bob", "Flint", "100"));
    bad.game.board().findWorkshop("Tinder")->setUpgrades(0);
    CHECK(bad.game.offerTrade("bob", "Flint", "100"));

    // And a second offer cannot stack on the first.
    CHECK(!bad.game.offerTrade("bob", "Tinder", "100"));

    // Receiving a mortgaged district charges 10% of its printed value at once.
    Harness fee;
    fee.seat();
    fee.district("Nexus").setOwner(&fee.alice());
    fee.district("Nexus").setMortgaged(true);
    CHECK(fee.game.offerTrade("bob", "Nexus", "50"));
    fee.game.acceptTrade();
    CHECK(fee.district("Nexus").owner() == &fee.bob());
    CHECK(fee.bob().reagents() == kStartingReagents - 50 - 26);  // 10% of 260
    CHECK(fee.district("Nexus").isMortgaged());  // still mortgaged afterwards

    // A trade nobody can pay for is refused outright rather than completed
    // into a negative balance.
    Harness poor;
    poor.seat();
    poor.district("Nexus").setOwner(&poor.alice());
    poor.district("Nexus").setMortgaged(true);
    poor.bob().setReagents(5);
    CHECK(!poor.game.offerTrade("bob", "Nexus", "0"));
    CHECK(poor.bob().reagents() == 5);
}

// ---------------------------------------------------------------------------
// Upgrades, through the rules rather than around them.
// ---------------------------------------------------------------------------

void testUpgradeRules() {
    Harness h;
    h.seat();

    CHECK(!h.game.buyUpgrade("Nowhere"));   // no such district
    CHECK(!h.game.buyUpgrade("Ironhold"));  // a Forge takes no upgrades
    CHECK(!h.game.buyUpgrade("Eastgate"));  // nor a Depot
    CHECK(!h.game.buyUpgrade("Tinder"));    // not owned

    h.district("Tinder").setOwner(&h.alice());
    CHECK(!h.game.buyUpgrade("Tinder"));  // Guild not held outright

    for (const char *name : {"Flint", "Cinder-stone", "Slagpit"}) {
        h.district(name).setOwner(&h.alice());
    }

    const int treasuryBefore = h.game.treasury();
    CHECK(h.game.buyUpgrade("Tinder"));
    CHECK(h.game.board().findWorkshop("Tinder")->upgradeCount() == 1);
    CHECK(h.alice().reagents() == kStartingReagents - 50);
    CHECK(h.game.treasury() == treasuryBefore + 50);

    // Up to the Master Atelier, and no further.
    for (int i = 0; i < 4; ++i) CHECK(h.game.buyUpgrade("Tinder"));
    CHECK(h.game.board().findWorkshop("Tinder")->upgradeCount() == kMaxUpgrades);
    CHECK(!h.game.buyUpgrade("Tinder"));
    CHECK(h.alice().reagents() == kStartingReagents - 250);

    // Mortgaging is refused while upgrades stand.
    CHECK(!h.game.mortgage("Tinder"));

    // Selling returns half, one level at a time.
    CHECK(h.game.sellUpgrade("Tinder"));
    CHECK(h.game.board().findWorkshop("Tinder")->upgradeCount() == 4);
    CHECK(h.alice().reagents() == kStartingReagents - 250 + 25);
    for (int i = 0; i < 4; ++i) CHECK(h.game.sellUpgrade("Tinder"));
    CHECK(!h.game.sellUpgrade("Tinder"));  // nothing left to sell
    CHECK(h.alice().reagents() == kStartingReagents - 250 + 125);

    // A mortgaged Workshop cannot be upgraded.
    CHECK(h.game.mortgage("Tinder"));
    CHECK(!h.game.buyUpgrade("Tinder"));

    // An Artificer may not upgrade someone else's Workshop on their turn.
    h.game.setCurrentPlayer(1);
    CHECK(!h.game.buyUpgrade("Flint"));
}

// ---------------------------------------------------------------------------
// The auction, including the case the spec leaves open.
// ---------------------------------------------------------------------------

void testAuction() {
    // Declining the purchase opens the bidding; the last bidder standing pays
    // their own final bid.
    Harness win{"no\n500\nwithdraw\n"};
    win.seat();
    win.game.offerPurchase(win.district("Nexus"), win.alice());
    CHECK(win.district("Nexus").owner() == &win.alice());
    CHECK(win.alice().reagents() == kStartingReagents - 500);
    CHECK(win.bob().reagents() == kStartingReagents);

    // Buying outright never reaches the auction.
    Harness buy{"yes\n"};
    buy.seat();
    buy.game.offerPurchase(buy.district("Nexus"), buy.alice());
    CHECK(buy.district("Nexus").owner() == &buy.alice());
    CHECK(buy.alice().reagents() == kStartingReagents - 260);

    // Nobody bids: the district stays with the Guild. The spec does not say
    // what happens here, and this is the resolution the report defends.
    Harness none{"no\nwithdraw\nwithdraw\n"};
    none.seat();
    none.game.offerPurchase(none.district("Nexus"), none.alice());
    CHECK(none.district("Nexus").owner() == nullptr);
    CHECK(none.alice().reagents() == kStartingReagents);
    CHECK(contains(none.text(), "stays with the Guild"));

    // A bid that is not a number, or not a raise, or beyond the bidder's
    // holdings, is refused and the same Artificer answers again.
    Harness retry{"no\nabc\n50\n40\n99999\n120\nwithdraw\n"};
    retry.seat();
    retry.game.offerPurchase(retry.district("Nexus"), retry.alice());
    CHECK(contains(retry.text(), "type just the number"));
    CHECK(retry.district("Nexus").owner() == &retry.bob());
    CHECK(retry.bob().reagents() == kStartingReagents - 120);

    // An Artificer who cannot afford the asking price is not offered it.
    Harness broke{"withdraw\nwithdraw\n"};
    broke.seat();
    broke.alice().setReagents(10);
    broke.game.offerPurchase(broke.district("Nexus"), broke.alice());
    CHECK(contains(broke.text(), "cannot afford"));
    CHECK(broke.district("Nexus").owner() == nullptr);
}

// ---------------------------------------------------------------------------
// Insolvency, both destinations.
// ---------------------------------------------------------------------------

void testInsolvency() {
    // Refused when there is no debt, and when the debt is affordable.
    Harness none;
    none.seat();
    none.clear();
    none.game.declareInsolvency();
    CHECK(contains(none.text(), "only declare insolvency when you owe"));

    Harness rich;
    rich.seat();
    rich.alice().setReagents(10);
    rich.game.payTreasury(rich.alice(), 500);
    CHECK(rich.game.owes(rich.alice()));
    rich.alice().setReagents(600);
    rich.clear();
    rich.game.declareInsolvency();
    CHECK(contains(rich.text(), "you can cover this debt"));
    CHECK(!rich.alice().isInsolvent());

    // To another Artificer: the creditor inherits everything.
    Harness toPlayer;
    toPlayer.seat();
    toPlayer.game.setCurrentPlayer(1);
    toPlayer.bob().setReagents(10);
    toPlayer.bob().setFreePasses(1);
    toPlayer.district("Nexus").setOwner(&toPlayer.bob());
    toPlayer.game.payPlayer(toPlayer.bob(), toPlayer.alice(), 500);
    CHECK(toPlayer.game.owes(toPlayer.bob()));
    toPlayer.game.declareInsolvency();
    CHECK(toPlayer.bob().isInsolvent());
    CHECK(toPlayer.district("Nexus").owner() == &toPlayer.alice());
    CHECK(toPlayer.alice().freePasses() == 1);
    CHECK(toPlayer.alice().reagents() == kStartingReagents + 10);
    CHECK(toPlayer.bob().reagents() == 0);
    CHECK(toPlayer.game.winner() == &toPlayer.alice());
    CHECK(toPlayer.game.isOver());

    // To the Treasury: tokens are destroyed and the districts are auctioned
    // unmortgaged. Three Artificers, so an auction can actually be held.
    Harness toGuild;
    toGuild.seat(3);
    toGuild.game.setCurrentPlayer(1);
    toGuild.bob().setReagents(10);
    toGuild.bob().setFreePasses(2);
    toGuild.district("Nexus").setOwner(&toGuild.bob());
    toGuild.district("Nexus").setMortgaged(true);
    toGuild.game.payTreasury(toGuild.bob(), 500);
    const int treasuryBefore = toGuild.game.treasury();
    toGuild.game.declareInsolvency();
    CHECK(toGuild.bob().isInsolvent());
    CHECK(toGuild.bob().freePasses() == 0);            // destroyed, not inherited
    CHECK(toGuild.game.totalFreePasses() == 0);
    CHECK(toGuild.game.treasury() == treasuryBefore + 10);
    CHECK(toGuild.district("Nexus").owner() == nullptr);
    CHECK(!toGuild.district("Nexus").isMortgaged());   // returns to the market clear
    CHECK(toGuild.game.solventCount() == 2);
    CHECK(!toGuild.game.isOver());

    // The save file has no insolvency field, so eliminated Artificers are
    // simply not written.
    const std::string file = "systemtest_solvent.txt";
    CHECK(SaveGame::save(toGuild.game, file));
    std::ifstream saved{file};
    std::ostringstream body;
    body << saved.rdbuf();
    saved.close();
    CHECK(body.str().compare(0, 1, "2") == 0);
    CHECK(!contains(body.str(), "bob "));
    std::remove(file.c_str());
}

// ---------------------------------------------------------------------------
// The squares that do something on arrival.
// ---------------------------------------------------------------------------

void testCardAndSpecialSquares() {
    // Go to Lockup detains without collecting the Grand Entrance bonus.
    Harness jail;
    jail.seat();
    jail.game.movePlayer(jail.alice(), kGoToLockupSquare);
    CHECK(jail.alice().inLockup());
    CHECK(jail.alice().position() == kLockupSquare);
    CHECK(jail.alice().reagents() == kStartingReagents);

    // Landing on The Lockup itself is only a visit.
    Harness visit;
    visit.seat();
    visit.game.movePlayer(visit.alice(), kLockupSquare);
    CHECK(!visit.alice().inLockup());

    // Free Rest costs nothing.
    Harness rest;
    rest.seat();
    rest.game.movePlayer(rest.alice(), kFreeRestSquare);
    CHECK(rest.alice().reagents() == kStartingReagents);

    // A Transit Toll is a flat fee.
    Harness toll;
    toll.seat();
    toll.game.movePlayer(toll.alice(), 23);
    CHECK(toll.alice().reagents() == kStartingReagents - kTransitTollFee);

    // The Guild Levy is the Artificer's choice of a flat fee or a share.
    Harness flat{"flat\n"};
    flat.seat();
    flat.game.movePlayer(flat.alice(), 3);
    CHECK(flat.alice().reagents() == kStartingReagents - kGuildLevyFlat);

    Harness share{"percent\n"};
    share.seat();
    share.game.movePlayer(share.alice(), 3);
    CHECK(share.alice().reagents() == kStartingReagents - kStartingReagents / 10);

    Harness confused{"maybe\nflat\n"};
    confused.seat();
    confused.game.movePlayer(confused.alice(), 3);
    CHECK(contains(confused.text(), "please answer flat or percent"));
    CHECK(confused.alice().reagents() == kStartingReagents - kGuildLevyFlat);

    // An Enchantment card either grants a token or moves Reagents by one of
    // the seven table values.
    Harness card;
    card.seat();
    card.game.movePlayer(card.alice(), 7);
    const int change = card.alice().reagents() - kStartingReagents;
    const bool legal = change == -200 || change == -100 || change == -50 || change == 25 ||
                       change == 50 || change == 100 || change == 200;
    CHECK(legal || card.alice().freePasses() == 1);

    // A Wandering Market resolves its destination rather than stranding
    // anyone: the Artificer ends up somewhere legal, having been told where.
    Harness market;
    market.seat();
    market.game.movePlayer(market.alice(), 5);
    CHECK(contains(market.text(), "a Wandering Market sends"));
    CHECK(market.alice().position() >= 0 && market.alice().position() < kBoardSize);

    // The token cap holds at the seam both card squares share.
    Harness capped;
    capped.seat();
    capped.alice().setFreePasses(4);
    CHECK(capped.game.totalFreePasses() == kMaxFreePassesInPlay);
    CHECK(!capped.game.tryGrantFreePass(capped.bob()));
}

// ---------------------------------------------------------------------------
// Load validation: the half of the save format that only fails on bad input.
// ---------------------------------------------------------------------------

std::vector<std::string> ownableLines() {
    const Board board;
    std::vector<std::string> lines;
    for (const OwnableDistrict *ownable : board.ownables()) {
        lines.push_back(ownable->name() + " GUILD 0");
    }
    return lines;
}

std::string join(const std::vector<std::string> &lines) {
    std::string text;
    for (const std::string &line : lines) text += line + "\n";
    return text;
}

std::string saveText(const std::string &players, std::size_t count,
                     const std::vector<std::string> &districts) {
    return std::to_string(count) + "\n\n" + players + "\n" + join(districts);
}

const char *const kTwoPlayers = "alice G 0 1500 0\nbob O 0 1500 5\n";

void writeFile(const std::string &name, const std::string &content) {
    std::ofstream out{name};
    out << content;
}

// A rejected load must report why and leave the game exactly as it was.
void expectRejected(const std::string &label, const std::string &content) {
    ++checks;
    const std::string file = "systemtest_load.txt";
    writeFile(file, content);

    Harness h;
    h.seat();  // a game already in progress, which must survive the attempt
    h.district("Tinder").setOwner(&h.alice());
    h.alice().setReagents(1234);

    std::string error;
    const bool loaded = SaveGame::load(h.game, file, error);
    std::remove(file.c_str());

    const bool untouched = h.game.playerCount() == 2 && h.alice().reagents() == 1234 &&
                           h.district("Tinder").owner() == &h.alice();
    if (loaded || error.empty() || !untouched) {
        ++failures;
        std::cout << "FAIL: load should reject " << label;
        if (loaded) std::cout << " (it was accepted)";
        if (!loaded && error.empty()) std::cout << " (no reason given)";
        if (!untouched) std::cout << " (the game was modified)";
        std::cout << '\n';
    }
}

void testLoadRejectsBadFiles() {
    const std::vector<std::string> districts = ownableLines();

    expectRejected("a file with no content at all", "");
    expectRejected("an empty file", "\n\n\n");
    expectRejected("a non-numeric Artificer count", "many\n\n" + std::string{kTwoPlayers});
    expectRejected("a single Artificer", saveText("alice G 0 1500 0\n", 1, districts));
    expectRejected("seven Artificers",
                   saveText("a G 0 1500 0\nb O 0 1500 0\nc X 0 1500 0\nd W 0 1500 0\n"
                            "e A 0 1500 0\nf P 0 1500 0\ng G 0 1500 0\n",
                            7, districts));
    expectRejected("a file that ends before all Artificers",
                   "2\n\nalice G 0 1500 0\n\n" + join(districts));
    expectRejected("a malformed Artificer line", saveText("alice\nbob O 0 1500 5\n", 2, districts));
    expectRejected("an unknown token", saveText("alice Z 0 1500 0\nbob O 0 1500 5\n", 2, districts));
    expectRejected("a position past the board",
                   saveText("alice G 0 1500 40\nbob O 0 1500 5\n", 2, districts));
    expectRejected("a negative position",
                   saveText("alice G 0 1500 -1\nbob O 0 1500 5\n", 2, districts));
    expectRejected("an Artificer starting on Go to Lockup",
                   saveText("alice G 0 1500 30\nbob O 0 1500 5\n", 2, districts));
    expectRejected("a negative Free-Pass count",
                   saveText("alice G -1 1500 0\nbob O 0 1500 5\n", 2, districts));
    expectRejected("five Free-Pass Tokens in play",
                   saveText("alice G 3 1500 0\nbob O 2 1500 5\n", 2, districts));
    expectRejected("two Artificers sharing a token",
                   saveText("alice G 0 1500 0\nbob G 0 1500 5\n", 2, districts));
    expectRejected("two Artificers sharing a name",
                   saveText("alice G 0 1500 0\nalice O 0 1500 5\n", 2, districts));
    expectRejected("a detained Artificer with no turn count",
                   saveText("alice G 0 1500 0\nbob O 0 1500 10 1\n", 2, districts));
    expectRejected("a detained Artificer past the third turn",
                   saveText("alice G 0 1500 0\nbob O 0 1500 10 1 3\n", 2, districts));

    std::vector<std::string> shuffled = districts;
    std::swap(shuffled[0], shuffled[1]);
    expectRejected("districts out of circuit order", saveText(kTwoPlayers, 2, shuffled));

    std::vector<std::string> unknownOwner = districts;
    unknownOwner[0] = "Tinder mallory 0";
    expectRejected("a district owned by an unknown Artificer",
                   saveText(kTwoPlayers, 2, unknownOwner));

    std::vector<std::string> guildMortgaged = districts;
    guildMortgaged[0] = "Tinder GUILD -1";
    expectRejected("an unowned district that is mortgaged",
                   saveText(kTwoPlayers, 2, guildMortgaged));

    std::vector<std::string> guildUpgraded = districts;
    guildUpgraded[0] = "Tinder GUILD 3";
    expectRejected("an unowned district that is upgraded",
                   saveText(kTwoPlayers, 2, guildUpgraded));

    std::vector<std::string> tooManyUpgrades = districts;
    tooManyUpgrades[0] = "Tinder alice 6";
    expectRejected("a Workshop above the Master Atelier",
                   saveText(kTwoPlayers, 2, tooManyUpgrades));

    std::vector<std::string> belowMortgage = districts;
    belowMortgage[0] = "Tinder alice -2";
    expectRejected("an upgrade value below the mortgage marker",
                   saveText(kTwoPlayers, 2, belowMortgage));

    std::vector<std::string> upgradedForge = districts;
    upgradedForge[2] = "Ironhold alice 1";  // index 2 is the first Forge
    expectRejected("an upgraded Forge", saveText(kTwoPlayers, 2, upgradedForge));

    std::vector<std::string> malformed = districts;
    malformed[0] = "Tinder";
    expectRejected("a malformed district line", saveText(kTwoPlayers, 2, malformed));

    std::vector<std::string> truncated = districts;
    truncated.pop_back();
    expectRejected("a file that ends before all districts",
                   saveText(kTwoPlayers, 2, truncated));

    // A file that is simply not there.
    ++checks;
    Harness missing;
    std::string error;
    if (SaveGame::load(missing.game, "systemtest_no_such_file.txt", error) || error.empty()) {
        ++failures;
        std::cout << "FAIL: load should reject a missing file\n";
    }
}

void testLoadAcceptsValidFiles() {
    std::vector<std::string> districts = ownableLines();
    districts[0] = "Tinder alice -1";  // mortgaged
    districts[1] = "Flint alice 5";    // a Master Atelier
    districts[2] = "Ironhold bob 0";

    const std::string file = "systemtest_good.txt";
    writeFile(file, saveText("alice G 3 900 10 1 2\nbob O 1 700 10 0\n", 2, districts));

    Harness h;
    std::string error;
    CHECK(SaveGame::load(h.game, file, error));
    CHECK(error.empty());
    CHECK(h.game.playerCount() == 2);

    // A detained Artificer and a visiting one share square 10 and are told
    // apart only by the trailing fields.
    CHECK(h.game.player(0).inLockup());
    CHECK(h.game.player(0).lockupTurns() == 2);
    CHECK(!h.game.player(1).inLockup());
    CHECK(h.game.player(1).position() == kLockupSquare);

    // Exactly four tokens in play is the limit, not past it.
    CHECK(h.game.totalFreePasses() == kMaxFreePassesInPlay);

    // -1 restores a mortgage with no upgrades under it.
    CHECK(h.game.board().findOwnable("Tinder")->isMortgaged());
    CHECK(h.game.board().findWorkshop("Tinder")->upgradeCount() == 0);
    CHECK(h.game.board().findWorkshop("Flint")->upgradeCount() == kMaxUpgrades);
    CHECK(h.game.board().findOwnable("Ironhold")->owner() == &h.game.player(1));

    // "Play resumes with the first-listed Artificer."
    CHECK(h.game.currentPlayerIndex() == 0);

    // main begins the turn once the roster is settled, by either route. A
    // detained Artificer must resume in The Lockup rather than with a free
    // roll that would walk them out of it.
    h.game.beginTurn();
    CHECK(h.game.turnStateName() == "in The Lockup");

    // What was loaded is what is written back out.
    const std::string echoed = "systemtest_good_echo.txt";
    CHECK(SaveGame::save(h.game, echoed));
    std::ifstream first{file};
    std::ifstream second{echoed};
    std::ostringstream a;
    std::ostringstream b;
    a << first.rdbuf();
    b << second.rdbuf();
    CHECK(a.str() == b.str());

    first.close();
    second.close();
    std::remove(file.c_str());
    std::remove(echoed.c_str());
}

}  // namespace

int main() {
    testCommandParsing();
    testTurnStates();
    testLockupExits();
    testTrading();
    testUpgradeRules();
    testAuction();
    testInsolvency();
    testCardAndSpecialSquares();
    testLoadRejectsBadFiles();
    testLoadAcceptsValidFiles();

    if (failures == 0) {
        std::cout << "systemtest: all " << checks << " checks passed\n";
        return 0;
    }
    std::cout << "systemtest: " << failures << " of " << checks << " checks failed\n";
    return 1;
}
