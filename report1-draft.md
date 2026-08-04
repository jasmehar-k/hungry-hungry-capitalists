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

Patterns are a vocabulary, not a checklist: the goal is not to use as many as possible, which
produces overengineered code, but to recognise a recurring structural problem and apply the proven
solution. The course's standard is that **a pattern is justified when you see the second
occurrence of a problem; the first occurrence is just code.** Six patterns are applied below, each
given as pattern name, where it is used, why it was chosen (with the alternatives rejected), and
how it is implemented. Two are held to that standard and found wanting; §3.7 says so rather than
claiming them.

Throughout, the two metrics used to judge a decision are **coupling** — the degree of
interdependence between modules, which the design should push down the ladder from content
towards data coupling — and **cohesion**, the degree to which the elements inside a module belong
together, where functional cohesion is the ideal and a "god class" of vaguely related code is the
failure mode.

### 3.1 Model-View-Controller

*Architectural compound pattern. Core problem: separate model, view, and controller concerns.
Primary principles: SRP, OCP.*

**Where it is used.** `Game` is the Model (`game.h`/`.cpp`); `TextView` is the View
(`textview.h`/`.cpp`); `ControllerInterface` and `GameController` are the Controller
(`controller.h`/`.cpp`). See Figure 2.

**Why it was chosen.** Not because a graphical display might be added later — that would be
premature abstraction, which is as harmful as premature optimization, and the enhancement was
never built. The justification is the Single Responsibility Principle: MVC separates display
logic from business logic from user input, and this program already contains all three as
distinct **axes of variation**:

1. **Rendering** — `textview.cpp` is 200 lines of grid geometry, cell-width arithmetic, name
   wrapping and token placement. It changes when the display changes.
2. **Interpretation** — a parser and thirteen command classes. It changes when the command
   language changes.
3. **Rules** — tolls, auctions, insolvency. It changes when the game rules change.

Collapsing them gives one class with three reasons to change, which is the definition of an SRP
violation and produces exactly the low cohesion the course calls a god class. Kept apart, each
has **functional cohesion**: every element contributes to one well-defined task. That the split
is real and not nominal is demonstrated rather than asserted — the test harnesses construct a
`Game`, drive it and assert on its state with no view attached at all, 78 checks running headless.

*Alternatives rejected:* **one class holding rules and rendering** — three reasons to change,
god-class cohesion, and headless testing impossible; **Model and View with no Controller** —
interpretation would migrate into the display class, coupling the command language to the
rendering code; **Controller mediating all data**, with the View asking the Controller for state —
Chapter 17 states this "is not MVC", and it adds a hop that raises coupling without raising
cohesion, since the View can query the Model directly.

**How it is implemented.** `Game` holds all state, extends `Subject`, and names no View.
`TextView` holds a `Game&` and a `shared_ptr<ControllerInterface>`, registers as an Observer, and
on notification re-queries the Model and redraws. Input reaches `TextView::handleInput`, which
forwards to the Controller; `GameController` mutates the Model, then fires one notification for
the whole command. This is the standard composition the course names: *the Model is an observable
subject; the View subscribes as an observer; the Controller mediates.*

**An honest limitation.** What is separated is *rendering* from rules, not *all output* from
rules. `Game` still writes narration straight to `std::cout` at 83 call sites — "Marco pays 26
Reagents to Vera", the auction and Guild Levy prompts. Encapsulation says every public commitment
constrains future change, and those `cout` calls are an undeclared commitment to a console: a
graphical View could not reuse `Game` without them following it. A stricter MVC would route them
through the View. This is the weakest point of the separation and the fair thing to be challenged
on at the demo.

### 3.2 Observer

*Behavioural. Core problem: one-to-many notification without tight coupling. Primary principles:
OCP, SRP.*

**Where it is used.** `Subject` and `Observer` in `observer.h`; `Game : Subject`;
`TextView : Observer`. See Figure 2.

**Why it was chosen — and why that choice is contestable.** The core problem Observer solves is
*one-to-many* notification. This program has **one-to-one**: exactly one Observer, one Subject,
and a single call site in `GameController::handleCommand` where `notifyObservers()` iterates a
one-element vector. Measured against the course's own rule — a pattern is justified at the second
occurrence of a problem, the first is just code — this is the first occurrence, and a direct
`view.render()` call at that same site would be behaviourally identical and simpler. YAGNI names
this failure mode precisely: an interface with unused implementations adds complexity without
adding value. A reviewer applying that strictly would call it over-built and would be right on
the principle.

What is offered against that is narrower, and is optionality rather than present value:

- Observer is the pattern that replaces direct method calls with event notification, moving the
  Model→View link down the coupling ladder. Because `Game` names no View, adding a second display
  is an OCP extension — a new subclass of `Observer` — rather than a modification of tested Model
  code, and the specification's own enhancement list names a graphical display, so this is a
  documented extension point rather than an invented one.
