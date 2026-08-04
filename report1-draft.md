# CS247 Project — Phase 1 Report (draft)

> **Draft for review.** Fill the title page, then read §6 line by line and correct anything
> that does not match your own account of how the work went — it is your integrity
> statement, not mine. Export the diagrams from `diagrams/*.png` into the PDF.

---

## 1. Title Page

- **Name:** _[your name]_
- **Student ID:** _[your ID]_
- **UW userid:** _[your userid]_
- **Group name:** _[group name]_
- **Game implemented:** ArcaneMarket
- **Course:** CS247 — Software Engineering Principles

---

## 2. UML Class Diagram

Source in `uml.md`; rendered images in `diagrams/`.

| Image file | Figure | Subject |
|---|---|---|
| `uml-1.png` | **1A** | **Class diagram, Part A** — application and control |
| `uml-2.png` | **1B** | **Class diagram, Part B** — domain model |
| `uml-3.png` | 2 | MVC + Observer (supplementary) |
| `uml-4.png` | 3 | Template Method (supplementary) |
| `uml-5.png` | 4 | State (supplementary) |
| `uml-6.png` | 5 | Turn phase transitions (state diagram) |
| `uml-7.png` | 6 | Command (supplementary) |
| `uml-8.png` | 7 | Simple Factory (supplementary) |

**Figures 1A and 1B together are the class diagram.** It is too large for one page, so it
is split into two clearly labelled parts. Every major class appears in exactly one part
with its key attributes and its full public interface. Where a relationship crosses the
split, the far class appears as a name-only box marked with the part that details it, so
no relationship is lost at the seam.

- **Part A — application and control:** the Observer infrastructure, `TextView`,
  `ControllerInterface` / `GameController`, `CommandParser`, `Command` and all thirteen
  concrete commands, and `Turn` with its four states.
- **Part B — domain model:** `Game` and its economy (`Debt`, `TradeOffer`, the Treasury),
  `Player`, `Board`, the full `District` hierarchy, `Dice`, `CardDeck`, `DistrictFactory`,
  `DistrictSpec`, and the `SaveGame` / `Setup` namespaces.

**Figures 2–7 are supplementary per-pattern views**, referenced from §3. They repeat
classes already present in 1A/1B, reduced to the participants in a single pattern so its
shape can be seen without tracing it out of the full diagram.

**Notation** follows Chapter 8: `+ - #` for visibility, *italics* for pure virtual,
underline for static, `const` marking query operations, hollow triangle for inheritance,
filled diamond for composition, hollow diamond for aggregation, dashed open arrow for
dependency.

**Relationship kinds** were chosen using the chapter's decision procedure rather than by
eye:

- **Composition** (filled diamond) wherever the whole holds the part by value or by
  `unique_ptr` and destroys it: `Game ◆— Board`, `◆— Player [2..6]`, `◆— Turn`, `◆— Dice`,
  `◆— CardDeck [0..1]`, `Board ◆— District [40]`, `Turn ◆— TurnState [4]`.
- **Aggregation** (hollow diamond) for the one `shared_ptr`: `TextView ◇— ControllerInterface`.
  The Controller does not die with the View, which is what separates this from composition.
- **Association** (plain line) for stored non-owning pointers: `Subject —▶ Observer`, and
  `OwnableDistrict —▶ Player` with role name `owner` and multiplicity `0..1`, since an
  unowned district holds `nullptr`.
- **Dependency** (dashed) for create/use relationships that are not stored:
  `Board ⋯▶ DistrictFactory`, `DistrictFactory ⋯▶ District`, `CommandParser ⋯▶ Command`,
  `SaveGame ⋯▶ Game`.

**Multiplicities** appear on both ends of every association. The most significant are
`1 Board — 40 District`, `1 Game — 2..6 Player`, and `1 Turn — 4 TurnState`. Crafting
Guild membership is *not* a multiplicity here: `Guild` is an `enum class` value carried by
each `Workshop`, not a class, so guild sizes (Ember 4, Smoke 3, Arcane 4, Gilded 5) are
enforced as a rule in `Game::guildComplete`, which counts actual membership rather than
assuming a fixed group size.

