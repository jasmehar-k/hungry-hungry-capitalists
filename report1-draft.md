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

Source in `uml.md`, rendered images in `diagrams/uml-1.png` … `uml-7.png`.

| Figure | Pattern demonstrated |
|---|---|
| `uml-1.png` | MVC + Observer |
| `uml-2.png` | Template Method |
| `uml-3.png` | State — `Turn` and its four phases |
| `uml-4.png` | State — phase transitions (state diagram) |
| `uml-5.png` | Command |
| `uml-6.png` | Simple Factory |
| `uml-7.png` | Structural context — how the patterns compose |

**Level of abstraction: the minimal detail needed to demonstrate each pattern's
uniqueness.** The diagrams are organised by pattern rather than by file, and each box
carries only the members that make its pattern identifiable — the roles, the polymorphic
hook, and the data that distinguishes the pattern from a plain class. Members that exist
in the code but say nothing about the pattern are omitted. This mirrors how the course
chapters draw their own figures: Chapter 20's `Light` receiver shows only `setLevel` and
`level`, not everything a `Light` can do.

Concretely, that means `Workshop` appears in the Template Method figure with nothing but
`computeToll`, because the overriding hook *is* its role in that pattern; its upgrade
level and toll table appear nowhere, because they demonstrate nothing about Template
Method. Conversely `RollCommand` shows its private `game_`, `die1_` and `die2_`, because
a command binding its receiver and arguments at construction is precisely what Chapter 20
identifies as the pattern's distinguishing feature.

Where a compartment is empty it is a deliberate statement that no member of that kind is
relevant to the pattern, not an omission.

Notation follows Chapter 8: `+ - #` for visibility, *italics* for pure virtual, underline
for static, `const` marking query operations, hollow triangle for generalization, filled
diamond for composition, hollow diamond for aggregation, dashed arrow for dependency.
Every association carries multiplicities at both ends, since the chapter notes that a
final diagram should not leave them blank.

**Choosing each relationship** followed the chapter's decision procedure rather than
guesswork:

- `Game ◆— Board`, `◆— Dice`: held by value, destroyed with the `Game`. Composition.
- `Game ◆— Player / Turn / CardDeck`, `Board ◆— District`, `Turn ◆— TurnState`:
  `unique_ptr`, which Chapter 8 calls "the key signal of composition".
- `TextView ◇— ControllerInterface`: a `shared_ptr`. The Controller does not die with the
  View, so this is **aggregation**, matching Chapter 17's own diagram of the same idea.
- `Subject —▶ Observer`: a plain association, not a diamond. The observer list holds
  non-owning raw pointers, so a Subject is not *made of* its Observers.
- `OwnableDistrict —▶ Player`: plain association, role name `owner`, multiplicity `0..1`
  because an unowned district holds `nullptr`.

---

## 3. Design Patterns Used

| Pattern | Chapter | Location | Status |
|---|---|---|---|
| MVC | 17 | `game.*` (Model), `textview.*` (View), `controller.*` (Controller) | Realized |
| Observer | 10, 17 | `observer.h`; `Game : Subject`, `TextView : Observer` | Realized |
| Template Method | 14 | `district.*`, `workshop.cpp`, `forge.cpp`, `depot.cpp` | Realized |
| State | 16 | `turn.*` (Context), `turnstate.*` (4 states) | Realized |
| Command | 20 | `command.*` (13 commands), `commandparser.*`, `GameController` as Invoker | Realized |
| Simple Factory (idiom) | 12 | `districtfactory.*`, `commandparser.*` | Used, **not** claimed as Factory Method |
| Decorator | 11 | — | **Deliberately rejected** — see Design Question 3 |
| Iterator | 15 | — | Not used — the circuit is indexed directly by square number |

### 3.1 MVC (Chapter 17)

`Game` is the Model and holds every piece of game state. `TextView` is the View. `GameController`
is the Controller. The chapter's arrangement is **not** the one most people assume, and the code
follows the chapter rather than the assumption:

- the **View owns the Controller** as its Strategy (`shared_ptr<ControllerInterface>`),
- the **View registers as an Observer of the Model**,
- the **View queries the Model directly** when redrawing.

The chapter warns that routing all data through the Controller "is not MVC", and that the
Controller only handles user-initiated events. That is exactly the split here: user input
arrives at `TextView::handleInput`, which forwards to the Controller; the Controller mutates
the Model; the Model notifies; the View re-queries and redraws.

**A deliberate deviation.** The chapter says the Model should notify at the end of every
mutating method. Here notification fires once, at the command boundary, in
`GameController::handleCommand`. The reason is that the spec redraws the *whole* board after
every command, and a single command can move an Artificer, pay a toll, transfer a district and
change the turn state — per-mutator notification redrew the board four times for one command.
This was found in practice: attaching the View before setup made six `addPlayer` calls print
six boards. Batching to the command boundary makes the redraw match the real unit of change.

