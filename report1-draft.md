# ArcaneMarket — Phase 1 Report (draft)

> **Draft for review.**
>
> 1. **§8 Lessons Learned is intentionally empty.** The requirements state, in bold, that you
>    must not use AI tools to write that section. The prompts are listed there for you to answer
>    in your own words.
> 2. **§6 must be checked line by line.** The tools table, frequency ratings and reflection
>    describe *your* experience — correct anything that does not match it.
> 3. Fill the title page, embed `diagrams/*.png` directly in the PDF, and save as `report.pdf`.
> 4. **Separate deliverable:** export the full AI conversation transcripts as PDFs, ZIP them, and
>    upload the ZIP to the Phase 1 report Dropbox on LEARN (§6 requires this).

---

## 1. Title Page

- **Project title:** ArcaneMarket
- **Course:** CS247 — Software Engineering Principles
- **Instructor:** Victoria Sakhnini
- **Student name:** _[full name]_
- **Student ID:** _[ID number]_
- **User name:** _[UW userid]_
- **Submission date:** _[date]_ (deadline August 5th, 11:59 PM; no penalty up to 48 hours late)

---

## 2. UML Class Diagram

Source in `uml.md`; rendered images in `diagrams/`. All diagrams are embedded directly in this
PDF.

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

**Figures 1A and 1B together are the class diagram**, split across two clearly labelled parts as
permitted. Every major class appears in exactly one part with its key attributes and its full
public interface; where a relationship crosses the split, the far class appears as a name-only
box marked with the part that details it, so no relationship is lost at the seam.

- **Part A — application and control:** the Observer infrastructure, `TextView`,
  `ControllerInterface` / `GameController`, `CommandParser`, `Command` and all thirteen concrete
  commands, and `Turn` with its four states.
- **Part B — domain model:** `Game` and its economy (`Debt`, `TradeOffer`, the Treasury),
  `Player`, `Board`, the full `District` hierarchy, `Dice`, `CardDeck`, `DistrictFactory`,
  `DistrictSpec`, and the `SaveGame` / `Setup` namespaces.

**Figures 2–7 are supplementary per-pattern views** referenced from §3, reducing the diagram to
the participants in one pattern so its shape is visible without tracing it out of the full
diagram.

**Notation** follows Chapter 8: `+ - #` for visibility, *italics* for pure virtual, underline for
static, `const` marking query operations, hollow triangle for inheritance, filled diamond for
composition, hollow diamond for aggregation, dashed open arrow for dependency.

**Relationship kinds** were chosen with the chapter's decision procedure:

- **Inheritance** — `Game : Subject`, `TextView : Observer`, and the `District`, `TurnState` and
  `Command` hierarchies.
- **Composition** (filled diamond) wherever the whole holds the part by value or `unique_ptr` and
  destroys it: `Game ◆— Board`, `◆— Player [2..6]`, `◆— Turn`, `◆— Dice`, `◆— CardDeck [0..1]`,
  `Board ◆— District [40]`, `Turn ◆— TurnState [4]`.
- **Aggregation** (hollow diamond) for the one `shared_ptr`: `TextView ◇— ControllerInterface`.
  The Controller does not die with the View, which is what separates this from composition.
- **Dependency** (dashed) for create/use relationships that are not stored:
  `Board ⋯▶ DistrictFactory`, `DistrictFactory ⋯▶ District`, `CommandParser ⋯▶ Command`,
  `SaveGame ⋯▶ Game`.

**Multiplicities** appear on both ends of every association; the most significant are
`1 Board — 40 District`, `1 Game — 2..6 Player`, and `1 Turn — 4 TurnState`. Crafting Guild
membership is deliberately *not* shown as a multiplicity: `Guild` is an `enum class` value carried
by each `Workshop`, not a class, so guild sizes (Ember 4, Smoke 3, Arcane 4, Gilded 5) are
enforced by `Game::guildComplete`, which counts actual membership rather than assuming a fixed
group size.