Following Chapter 8, method **parameters are not associations**: `landOn(Game&, Player&)`
passes references at call time, which the chapter explicitly excludes from association.
Only stored references and pointers are drawn as lines.

---

## 3. Design Patterns Used

Six patterns are applied. Each is given as pattern name, where it is used, why it was chosen
(including the alternatives rejected), and how it is implemented. Patterns that were considered
and deliberately **not** used are listed at the end, since rejecting one is also a design
decision.

---

### 3.1 Model-View-Controller

**Where it is used.** `Game` is the Model (`game.h` / `game.cpp`). `TextView` is the View
(`textview.h` / `textview.cpp`). `ControllerInterface` and its concrete `GameController` are the
Controller (`controller.h` / `controller.cpp`). See Figure 2.

**Why it was chosen.** The spec requires the board to be redrawn after every command, requires a
text display now, and lists a graphical display as an optional enhancement. The rules of the game
therefore have to be expressed independently of how they are shown.

*Alternatives considered and rejected:*

- **One class holding both rules and printing.** Rejected because the rules could then only be
  tested by capturing standard output, and adding the graphical display would mean editing the
  rules themselves.
- **Model and View with no Controller, the View interpreting input directly.** Rejected because
  command parsing and legality checking would live inside the display class, so changing the
  interpreter would mean touching rendering code.
- **Controller mediating all data, with the View asking the Controller for state.** Rejected on
  Chapter 17's own advice, which says routing all data through the Controller "is not MVC" and
  creates unnecessary coupling; the View can query the Model directly.

**How it is implemented.** `Game` holds all state and extends `Subject`, and names no View.
`TextView` holds a `Game&` and a `shared_ptr<ControllerInterface>`, registers itself as an
Observer of the Model, and on notification re-queries the Model and redraws. User input arrives
at `TextView::handleInput`, which forwards it to the Controller; `GameController` mutates the
Model and then fires a single notification for the whole command.

---

### 3.2 Observer

**Where it is used.** `Subject` and `Observer` in `observer.h`; `Game` inherits `Subject`;
`TextView` inherits `Observer`. See Figure 2.

**Why it was chosen.** The Model must be able to trigger a redraw without knowing what displays
exist, so that the graphical-display enhancement could be added without touching `Game`.

*Alternatives considered and rejected:*

- **`Game` holding a `TextView*` and calling `render()` directly.** Rejected because it couples
  the Model to one concrete View; a second simultaneous view would require editing `Game`.
- **The main loop calling `view.render()` after each command (polling).** Rejected because it
  moves the "something changed" decision out of the Model, and a view attached later in the
  program's life would not be driven at all.

**How it is implemented.** `Subject` keeps a `vector<Observer*>` of non-owning pointers, with
`addObserver` (duplicate-checked with `std::find`), `removeObserver` (erase-remove) and
`notifyObservers`. `TextView` registers in its constructor and deregisters in its destructor.
Notification is fired once per command from `GameController::handleCommand` rather than from
every Model mutator, because the spec redraws the whole board after each command, so the command
— not the individual field write — is the real unit of change.

---

### 3.3 Template Method

**Where it is used.** `OwnableDistrict::landOn` (declared `final`) with the pure virtual hook
`OwnableDistrict::computeToll`; overridden by `Workshop`, `Forge` and `Depot`. See Figure 3.

**Why it was chosen.** Every visit to an ownable district follows the same fixed sequence —
unowned, so offer it; owned by the visitor, so nothing; mortgaged, so nothing; otherwise charge —
but the amount charged is computed three entirely different ways.

*Alternatives considered and rejected:*

- **A single `landOn` with a `switch` on `DistrictKind`.** Rejected because adding a district
  kind would mean editing that function, and the invariant sequence would sit interleaved with
  three unrelated pricing rules.
- **Strategy, injecting a `TollPolicy` object into each district.** Rejected because the pricing
  rule is fixed by the district's type and never changes at runtime; a swappable policy object
  would add an allocation and a level of indirection to buy flexibility the game never uses.