### 3.2 Observer (Chapters 10, 17)

`observer.h` implements `Subject` and `Observer` in the chapter's shape: `addObserver` with
duplicate detection via `std::find`, `removeObserver` via the erase-remove idiom,
`notifyObservers` iterating and calling `update()`. `Game` extends `Subject` and never names a
View. `TextView` registers itself in its constructor and deregisters in its destructor, the same
lifetime contract Chapter 17's `StudentLeaf` uses. The observer list is non-owning raw pointers.

The payoff is concrete rather than theoretical: because `Game` holds no reference to any view, a
second view could attach with no change to the Model at all.

### 3.3 Template Method (Chapter 14)

`OwnableDistrict::landOn` is declared `final`. It fixes the invariant sequence of a visit —
unowned? offer purchase; owned by the visitor? nothing; mortgaged? nothing; otherwise charge —
and defers exactly one step, the price, to the pure virtual hook `computeToll`.

The three subclasses vary only in that hook, and they vary genuinely:

- `Workshop` indexes a six-entry toll table by upgrade level, doubling an *unimproved* toll when
  the owner holds the whole Crafting Guild.
- `Forge` indexes `kForgeLevy` by how many Forges the owner controls (25 / 50 / 100 / 200).
- `Depot` multiplies the dice total by 4, or by 10 when one Artificer holds both.

Making `landOn` `final` is the part that matters: it is what stops a subclass quietly reordering
the sequence, which is the whole point of the pattern.

### 3.4 State (Chapter 16)

`Turn` is the Context. Following the chapter exactly, it **owns all four states as `unique_ptr`
members constructed once** and swaps a raw, non-owning `state_` pointer through `setState`, so a
transition allocates nothing. Each concrete state holds a `Turn&` reference and is `final`, with
an `explicit` constructor. `turnstate.h` forward-declares `Turn` to break the cycle.

The four phases exist because the spec's own turn description gives them different legal command
sets: `PreRollState` (may roll, may not end turn), `PostRollState` (may end turn, may not roll),
`LockupState` (rolling attempts the exit procedure), and `AwaitingTradeResponseState` (only
`accept` / `reject`).

`Turn` also records `resume_`, the phase a trade offer interrupted, because an offer may be made
**before or after** rolling and the Context cannot assume a fixed state to return to. That is
precisely the case an enum-plus-if-else turn model gets wrong silently.

**`Turn` is a separate class rather than folding the states into `Game`** because `Game` is
already the Model; holding four more members plus the turn's rules would push it past one
responsibility.

### 3.5 Command (Chapter 20)

`Command` declares `execute()` and nothing else. Chapter 20 says the interface declares
`execute()` "and, **when undo is required**, its inverse, `undo()`". ArcaneMarket has no undo
requirement anywhere in the spec, so adding an `undo()` that all thirteen commands stub out
would claim a capability the program does not have.

Per the chapter, each concrete command **binds its receiver and arguments as data members at
construction**, so commands are built fresh from each input line rather than registered once and
handed an argument stream. `GameController` is the Invoker and knows only the interface.
`CommandParser` returns `nullptr` for anything unrecognized, and that single return path is what
satisfies the spec's "handle misspelled commands gracefully".

`MessageCommand` is the one command with no receiver: it reports a refusal. It exists so that a
specific diagnostic ("explicit dice values need `-testing`") can replace the generic "I don't
understand", while keeping console output in the Command layer instead of pushing it into the
parser.

### 3.6 Simple Factory — used, and named honestly

`DistrictFactory::create` builds the 40 districts from a static table, and `CommandParser::parse`
builds commands from input. Both are the **Simple Factory idiom, not the GoF Factory Method**,
and the report says so deliberately.

Chapter 12 states plainly that the Gang of Four never named Simple Factory and that it suits "a
small, fixed set of product types that you do not expect to grow". The 40-square circuit and the
spec's command list are exactly that. Building an abstract creator hierarchy purely to make
Factory Method applicable would be the over-engineering the same chapter warns against — "don't
reach for factories when a constructor would do". Naming it accurately costs a claimed pattern
and is the more defensible position.

### 3.7 A note on Strategy

Chapter 17 describes the Controller as the View's Strategy, and `TextView` does hold a
`shared_ptr<ControllerInterface>`. However there is currently only **one** concrete controller,
so nothing demonstrates swappability. It is therefore presented as part of MVC's compound
structure rather than claimed as a pattern in its own right. A second controller is unlikely to
earn its place later either: mode-specific command legality is State's job here, not a second
Controller's.

---

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