Following Chapter 8, method **parameters are not associations**: `landOn(Game&, Player&)` passes
references at call time, which the chapter explicitly excludes. Only stored references and
pointers are drawn as lines.

---

## 3. Design Patterns Used

Six patterns are applied. Patterns considered and deliberately rejected are listed at the end,
since rejecting one is also a design decision.

### 3.1 Model-View-Controller

**Where it is used.** `Game` is the Model (`game.h`/`.cpp`); `TextView` is the View
(`textview.h`/`.cpp`); `ControllerInterface` and `GameController` are the Controller
(`controller.h`/`.cpp`). See Figure 2.

**Why it was chosen.** Deliberately *not* because a graphical display might be added later — that
would be speculative generality, and the enhancement was never built. The justification is that
three genuinely distinct concerns already exist in this program today:

1. **Board rendering is substantial and rule-free.** `textview.cpp` is 200 lines of grid geometry,
   cell-width arithmetic, name wrapping and token placement. None of it encodes a game rule.
2. **Input interpretation is a third thing again** — a parser plus thirteen command classes,
   concerned with what the user typed, not with what the board looks like or what the rules say.
3. **The rules themselves** must be exercisable without either of the above. This is realised, not
   hypothetical: the test harnesses construct a `Game`, drive it and assert on its state with no
   view attached at all — 78 checks run headless.

Ordinary cohesion demands that split regardless of any future view; MVC is simply the course's
named form of it.

*Alternatives rejected:* **one class holding rules and printing** — the 200 lines of layout would
sit among the rules, and headless testing would be impossible; **Model and View with no
Controller** — command parsing and legality would live inside the display class; **Controller
mediating all data** — Chapter 17 states this "is not MVC" and adds coupling, since the View can
query the Model directly.

**How it is implemented.** `Game` holds all state, extends `Subject`, and names no View.
`TextView` holds a `Game&` and a `shared_ptr<ControllerInterface>`, registers as an Observer, and
on notification re-queries the Model and redraws. Input reaches `TextView::handleInput`, which
forwards to the Controller; `GameController` mutates the Model, then fires one notification for
the whole command.

**An honest limitation.** The separation achieved is *board rendering* from rules, not *all
output* from rules. `Game` still writes narration directly to `std::cout` — "Marco pays 26
Reagents to Vera", the auction and Guild Levy prompts, and so on, 83 call sites in total. A
stricter MVC would route those through the View. This was left as-is partly for time and partly
because Chapter 16's own `GumballMachine` prints from the Model, but it is the weakest point of
the separation and a fair thing to be challenged on.

### 3.2 Observer

**Where it is used.** `Subject` and `Observer` in `observer.h`; `Game : Subject`;
`TextView : Observer`. See Figure 2.

**Why it was chosen — and where that reasoning is weak.** Unlike the separation above, Observer
does **not** pay for itself in the program as submitted, and it would be dishonest to claim it
does. There is exactly one Observer (`TextView`), exactly one Subject (`Game`), and
`notifyObservers()` is invoked from exactly one place — `GameController::handleCommand` — where it
iterates a one-element vector. Replacing the whole mechanism with a direct `view.render()` call at
that same site would be behaviourally identical and strictly simpler. By a strict YAGNI reading
this is over-built, and that is a fair criticism.

Three things are offered in its defence, none of which is present-day value:

- The specification's own enhancement list names a graphical display. That makes a second view a
  *documented* extension point rather than an invented one — YAGNI warns against needs you
  imagine, and this one is written down. It remains optionality, not value.
- Design Question 1 asks directly whether Observer suits the Circuit Board. Implementing it makes
  that answer concrete and testable rather than hypothetical.
- The cost is genuinely small: 45 lines in `observer.h`, one virtual call per command, no
  allocation.