**How it is implemented.** `landOn` is `final`, so no subclass can reorder or skip a step of the
sequence, and `computeToll` is a protected pure virtual — the single point of variation.
`Workshop` indexes a six-entry toll table by upgrade level and doubles an unimproved toll when
the owner holds the whole Crafting Guild; `Forge` indexes a levy table by how many Forges the
owner controls; `Depot` multiplies the dice total by four, or ten when one Artificer holds both.

---

### 3.4 State

**Where it is used.** `Turn` is the Context (`turn.h` / `turn.cpp`); `TurnState` is the state
interface, with `PreRollState`, `PostRollState`, `LockupState` and `AwaitingTradeResponseState`
(`turnstate.h` / `turnstate.cpp`). See Figures 4 and 5.

**Why it was chosen.** Which commands are legal depends on the phase of the turn: `roll` only
when a roll is pending, `next` only when it is not, rolling in The Lockup runs an exit procedure
instead of moving, and while a trade offer is outstanding only `accept` and `reject` apply.

*Alternatives considered and rejected:*

- **An `enum TurnPhase` with `if`/`else` chains in each command.** Rejected on Chapter 16's
  argument: every new phase requires editing every command, and the phase logic ends up scattered
  across the code rather than localised.
- **A set of booleans (`hasRolled`, `isDetained`, `awaitingTrade`).** Rejected because they admit
  contradictory combinations that no rule permits, and nothing enforces which transitions are
  legal.

**How it is implemented.** `Turn` owns all four states as `unique_ptr` members constructed once,
and selects the active one through a raw non-owning `state_` pointer, so a transition allocates
nothing. Each concrete state holds a `Turn&` and drives its own transitions by calling
`setState`. `Turn` additionally records `resume_`, the phase a trade offer interrupted, because
an offer may be made before *or* after rolling and the Context cannot assume a fixed state to
return to.

---

### 3.5 Command

**Where it is used.** The `Command` interface and thirteen concrete commands (`RollCommand`,
`NextCommand`, `MortgageCommand`, `UnmortgageCommand`, `UpgradeCommand`, `TradeCommand`,
`AcceptCommand`, `RejectCommand`, `InsolventCommand`, `AssetsCommand`, `AllCommand`,
`SaveCommand`, `MessageCommand`) in `command.h` / `command.cpp`; `CommandParser` in
`commandparser.*`; `GameController` acts as the Invoker and `Game` as the Receiver. See Figures
1A and 6.

**Why it was chosen.** The spec defines eleven commands with different arities — from bare `next`
to `trade <name> <give> <receive>` — and requires that misspelled commands be handled gracefully
rather than crashing.

*Alternatives considered and rejected:*

- **An `if`/`else` chain on the verb inside the controller.** Rejected because it mixes parsing
  with execution and grows without bound; every new command would mean editing the controller.
- **A `map<string, function<void(istream&)>>` of handlers.** Rejected because arguments would
  then either be re-parsed inside each handler or captured untyped in a lambda. Binding them as
  typed data members at construction is what Chapter 20 prescribes and is what makes each request
  a self-contained object.

**How it is implemented.** Each concrete command stores its receiver (`Game&`) and its parsed
arguments as data members, so it is complete before it is executed. `CommandParser::parse` builds
the right command from an input line and returns `nullptr` for anything it does not recognise,
which is the graceful-failure path the spec requires. `GameController` fires `execute()` through
the interface without knowing which command it holds. `undo()` is deliberately absent: Chapter 20
adds it only "when undo is required", and ArcaneMarket has no undo.

---

### 3.6 Simple Factory

**Where it is used.** `DistrictFactory::create` (`districtfactory.*`), called by `Board`'s
constructor; the same idiom appears in `CommandParser::parse`. See Figure 7.

**Why it was chosen.** `Board` must build 40 districts of twelve different kinds from the static
board table without the names of concrete district classes spreading through the code.

*Alternatives considered and rejected:*

- **A `switch` inside `Board`'s constructor.** Rejected because it puts every concrete district
  class name into `Board` and mixes object construction with circuit management.