- The cost is 45 lines in `observer.h`, one virtual call per command, and no allocation.

*Alternatives rejected:* **`Game` holding a `TextView*` and calling `render()` directly** — the
honest competitor; its only real defect is that a high-level module (the Model) would then depend
on a low-level concrete detail, inverting DIP; **the main loop polling `view.render()`
unconditionally** — workable, but it moves the redraw decision outside both Model and Controller.

**How it is implemented.** `Subject` keeps a `vector<Observer*>` of non-owning pointers with
`addObserver` (duplicate-checked via `std::find`), `removeObserver` (erase-remove) and
`notifyObservers`. `TextView` registers in its constructor and deregisters in its destructor —
RAII applied to a registration rather than to memory. Notification fires once per command rather
than per mutator, a change made after per-mutator notification printed six boards during
six-player setup.

### 3.3 Template Method

*Behavioural. Core problem: define the skeleton of an algorithm; let subclasses fill in steps.
Primary principles: OCP, DRY.*

**Where it is used.** `OwnableDistrict::landOn`, declared `final`, with the pure virtual hook
`computeToll`, overridden by `Workshop`, `Forge` and `Depot`. See Figure 3.

**Why it was chosen.** Every visit to an ownable district follows one invariant sequence —
unowned, so offer it; owned by the visitor, so nothing; mortgaged, so nothing; otherwise charge —
while the amount is computed three genuinely different ways. This is the DRY problem the pattern
exists for: extract the invariant skeleton of an algorithm into the base class and leave only the
variant steps to subclasses. Written out three times, the sequence would have three authoritative
representations, and a rule change would need three edits with one eventually missed.

*Alternatives rejected:* **a single `landOn` with a `switch` on `DistrictKind`** — the canonical
OCP violation, an if-else chain dispatching on a type tag; every new district kind edits that
function, whereas with virtual dispatch the switch disappears because the vtable handles it;
**Strategy, injecting a `TollPolicy` object** — Strategy and Template Method both isolate a
varying step, but Strategy's intent is *external* algorithm selection, and here the pricing rule
is fixed by the district's type and never selected at runtime. A hierarchy for something with
exactly one policy per type adds indirection without adding value.

**How it is implemented.** `landOn` is `final`, so no subclass can reorder or skip a step of the
sequence — the invariant is enforced by the language, not by convention — and `computeToll` is a
protected pure virtual, the single point of variation. `Workshop` indexes a six-entry toll table
by upgrade level, doubling an unimproved toll when the Guild is complete; `Forge` indexes a levy
table by Forges owned; `Depot` multiplies the dice total by 4, or 10 when one Artificer holds
both.

### 3.4 State

*Behavioural. Core problem: an object changes behaviour when its internal state changes. Primary
principles: OCP, SRP.*

**Where it is used.** `Turn` is the Context (`turn.h`/`.cpp`); `TurnState` is the state interface,
with `PreRollState`, `PostRollState`, `LockupState` and `AwaitingTradeResponseState`. Figures 4–5.

**Why it was chosen.** Which commands are legal depends on the phase of the turn, and there are
four phases with four different answers — the second, third and fourth occurrences of the problem,
so the pattern is justified on the course's own criterion rather than on anticipation. `roll` is
legal only when a roll is pending; `next` only when it is not; rolling in The Lockup runs an exit
procedure instead of moving; while a trade offer stands only `accept`/`reject` apply.

*Alternatives rejected:* **an `enum TurnPhase` with if-else chains in each command** — again the
canonical OCP violation, dispatch on a type tag; every new phase edits every command, and the
phase logic loses cohesion by scattering across the command classes; **a set of booleans**
(`hasRolled`, `isDetained`, `awaitingTrade`) — worse still, because they admit contradictory
combinations that no rule permits and nothing enforces which transitions are legal.

**How it is implemented.** `Turn` owns all four states as `unique_ptr` members constructed once
and selects the active one through a raw non-owning `state_` pointer, so a transition allocates
nothing. Each concrete state holds a `Turn&` and drives its own transitions via `setState`. `Turn`
also records `resume_`, the phase a trade offer interrupted, because an offer may be made before
*or* after rolling. State and Strategy share this structure — a pointer to an interface that
changes which code runs — and differ in intent: here the transitions are internal to the object,
not selected by a client.

### 3.5 Command

*Behavioural. Core problem: encapsulate a request as an object so it can be stored, queued,
logged, and undone. Primary principles: OCP, SRP.*

**Where it is used.** The `Command` interface and thirteen concrete commands in
`command.h`/`.cpp`; `CommandParser`; `GameController` as Invoker; `Game` as Receiver. Figures 1A
and 6.

**Why it was chosen.** Eleven spec commands with different arities, from bare `next` to
`trade <name> <give> <receive>` — thirteen occurrences of one structural problem, so the pattern
is well past the second-occurrence threshold. Encapsulating each request as an object separates
*what was asked for* from *who asked* and *who performs it*, and the Invoker depends only on the
abstraction, which is DIP at the point of dispatch.