*Alternatives rejected:* **`Game` holding a `TextView*`** — this is the honest competitor, and its
only real defect is that the Model would then name a concrete View; **the main loop polling
`view.render()` unconditionally** — rejected because it puts the redraw decision outside both the
Model and the Controller, though it too would work.

**How it is implemented.** `Subject` keeps a `vector<Observer*>` of non-owning pointers with
`addObserver` (duplicate-checked via `std::find`), `removeObserver` (erase-remove) and
`notifyObservers`. `TextView` registers in its constructor and deregisters in its destructor.
Notification fires once per command rather than from every mutator, because the spec redraws the
whole board per command — a change that was made after per-mutator notification printed the board
six times during six-player setup.

### 3.3 Template Method

**Where it is used.** `OwnableDistrict::landOn` (declared `final`) with the pure virtual hook
`computeToll`, overridden by `Workshop`, `Forge` and `Depot`. See Figure 3.

**Why it was chosen.** Every visit to an ownable district follows one fixed sequence — unowned so
offer it; owned by the visitor so nothing; mortgaged so nothing; otherwise charge — but the amount
is computed three different ways. *Alternatives rejected:* **a single `landOn` with a `switch` on
`DistrictKind`** — adding a kind would edit that function, and the invariant sequence would sit
interleaved with three pricing rules; **Strategy, injecting a `TollPolicy`** — the pricing rule is
fixed by the district's type and never changes at runtime, so a swappable policy adds an
allocation and indirection for flexibility the game never uses.

**How it is implemented.** `landOn` is `final` so no subclass can reorder or skip a step, and
`computeToll` is a protected pure virtual — the single point of variation. `Workshop` indexes a
six-entry table by upgrade level, doubling an unimproved toll when the Guild is complete; `Forge`
indexes a levy table by Forges owned; `Depot` multiplies the dice total by 4, or 10 when one
Artificer holds both.

### 3.4 State

**Where it is used.** `Turn` is the Context (`turn.h`/`.cpp`); `TurnState` with `PreRollState`,
`PostRollState`, `LockupState`, `AwaitingTradeResponseState` (`turnstate.h`/`.cpp`). Figures 4–5.

**Why it was chosen.** Which commands are legal depends on the turn phase: `roll` only when a roll
is pending, `next` only when it is not, rolling in The Lockup runs an exit procedure instead of
moving, and while a trade offer stands only `accept`/`reject` apply. *Alternatives rejected:* **an
`enum` with `if`/`else` chains in each command** — Chapter 16's argument, that every new phase
edits every command and the logic scatters; **a set of booleans** (`hasRolled`, `isDetained`,
`awaitingTrade`) — they admit contradictory combinations no rule permits, and nothing enforces
legal transitions.

**How it is implemented.** `Turn` owns all four states as `unique_ptr` members built once and
selects the active one through a raw non-owning `state_` pointer, so a transition allocates
nothing. Each state holds a `Turn&` and drives its own transitions via `setState`. `Turn` also
records `resume_`, the phase a trade offer interrupted, because an offer may be made before *or*
after rolling.

### 3.5 Command

**Where it is used.** The `Command` interface and thirteen concrete commands (`RollCommand`,
`NextCommand`, `MortgageCommand`, `UnmortgageCommand`, `UpgradeCommand`, `TradeCommand`,
`AcceptCommand`, `RejectCommand`, `InsolventCommand`, `AssetsCommand`, `AllCommand`,
`SaveCommand`, `MessageCommand`) in `command.h`/`.cpp`; `CommandParser`; `GameController` as
Invoker; `Game` as Receiver. See Figures 1A and 6.

**Why it was chosen.** Eleven spec commands with different arities, from bare `next` to
`trade <name> <give> <receive>`, and misspellings must fail gracefully. *Alternatives rejected:*
**an `if`/`else` chain on the verb in the controller** — mixes parsing with execution and grows
without bound; **a `map<string, function<void(istream&)>>`** — arguments would be re-parsed inside
each handler or captured untyped, whereas binding them as typed members at construction is what
Chapter 20 prescribes and is what makes each request self-contained.