- **GoF Factory Method with an abstract creator hierarchy.** Rejected deliberately. Chapter 12
  states that Simple Factory suits "a small, fixed set of product types that you do not expect to
  grow" — which the 40-square circuit is — and warns against reaching for factories when a
  constructor would do. Building a creator hierarchy purely so the stronger pattern could be
  claimed would be over-engineering.

**How it is implemented.** A single static `create` takes a `DistrictSpec` from the board table,
switches on its `kind`, and returns `unique_ptr<District>`. It is the one place in the program
where concrete district class names appear. It is named the Simple Factory *idiom* rather than
Factory Method because there is no creator hierarchy, which is precisely the structural
difference between the two.

---

### 3.7 Patterns considered and deliberately not used

- **Decorator** for Workshop upgrades. Rejected; the reasoning is Design Question 3 below.
- **Iterator** for traversing the circuit. The board is a `vector<unique_ptr<District>>` indexed
  directly by square number, and movement is arithmetic modulo 40. A hand-written iterator would
  add notation without removing any indirection.
- **Strategy** as a separate claim. Chapter 17 describes the Controller as the View's Strategy and
  `TextView` does hold a `shared_ptr<ControllerInterface>`, but only one concrete controller
  exists, so nothing demonstrates swappability. It is reported as part of MVC's compound
  structure rather than claimed in its own right.

## 4. Design Questions

### Design Question 1 — Would the Observer Pattern be a good fit for the Circuit Board?

**Yes, but the granularity matters more than the yes.**

The real payoff is that `Game` holds no reference to any view. It calls `notifyObservers()` and
is done. That is what would make the graphical-display bonus additive rather than invasive: a
second view attaches to the same Model with no Model change, and both redraw from one
notification.

The caveat worth stating is that the spec redraws the **entire** board after every command. So
per-district Subjects would be pure overhead — a change to one district triggers a full redraw
regardless, and there is no partial repaint to optimize. The right shape is one Subject (`Game`)
with the whole `TextView` as a single Observer. Chapter 17's worked example puts a Composite
inside the Model so notifications climb leaf-to-root; that machinery buys nothing when every
update repaints everything, so it was deliberately not copied.

**A consequence actually encountered, not hypothesised:** because every mutator notified, six
`addPlayer` calls during setup printed six boards. Observer was behaving correctly; the fix
belonged in *when* notification fires, which is now the command boundary (§3.1). The lesson is
that Observer decides *who* learns about a change, but the designer still has to decide what
counts as one change.

### Design Question 2 — Modelling the decks as a real shuffled deck: which pattern, and how?

**What the spec requires** is a probability distribution (Tables 2 and 3), not a deck. That is
what `cards.h` implements: integer weights — twenty-fourths for Table 2, eighteenths for Table 3
— drawn with a single uniform ticket. Seven classes wrapping seven integer deltas would have
been pattern-padding.

**A real deck is a genuinely different mechanic, not a reskin.** A distribution is *memoryless*:
−200 can come up twice in a row and the odds never move. A deck is *stateful*: each card leaves
the deck when drawn, so every later draw has different odds, and counting cards becomes a
legitimate strategy. Over one full pass a deck deals Table 3 **exactly**, which a distribution
only approaches in the limit.

**It is implemented** as the optional `CardDeck` (`-deck` at startup): eighteen cards, one per
weight unit, dealt from the front and reshuffled on exhaustion. Both implementations meet behind
a single seam, `Game::drawEnchantmentValue()`, so the `EnchantmentDeck` square is identical
either way and the choice is made at runtime rather than compile time — which is what the spec
requires of an enhancement.

**What building it changed about the answer.** The plan had assumed each card would need to be a
Command object. It did not. The seam turned out to be one function returning an `int`, so the
deck is a container of values and a class per card would have bought nothing. The honest answer
to "which pattern" is therefore: **none beyond what a container already gives you**. Iterator is
the closest fit conceptually — deal from the front, reshuffle at the end — but `std::vector`
with an index already provides it, and wrapping that in a hand-written iterator would be
notation, not design.

### Design Question 3 — Is Decorator a good pattern for Workshop Upgrades?