*Alternatives rejected:* **an if-else chain on the verb inside the controller** — dispatch on a
type tag once more, mixing interpretation with execution and giving the controller a second reason
to change; **a `map<string, function<void(istream&)>>`** — the closest competitor, but arguments
would be re-parsed inside each handler or captured untyped, so the request would not be a
self-contained object and the invoker would be stamp-coupled to a stream it does not need.

**How it is implemented.** Each concrete command stores its receiver (`Game&`) and its parsed
arguments as data members, so the request is complete before it is executed. `CommandParser::parse`
builds the right command and returns `nullptr` for anything unrecognised — the single path that
satisfies the spec's requirement to handle misspelled commands gracefully. `GameController` fires
`execute()` through the interface without knowing which command it holds. `undo()` is deliberately
absent: the pattern's canonical form supports undo, but the spec requires none, and declaring an
operation that all thirteen commands would stub out is exactly the unused interface surface that
YAGNI and ISP both warn against.

### 3.6 Simple Factory

*Creational idiom, not the GoF Factory Method. Primary principle: DIP.*

**Where it is used.** `DistrictFactory::create` (`districtfactory.*`), called from `Board`'s
constructor; the same idiom in `CommandParser::parse`. See Figure 7.

**Why it was chosen.** `Board` must build 40 districts of twelve kinds without concrete class
names spreading through it. Localising construction in one place keeps `Board` depending on the
`District` abstraction rather than on the details, and gives `Board` a single reason to change.

*Alternatives rejected:* **a `switch` inside `Board`'s constructor** — every concrete district
name enters `Board`, mixing construction with circuit management and giving it a second axis of
variation; **GoF Factory Method with an abstract creator hierarchy** — whose core problem is
"subclasses decide which class to instantiate". There are no subclasses of the creator here and no
prospect of any, so building the hierarchy would satisfy the pattern's shape without its purpose:
a factory that can produce products no one has asked for.

**How it is implemented.** One static `create` takes a `DistrictSpec` from the board table,
switches on its `kind`, and returns `unique_ptr<District>`. The switch is not an OCP violation in
the usual sense: it exists precisely so that it is the *only* place concrete names appear, which
is the acknowledged cost of the idiom. It is named the Simple Factory idiom rather than Factory
Method because there is no creator hierarchy — the structural difference between the two.

### 3.7 Patterns considered and deliberately not used

- **Decorator** — core problem: add responsibilities to objects dynamically without subclassing.
  Rejected for upgrades; the reasoning is Design Question 3.
- **Iterator** — core problem: sequential access without exposing internal structure. The board is
  a `vector` indexed directly by square number and movement is arithmetic modulo 40, so there is
  no internal structure to hide and no traversal to abstract.
- **Strategy** as a separate claim — Chapter 17 describes the Controller as the View's Strategy,
  and `TextView` does hold a `shared_ptr<ControllerInterface>`. But there is exactly one concrete
  controller, and YAGNI names this case explicitly: a Strategy hierarchy for a problem that has
  exactly one strategy adds complexity without adding value. It is reported as part of MVC's
  compound structure rather than claimed as a pattern in its own right.

---

## 4. Answers to Design Questions

### Design Question 1 — Observer Pattern for the Circuit Board

> *After reading the Districts subsection of the specification, would the Observer Pattern be a
> good pattern to use when implementing the Circuit Board? Why or why not?*

Yes — with a caveat the course's own standard forces me to state. **The Subject is `Game`**, the
Model owning the board, the players and the turn; **the Observer is `TextView`**. Districts are
deliberately *not* Subjects: the spec redraws the **entire** board after every command, so
per-district notification would give forty objects an observer list to trigger the same full
redraw.

**The triggering event is the completion of a command**, not an individual field write. This
departs from Chapter 17, which notifies at the end of every mutating method. One command can move
an Artificer, pay a toll, transfer a district and change the turn phase; notifying per mutation
redrew the board four times for one command and printed six boards during six-player setup.
Notification therefore fires once, from `GameController::handleCommand`.

**Whether direct method calls are better suited** is genuinely arguable. Observer's core problem
is *one-to-many* notification without tight coupling; this program has **one-to-one** — one
Observer, one Subject, one call site iterating a one-element vector. A pattern is justified at the
second occurrence of a problem, and this is the first, so a direct `view.render()` would be
behaviourally identical and simpler.

What Observer does buy is that the Model names no View, so a second display would be an OCP
extension rather than a modification of tested code — optionality against an enhancement the spec
itself lists, for 45 lines. What is *not* arguable is the SRP separation underneath: rendering is
200 lines of geometry with no rule in it, and the rules are exercised headless by the test
harness. That split stands whether or not the notification between them goes through Observer.

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