**How it is implemented.** Each command stores its receiver (`Game&`) and parsed arguments as data
members, so it is complete before execution. `CommandParser::parse` builds the right command and
returns `nullptr` for anything unrecognised — the graceful-failure path. `GameController` fires
`execute()` through the interface. `undo()` is deliberately absent: Chapter 20 adds it only "when
undo is required", and ArcaneMarket has none.

### 3.6 Simple Factory

**Where it is used.** `DistrictFactory::create` (`districtfactory.*`), called from `Board`'s
constructor; the same idiom in `CommandParser::parse`. See Figure 7.

**Why it was chosen.** `Board` must build 40 districts of twelve kinds without concrete class names
spreading through the code. *Alternatives rejected:* **a `switch` inside `Board`'s constructor** —
puts every concrete name into `Board` and mixes construction with circuit management; **GoF Factory
Method with an abstract creator hierarchy** — Chapter 12 says Simple Factory suits "a small, fixed
set of product types that you do not expect to grow", which the 40-square circuit is, and warns
against reaching for factories when a constructor would do.

**How it is implemented.** One static `create` takes a `DistrictSpec` from the board table,
switches on its `kind`, and returns `unique_ptr<District>`. It is the only place concrete district
names appear. It is named the Simple Factory *idiom* rather than Factory Method because there is no
creator hierarchy — precisely the structural difference between the two.

### 3.7 Patterns considered and deliberately not used

- **Decorator** for upgrades — see Design Question 3.
- **Iterator** for the circuit — the board is a `vector` indexed by square number and movement is
  arithmetic modulo 40; a hand-written iterator would add notation without removing indirection.
- **Strategy** as a separate claim — Chapter 17 describes the Controller as the View's Strategy and
  `TextView` does hold a `shared_ptr<ControllerInterface>`, but only one concrete controller
  exists, so nothing demonstrates swappability. Reported as part of MVC rather than claimed alone.

---

## 4. Answers to Design Questions

### Design Question 1 — Observer Pattern for the Circuit Board

> *After reading the Districts subsection of the specification, would the Observer Pattern be a
> good pattern to use when implementing the Circuit Board? Why or why not?*

Yes, and it is used. **The Subject is `Game`** — the Model owning the board, the players and the
turn — and **the Observer is `TextView`**. Districts are deliberately *not* Subjects. A
per-district Subject would let one square announce its own change, but the spec redraws the
**entire** board after every command, so there is no partial repaint for that finer granularity to
optimise; it would add an observer list to all forty districts to trigger the same full redraw.

**The event that triggers notification is the completion of a command**, not an individual field
write. This was a deliberate departure from Chapter 17, which notifies at the end of every mutating
method. One command can move an Artificer, pay a toll, transfer a district and change the turn
phase; notifying per mutation redrew the board four times for one command. Notification therefore
fires once, from `GameController::handleCommand`.

**Compared with direct method calls, the honest answer is that Observer is not clearly better for
this program as submitted.** There is one view, one subject, and one call site, so
`notifyObservers()` iterates a single-element vector; a direct `view.render()` from the same place
in the Controller would be behaviourally identical and simpler. Claiming Observer earns its keep
here would be arguing from a graphical display that was never built, which is speculative
generality rather than design.

What Observer buys is narrower and worth stating precisely: the Model never names a concrete View.
That is optionality against an extension the specification itself lists, not a benefit realised
today, and it costs 45 lines. Whether that trade is right is genuinely arguable — a reviewer
applying YAGNI strictly would call it over-built, and I would not dispute the reasoning, only the
conclusion. What is *not* arguable is the separation underneath it: rendering the board is 200
lines of geometry with no rules in it, and the rules are exercised headless by the test harness.
That split is justified whether or not the notification between them goes through Observer.