**No.** Four reasons, strongest first:

1. **Tolls are a table, not a composition.** Tinder's tolls run 2, 10, 30, 90, 160, 250 —
   neither additive nor multiplicative. Decorator composes behaviour, but there is no per-upgrade
   delta to compose; each level is an independent table entry. The pattern would wrap five
   objects only to index an array.
2. **Selling breaks the chain.** An Artificer may sell one upgrade at a time. Decorator has no
   removal protocol — you would have to unwrap and rebuild the chain, which is not what the
   pattern is for.
3. **Identity churn.** Re-wrapping produces a *different object*. `Player` and `Board` hold
   pointers to the district; every purchase or sale would invalidate them or leave them pointing
   at a stale wrapper. This is the decisive one: ownership in this game is recorded by pointer
   identity.
4. **Bounded at five.** Decorator earns its keep with open-ended composition. Upgrades cap at
   four Forge Stations plus a Master Atelier.

**Implemented instead:** an `int upgrades_` on `Workshop` indexing a fixed toll table. The
guild-complete doubling on unimproved Workshops is a *rule* evaluated at toll time, not a
decoration of the object.

---

## 5. Challenges and How They Were Addressed

Most of these were found by deliberately testing rules the spec states but play rarely exercises,
rather than by waiting for them to surface.

### 5.1 The spec contradicts itself in two places

The board figure and Table 1 disagree. The figure prices **Veil-watch at 180**, Table 1 says
**170**; the figure's legend lists **five** square numbers for **four** Forges, implying square
14 is a Forge when the save-file ordering places **Westgate Depot** there (between Coal-gate 13
and Mist-forge 16). Both were resolved in favour of Table 1 and the save-file ordering, since
those are internally consistent with the toll tiers and upgrade costs. Documented rather than
silently chosen.

### 5.2 Backward movement paid the Grand Entrance bonus

`passedGrandEntrance(from, to)` tested `to < from`, which is true for *any* backward move — so a
Wandering Market sending an Artificer from square 5 to square 2 wrongly paid 200 Reagents. The
destination alone cannot distinguish a forward lap from a backward step. The signature now takes
the step count and pays only for `steps > 0`. Later extended to count laps, so a `-testing` roll
that crosses twice pays twice.

### 5.3 A save/load cycle resurrected an eliminated Artificer

The save format has no field for insolvency, and `save` wrote every Artificer in the roster. An
insolvent one came back as a normal player holding 0 Reagents — and, being first in the file,
took the first turn. `save` now writes only solvent Artificers, with `numPlayers` as the solvent
count. This mattered doubly because loading is the spec's top marking priority.

### 5.4 An Artificer who could not pay the Lockup fine was detained forever

On the third turn the spec forces the fine or a token. The code checked affordability first and
printed "cannot pay", so a poor Artificer stayed detained with `lockupTurns` climbing past three
and no way out — they could not even declare insolvency, because no debt had been registered.
The fine now goes through the ordinary debt path, which blocks the turn until it is settled or
insolvency is declared.

### 5.5 The Lockup exit procedure offered only one of its three routes

Spec p. 7 lists three ways out — roll doubles, pay 50 Reagents, or spend a Free-Pass Token —
available on any detained turn. The implementation only ever rolled for doubles and made the fine
compulsory on the third turn, so an Artificer holding 800 Reagents and a token was still stuck
hoping for doubles: typing `pay` returned "I don't understand". Found by re-reading the spec
against the code. Rolling now prompts for the exit route, and asks nothing when neither is
affordable, which preserves 5.4's debt path.

### 5.6 Settling the Lockup fine did not finish the exit

Once 5.4 parked the fine as a debt, settling it released nobody: the Artificer paid, stayed
detained, and never made the move the spec requires ("then move the dice total"). Next turn they
would be charged **again**. The debt now carries the fact that it is a Lockup fine plus the roll
it owes, and settling it releases and moves them.

### 5.7 Receiving a mortgaged district drove Reagents negative