### Design Question 2 — Card-Deck Mechanics for Wandering Market and Enchantment Deck

> *Suppose we wanted to model the Wandering Market and Enchantment Deck more closely after
> card-deck mechanics, with a shuffled deck drawn one card at a time and reshuffled when
> exhausted. Is there a suitable design pattern you could use? How would you structure it?*

The base spec defines both squares as probability *distributions* (Tables 2 and 3), and that is
what `cards.h` implements with integer weights. A deck is a different mechanic, not a reskin: a
distribution is memoryless, so −200 can recur immediately, whereas a deck is stateful — each card
leaves it when drawn, shifting the remaining odds and making card-counting a real strategy. Over
one full pass a deck deals Table 3 exactly.

**The implemented deck** (`CardDeck`, enabled with `-deck`) holds eighteen cards, one per Table 3
weight unit, dealt from the front and reshuffled on exhaustion. Because every current card is
simply a Reagent delta, the deck is a `vector<int>` and **no pattern beyond a container is
warranted**; wrapping seven integers in seven classes would be notation, not design. Both
implementations meet behind one seam, `Game::drawEnchantmentValue()`.

**If we were to extend the design** with heterogeneous card types — "advance to Grand Entrance",
"receive a Free-Pass Token" — the representation would have to change, and **Command** is the
pattern that fits: an abstract `EnchantmentCard` with a single `apply(Game&, Player&)` operation,
each concrete card encapsulating its own effect. The deck would become
`vector<unique_ptr<EnchantmentCard>>`. Adding a card type would then mean adding one class and one
line in the deck's build function; the deck's draw and reshuffle logic would not change, which is
the Open–Closed property the question asks about. `CardDeck` could be refactored to this by
changing its element type alone — nothing outside it depends on the element being an `int`.

### Design Question 3 — Decorator Pattern for Upgrades

> *Is the Decorator Pattern a good pattern to use when implementing Upgrades on Workshop
> districts? Why or why not? Consider how Toll Fees are calculated with and without upgrades, and
> how selling upgrades affects the object structure.*

**No.** Decorator would model an upgraded Workshop as a chain: a base `Workshop` wrapped by one
`UpgradeDecorator` per level, each `computeToll` calling the wrapped object's and adjusting the
result. That works only if each upgrade contributes a *composable* increment.

It does not. Tinder's tolls run 2, 10, 30, 90, 160, 250 — neither additive nor multiplicative, so
each decorator would have to know the absolute figure for its own level and discard the wrapped
value entirely. The chain would exist purely to count how many wrappers deep it was.

**Selling an upgrade is where it breaks structurally.** An Artificer may sell one level at a time,
which means removing a decorator from the chain. Decorator has no removal protocol: the district
would have to be unwrapped and rebuilt, and because re-wrapping produces a *different object*,
every `Player` and `Board` pointer recording ownership would be left pointing at a stale wrapper.
Ownership in this game is recorded by pointer identity, so that is decisive.

**A counter achieves the same effect with far less complexity**, and is what was implemented: an
`int upgrades_` on `Workshop` indexing a fixed six-entry toll table, so `computeToll` is one array
lookup, buying is `+1`, selling is `−1`, and the object's identity never changes. Decorator also
earns its keep on open-ended composition, whereas upgrades cap at five. The guild-complete doubling
on unimproved Workshops is applied as a rule at toll time rather than as another decoration.

---

## 5. Challenges & How You Addressed Them

### 5.1 The specification contradicts itself

**The challenge.** The board figure and Table 1 disagree in two places. The figure prices
Veil-watch at 180 where Table 1 says 170, and the figure's legend lists five square numbers for
four Forges, implying square 14 is a Forge when the save-file ordering places Westgate Depot there
(between Coal-gate 13 and Mist-forge 16). Building from the wrong source would corrupt every toll
downstream.

**Approaches considered.** Following the figure; following Table 1; or picking whichever seemed
more authoritative case by case.

**Resolution.** Both resolved in favour of Table 1 and the save-file ordering, because those two
are internally consistent with each other and with the upgrade costs and six toll tiers, while the
figure is not. Recorded in the project notes rather than decided silently, so it can be defended.

### 5.2 Distinguishing a forward lap from a backward move

**The challenge.** `passedGrandEntrance(from, to)` tested `to < from`, which is true for *any*
backward move — so a Wandering Market sending an Artificer from square 5 to square 2 wrongly paid
the 200-Reagent bonus.

**Approaches considered.** Comparing destination against origin (the broken version); tracking a
lap count on `Player`; passing the direction of travel alongside the destination.

**Resolution.** The destination alone cannot carry this information, so the signature takes the
**step count** instead, paying only when `steps > 0`. Later extended from a boolean to a lap count,
because `-testing` accepts dice totals large enough to cross Grand Entrance twice in one move.

### 5.3 A save/load cycle resurrected eliminated Artificers

**The challenge.** The save format has no field for insolvency, and `save` wrote every Artificer in
the roster. A bankrupt one reloaded as a normal player holding 0 Reagents and, being first in the
file, took the first turn. Loading is the spec's top marking priority, so this mattered doubly.

**Approaches considered.** Adding a non-standard insolvency field (which would break the required
format); keeping insolvent players in the file and filtering on load; excluding them at save time.

**Resolution.** Excluding at save time, with `numPlayers` written as the solvent count. The format
stays exactly as specified, and an eliminated Artificer no longer exists in the saved game — which
is what elimination means.

### 5.4 A rule spanning two subsystems left a player permanently stuck

**The challenge.** On the third turn in The Lockup the spec forces the fine or a token. The code
checked affordability first and printed "cannot pay", so an Artificer with 10 Reagents stayed
detained indefinitely, unable even to declare insolvency because no debt had been registered.

**Approaches considered.** Special-casing the fine as a distinct obligation; releasing the
Artificer free when they cannot pay; routing the fine through the existing debt mechanism.

**Resolution.** The third. The fine goes through `payTreasury`, which parks a debt when
unaffordable, and the debt blocks the turn until settled or insolvency is declared. This exposed a
follow-on defect — settling the debt released nobody and never made the required move, so the
Artificer would be charged again next turn. The `Debt` record now carries the fact that it is a
Lockup fine plus the roll it owes, and settling it completes the exit.

### 5.5 One debt slot silently forgave obligations

**The challenge.** Fixing a negative-balance bug — a mortgaged district transfer subtracted its 10%
fee with no affordability check, taking one Artificer to −22 Reagents — exposed something larger.
The whole debt system was three fields on `Game`, so a second debt overwrote the first. That is
normally safe, because a turn cannot end while the current Artificer owes; but insolvency charges
mortgage fees to the **creditor**, who is not the current player, so that debt outlives its turn. A
reproduction showed a creditor's 26-Reagent obligation erased by an unrelated debt, leaving them
richer and the Treasury unpaid, with no error message.

**Approaches considered.** Leaving it and documenting the limitation; resolving the creditor's fee
immediately with a modal raise-funds loop during another player's turn; tracking debts per
Artificer.

**Resolution.** A `map<Player*, Debt>`, plus a guard that an Artificer who owes anything may not
roll. The guard is what makes one debt per player *sufficient*: without it, a player could begin a
turn owing the Treasury, roll into a toll, and owe two creditors at once — a situation insolvency
has no answer for. It is also closer to the spec, which resolves a shortfall on the spot.

### 5.6 Verifying rules that play cannot exercise

**The challenge.** Tables 2 and 3 are probability distributions. A transposed weight produces
plausible output forever; no amount of playing reliably reveals it.