The 10% transfer charge was subtracted directly with no affordability check, taking one Artificer
to **−22 Reagents**. The fix differs by path, deliberately: a **trade** is voluntary, so it is
now *refused* up front if either side cannot cover the charge; an **insolvency transfer** is not
voluntary, so its fees are accumulated and routed through the debt system.

### 5.8 The debt system had one slot, and silently forgave debts

Chasing 5.7 exposed a deeper problem. The whole debt system was three fields on `Game`, so a
second debt overwrote the first. Normally safe — a turn cannot end while the current Artificer
owes — but insolvency charges mortgage fees to the **creditor**, who is not the current player,
so that one debt outlives its turn. A reproduction showed a creditor's 26-Reagent obligation
being erased by an unrelated debt, leaving them 26 Reagents richer with the Treasury unpaid.

Replaced with `std::map<Player*, Debt>`, plus a guard that an Artificer who owes anything may not
roll. The guard is what makes one debt per player *sufficient*: without it, a player could begin a
turn owing the Treasury, roll into a toll, and owe two creditors at once — a situation insolvency
has no answer for. It is also closer to the spec, which resolves a shortfall on the spot.

### 5.9 Loading accepted impossible districts

Owner and upgrades were validated independently, so `Tinder GUILD -1` (unowned but mortgaged) and
`Tinder GUILD 3` (unowned but upgraded) both loaded. Mortgaging and upgrading are owner actions;
a district held by the Guild must be plain. Now rejected with a specific message.

### 5.10 Malformed input silently did something else

`roll 1 2` without `-testing` rolled random dice, ignoring the arguments; so did `roll 1`,
`roll abc` and `roll 4x 2` *inside* testing mode. Silently doing something other than what was
typed is worse than refusing. Each now reports specifically.

### 5.11 The renderer threw `std::length_error`

When the token string exceeded a 12-character cell, the padding length underflowed as an unsigned
value. The root cause was not the arithmetic but that nothing enforced the spec's 2–6 player cap,
so `Game::addPlayer` now rejects a seventh Artificer and a duplicate token, with clamping as a
second line of defence.

### 5.12 A prompt that invited the wrong answer

Live play exposed a wording problem: the auction prompt read "raise or withdraw? [amount/withdraw]",
which invites typing the word "raise" when it wants a bare number. Three inputs were rejected
before the right form was found. Reworded as an instruction — `enter an amount above 0, or
"withdraw"` — with the rejection message naming the actual ceiling.

### 5.13 An ambiguity the spec does not resolve

If every Artificer withdraws from an auction before any bid, the survivor's "final bid" does not
exist. Rather than transfer a district for nothing, it stays with the Guild. Flagged here because
the spec is genuinely silent and a different reading is defensible.

---

## 6. AI Tool Usage Disclosure

> **Read and correct this section before submitting.** It must match your own account.

### Tool used

**Claude (Anthropic), via Claude Code**, used throughout the project.

### Extent of use — heavy

AI assistance was **extensive**. It is disclosed in full here, on the understanding that thorough
disclosure of heavy use is graded the same as thorough disclosure of light use, and that failure
to disclose is an academic offence.

The AI was used for:

- **Reading and interpreting the specification.** Extracting the board layout, Table 1, the save
  format and the command list from the PDFs, and identifying the two internal contradictions
  described in §5.1.
- **Reading the course chapters** in `resources/` before each pattern was implemented, and
  matching the implementation to each chapter's conventions (for example Chapter 17's
  View-owns-Controller arrangement, and Chapter 16's Context-owns-all-states structure).
- **Architecture and pattern selection**, including the decision *not* to claim Factory Method,
  not to use Decorator, and to omit `undo()` from `Command`.
- **Writing the C++ source.** Substantially all of the submitted code was AI-written.
- **Writing the test harnesses** (three suites, 78 checks) and the save-file fixtures.
- **Diagnosing and fixing defects**, including all of §5.
- **Producing the UML diagrams** and drafting this report.

### My own contribution

- Set the project conventions the work had to follow, including the rule that **every design
  pattern used must appear in the course `resources/` chapters** and must follow that chapter's
  conventions — which directly shaped the architecture.
- Directed scope at each stage: which spec priorities to implement in which order, and what to
  leave out.
- Made design calls put to me, including labelling the district factory honestly as Simple
  Factory rather than claiming Factory Method.
- **Reviewed the implementation and independently identified five defects**, which the AI then
  confirmed and fixed: the stranded Lockup fine (§5.6), the negative-Reagent mortgage transfer
  (§5.7), load accepting unowned-but-mortgaged or upgraded districts (§5.9), the command-line
  parsing deviating from the fixed contract, and `roll 1 2` outside `-testing` silently becoming
  a random roll (§5.10).
- Built and ran the program **on the UW student server**, and found the auction prompt problem
  (§5.12) through live play.
- Directed a full review of the project from a grading perspective, which produced the remaining
  work list.

### Verification

Correctness was not taken on trust. Every rule was checked by running the program:

- three test suites — 34 board/movement checks, 23 save-format checks, 21 card/deck checks;
- the save round-trip is verified **byte-identical**, and nine malformed save files are each
  verified to be rejected with a specific message;
- the two probability tables are checked against the spec by drawing 200,000 outcomes and
  comparing observed frequencies, because a mistyped weight is invisible in play;
- the build is warning-free under `-Wall -Wextra`, and compiles and runs on the student server.

### Statement

I understand that I must be able to explain any part of the submitted code at the demo, and that
a discrepancy between what I can explain and what was submitted will be referred under Policy 71.

---

## 7. Extra Features Implemented

**Shuffled Enchantment Deck** (`./arcanemarket -deck`).

The spec defines the Enchantment squares as a probability table and lists a real deck as an
enhancement. Both are implemented and selected at startup, so the same binary plays with or
without it and no recompilation is needed, as the spec requires of an enhancement.

The deck holds eighteen cards — one per Table 3 weight unit — dealt from the front and reshuffled
on exhaustion. Design Question 2 above explains why this is a genuinely different mechanic rather
than a reskin of the table, and the test suite verifies the defining property: **100 consecutive
passes each deal exactly Table 3**, and a drawn card leaves the deck.

---

## 8. Lessons Learned

**Reading the chapter first changes the design, not just the vocabulary.** Chapter 17's MVC is
not the arrangement most people assume — the View owns the Controller and queries the Model
directly. Had the code been written first and the chapter consulted afterwards, the result would
have been a plausible-looking MVC that a reviewer following the chapter would mark down. The same
happened with Chapter 16, where the Context owns *all* states up front rather than allocating on
transition.

**A pattern that is not reachable is not implemented.** `AwaitingTradeResponseState` was written,
then deleted, because trading did not exist yet — the compiler flagged its unused `Turn&` member.
It went back in when `trade` landed and it had real behaviour. The same discipline produced the
decision to name the district factory Simple Factory and to omit `undo()` from `Command`. Claiming
fewer patterns accurately is worth more than claiming more loosely, because Phase 2 review
classifies each claim as actually realized, partially realized, or merely named.

**Predictions about design are worth less than the act of building it.** The plan asserted that a
real card deck would need Command objects. Building it showed the seam was a single function
returning an `int`. The Design Question 2 answer is now grounded in code rather than in a guess.

**Test what play cannot reveal.** The two probability tables are the one part of the project where
a wrong number produces plausible output forever. Compile-time sum checks plus 200,000-draw
frequency comparisons catch a transposed digit; no amount of playing reliably would.

**Bugs cluster where a rule spans two pieces of code.** Almost every defect in §5 lived in a gap
rather than inside a function: movement direction versus bonus eligibility, the save format versus
the insolvency flag, the Lockup fine versus the debt system, the owner field versus the upgrade
field. Testing individual functions would have missed all of them; the tests that found them
exercised a rule end to end.

**A fix that only treats the symptom hides the real defect.** The renderer's `length_error` looked
like an arithmetic bug; the actual fault was that nothing enforced the 2–6 player cap. The
negative-Reagent transfer looked like a missing check; chasing it properly exposed a debt system
that could silently forgive obligations. Both times, asking "why was this state reachable at all"
found something larger than the reported symptom.