**Approaches considered.** Re-reading the tables by eye; asserting on individual draws; measuring
the distribution statistically.

**Resolution.** Compile-time and statistical checks together. The weights are asserted to sum to 24
and 18 at compile time, and a harness draws 200,000 outcomes from each table and compares observed
frequencies against the spec within a tolerance. The same harness verifies the shuffled deck's
defining property — that 100 consecutive passes each deal exactly Table 3.

### 5.7 Silent failure on malformed input

**The challenge.** `roll 1 2` without `-testing` rolled random dice and ignored the arguments; so
did `roll 1`, `roll abc` and `roll 4x 2` inside testing mode. Doing something other than what was
typed, without saying so, is worse than refusing.

**Approaches considered.** Letting the parser print diagnostics directly; returning `nullptr` and
accepting the generic "I don't understand"; introducing a command that reports a refusal.

**Resolution.** The third. `MessageCommand` carries a specific diagnostic, so the parser stays free
of console output while the user gets a precise message. A related robustness fix: an unrecognised
command-line argument used to abort the program entirely — the wrong failure mode at a demo — so
unknown arguments now warn and continue.

---

## 6. AI Tool Usage Disclosure

> **Check this section against your own recollection before submitting.** The frequency ratings
> and the reflection describe your experience.

### 6.1 Tools used

| Tool | Tier | Primary purpose | Frequency |
|---|---|---|---|
| Claude (Anthropic), via Claude Code | _[free / paid / institutional / GitHub Education / API — fill in]_ | Specification analysis, design and pattern selection, C++ implementation, test harnesses, debugging, UML, report drafting | Frequently |

_Add rows for any other tools used by any member._

### 6.2 Categorised usage log

| Category | Used? | Concrete steps |
|---|---|---|
| **Conceptual learning** | **Yes** | Had the course chapters in `resources/` read and summarised *before* each pattern was implemented — Chapter 17 on MVC's arrangement, Chapter 16 on the State Context, Chapter 20 on the Command interface, Chapter 12 on Simple Factory vs Factory Method, Chapter 8 on UML notation. |
| **Brainstorming design alternatives** | **Yes** | Compared the alternatives recorded in §3 — Template Method vs a `switch` on district kind vs Strategy; State classes vs an enum with conditionals; Command objects vs a `map` of handlers. Also debated whether the district factory could honestly be called Factory Method, and whether card effects needed to be objects. |
| **Code generation** | **Yes** | Substantially all of the submitted C++ was AI-written — Makefile, board data table, district hierarchy, command interpreter, save/load, auctions, insolvency, upgrades and trading. |
| **Debugging assistance** | **Yes** | Used to reproduce and fix the defects in §5. Typical interaction: describe a suspected fault, have a scripted scenario built to confirm or refute it, then have the fix written and the regression re-run. |
| **Code review or critique** | **Yes** | Asked for a review of the whole project "from a grading point of view", which produced the list of remaining gaps; and asked repeatedly to predict what could go wrong at each stage and test against those predictions. |
| **Writing or editing the report itself** | **Yes** | Sections 1–7 were AI-drafted from the work done. **Section 8 was not** — the requirements prohibit it. |

### 6.3 Transcripts

Full conversation transcripts are exported as PDFs, compressed into a single ZIP, and uploaded to
the Phase 1 report Dropbox on LEARN, as required.

### 6.4 Reflection

> _Drafted from what happened — rewrite in your own voice and correct anything that does not match
> your experience._

AI assistance was most valuable in two places. The first was reading the course chapters before
each pattern was written rather than after. Chapter 17's MVC is not the arrangement most people
assume — the View owns the Controller and queries the Model directly — and writing the code first
would have produced a plausible-looking MVC that a reviewer following the chapter would mark down.
The second was building test scenarios: several rules here cannot be checked by playing, and having
harnesses written quickly made a statistical check of the probability tables practical.

The advice was not always right. The plan asserted that a shuffled Enchantment deck would need
Command objects, one class per card. Building it showed the seam between the two implementations
was a single function returning an `int`, so the deck is a container of values and a class per card
would have bought nothing; Design Question 2 now says so, grounded in code rather than prediction.
Earlier, it produced a UML diagram whose class boxes had empty attribute and operation compartments
and defended that as an "abstraction level" — it was simply an unfinished diagram, and only became
a defensible choice once the rendering was fixed.

I also overrode it directly. Reviewing the implementation, I found five defects it had introduced
and reported them: a Lockup fine that stranded a player after they raised funds, a mortgaged
district transfer that drove Reagents negative, a save loader that accepted unowned-but-mortgaged
districts, command-line parsing that deviated from the fixed contract, and `roll 1 2` outside
`-testing` silently becoming a random roll. Each was real; each was then fixed and regression
tested.

---

## 7. Extra Features Implemented

### Shuffled Enchantment Deck

**Feature name and description.** The spec defines the Enchantment squares as a probability table
and lists a real deck as an enhancement. With the enhancement enabled, effects are drawn from a
genuine deck of eighteen cards — one per Table 3 weight unit — dealt one at a time and reshuffled
only when exhausted. A player interacts with it exactly as with the base game, by landing on an
Enchantment Deck square; the difference is that the odds shift as cards are consumed, so a full
pass deals Table 3 exactly and counting cards becomes a real strategy.

**Design — how it integrates with the existing class structure.** `CardDeck` (`cards.h`/`.cpp`)
holds the shuffled `vector<int>` and exposes `draw()`, refilling and reshuffling when spent. `Game`
holds a `unique_ptr<CardDeck>` that is null unless the enhancement is on, and both implementations
meet behind one seam, `Game::drawEnchantmentValue()`. `EnchantmentDeck::landOn` calls that seam and
is identical either way, so no district code knows which mode is active. The choice is made at
startup, satisfying the requirement that the game plays with and without enhancements without
recompiling.

**How to trigger it during the demo.**

```
./arcanemarket -deck                            # new game, deck enabled
./arcanemarket -load save.txt -testing -deck    # loaded game, deterministic dice, deck enabled
```

On startup the program prints `Enchantment effects come from a shuffled deck of 18 cards`,
confirming the mode is active. Then land any Artificer on an Enchantment Deck square — squares 7,
19, 26 or 32; with `-testing`, for example `roll 0 7` from Grand Entrance. Running the same
scenario **without** `-deck` uses the spec's probability table instead, demonstrating that both
modes work from the same binary.

---

## 8. Lessons Learned

> **This section must be written by you, without AI tools.** The requirements state so explicitly
> and in bold, so it has deliberately been left blank.
>
> The requirements suggest addressing at least the following. You need not answer every one — pick
> those you have something real to say about.
>
> - Which parts of the project did you use AI for, and which did you deliberately *not* use it
>   for, and why?
> - Was there a moment AI gave you something that looked good at first but turned out to be wrong,
>   generic, or broken once you dug in? What tipped you off?
> - Did AI ever push your design in a direction you wouldn't have chosen on your own, for better
>   or worse?
> - When AI suggested something, how did you decide whether to use it as-is, modify it, or reject
>   it? Can you give a specific example?
> - If a teammate or instructor asked "why did you make this design choice?", would you be able to
>   answer for every AI-assisted part of the game?
> - Is there a skill you think you didn't build because AI did that part for you? Does that matter
>   to you?
> - Is there something you understand better because you used AI — because it explained a concept,
>   or because its first attempt was wrong and figuring out why taught you something?
> - If you had to redo this project with no AI access at all, what would you do differently, and
>   what would simply not get done?
> - Did using AI change how much you iterated?
> - What parts of using LLMs for coding could be improved to make them more helpful? What
>   challenges did you encounter, or features do you wish these tools had?
