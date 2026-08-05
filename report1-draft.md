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
occurrence of a problem; the first occurrence is just code.**

Four patterns are applied independently — Template Method, State, Command and the Simple Factory
idiom — plus one architectural **compound**, MVC, whose own constituents (Observer and Strategy)
are reported inside it rather than counted separately. Each entry gives the pattern name, where it
is used, why it was chosen with the alternatives rejected, and how it is implemented. §3.6 covers
a separation that is deliberately *not* given a pattern name, and §3.7 the patterns rejected
outright.

Throughout, the two metrics used to judge a decision are **coupling** — the degree of
interdependence between modules, which the design should push down the ladder from content
towards data coupling — and **cohesion**, the degree to which the elements inside a module belong
together, where functional cohesion is the ideal and a "god class" of vaguely related code is the
failure mode.

### 3.1 Model-View-Controller, with Observer and Strategy

*Architectural compound pattern. Core problem: separate model, view, and controller concerns.
Primary principles: SRP, OCP.*

Chapter 17 defines MVC as a compound: Observer links the Model to the View, and Strategy links the
View to its Controller. Both appear in this program **only** as parts of that compound — `Subject`
has exactly one observer and `ControllerInterface` exactly one implementation — so they are
reported here as constituents rather than claimed as two further patterns. Counting them
separately would inflate the tally without adding a second, independent justification.

**Where it is used.** `Game` is the Model (`game.h`/`.cpp`); `TextView` is the View
(`textview.h`/`.cpp`); `ControllerInterface` and `GameController` are the Controller
(`controller.h`/`.cpp`); `Subject` and `Observer` are in `observer.h`. See Figure 2.

**Why it was chosen.** Not because a graphical display might be added later — that would be
premature abstraction, which is as harmful as premature optimization, and the enhancement was
never built. The justification is the Single Responsibility Principle: MVC separates display logic
from business logic from user input, and this program already contains all three as distinct
**axes of variation**:

1. **Rendering** — `textview.cpp` is 200 lines of grid geometry, cell-width arithmetic, name
   wrapping and token placement. It changes when the display changes.
2. **Interpretation** — a parser and thirteen command classes. It changes when the command
   language changes.
3. **Rules** — tolls, auctions, insolvency. It changes when the game rules change.

Collapsing them gives one class with three reasons to change, which is the definition of an SRP
violation and produces exactly the low cohesion the course calls a god class. Kept apart, each has
**functional cohesion**: every element contributes to one well-defined task. That the split is
real and not nominal is demonstrated rather than asserted — `tests/gametest.cpp`, run with
`make test`, constructs a `Game` over string streams, drives it and asserts on its state with no
view attached at all, 56 checks running headless.

*Alternatives rejected:* **one class holding rules and rendering** — three reasons to change,
god-class cohesion, and headless testing impossible; **Model and View with no Controller** —
interpretation would migrate into the display class, coupling the command language to the
rendering code; **Controller mediating all data**, with the View asking the Controller for state —
Chapter 17 states this "is not MVC", and it adds a hop that raises coupling without raising
cohesion, since the View can query the Model directly.

**How it is implemented.** `Game` holds all state and names no View. `TextView` holds a `Game&`
and a `shared_ptr<ControllerInterface>`, and on notification re-queries the Model and redraws.
Input reaches `TextView::handleInput`, which forwards to the Controller; `GameController` mutates
the Model, then signals once for the whole command. This is the composition the course names: *the
Model is an observable subject; the View subscribes as an observer; the Controller mediates.*

#### Observer, the Model→View link

*Behavioural. Core problem: one-to-many notification without tight coupling.*

`Subject` keeps a `vector<Observer*>` of non-owning pointers with `addObserver`
(duplicate-checked via `std::find`), `removeObserver` (erase-remove) and `notifyObservers`.
`Game` extends `Subject`; `TextView` extends `Observer`, registering in its constructor and
deregistering in its destructor — RAII applied to a registration rather than to memory.
Notification fires from `GameController::handleCommand`, once per command rather than once per
mutator, because the spec's unit of redraw is the command: one command may move an Artificer, pay a toll, transfer a district and
change the turn phase, and the board is drawn once for all of it.

**Where this claim is weak, stated plainly.** Observer's core problem is *one-to-many*
notification, and this program is one-to-one: a single observer, and one call site where
`notifyObservers()` iterates a one-element vector. Nor is the trigger a Model state change — no
`Game` mutator calls `notifyObservers()`, so the Controller is signalling the Subject to signal the
Observer. By the second-occurrence rule this is the first occurrence, and replacing it with a
direct `view.render()` from the command loop would be behaviourally identical and shorter. Observer
is present because Chapter 17 defines MVC as Observer plus Strategy — a reason of taxonomy rather
than of need. What pays for itself is the separation the notification sits inside: the Model names
no View, which is what allows `gametest` to drive it headless. A reviewer who judged the
notification machinery over-built on its own terms would be right on the principle.

#### Strategy, the View→Controller link

*Behavioural. Core problem: the algorithm varies independently of the class that uses it.*

`TextView` holds its Controller as a `shared_ptr<ControllerInterface>` and delegates every user
event to it, which is the shape Chapter 17 describes. It is reported as **structural only**:
unlike Observer, no machinery implements it — there is no Strategy class, only the abstract
interface and the single `GameController`. With exactly one implementation nothing demonstrates
swappability, and YAGNI names that case directly: a Strategy hierarchy for a problem with exactly
one strategy adds complexity without adding value. A second controller is also unlikely to earn
its place later, since mode-specific command legality is State's responsibility here.

**Where the Model's dependency on I/O stops.** `Game` and `TextView` take their `std::istream&`
and `std::ostream&` by constructor injection, so neither names a console: `main` is the only file
in the program that mentions `std::cin` or `std::cout`. This is Dependency Inversion at the
boundary the Model cannot avoid — the spec obliges the Model to conduct mid-turn dialogues, such
as the purchase offer, the auction bid and the Guild Levy choice — and it is the seam that lets
`tests/gametest.cpp` drive a complete game through string streams.

**An honest limitation.** What that separates is the *destination* of output, not its
*authorship*. `Game` still composes its own narration — "Marco pays 26 Reagents to Vera" — and its
mid-turn dialogues bypass the command pipeline. A graphical View would receive that text and have
to decide what to do with it. This is the weakest point of the separation and the fair thing to be
challenged on at the demo.

### 3.2 Template Method

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

### 3.3 State

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

### 3.4 Command

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

### 3.5 Simple Factory

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

### 3.6 A separation that is deliberately not a pattern

`Auction` (`auction.h`/`.cpp`) runs the bidding when a purchase is declined or Treasury insolvency
releases districts. It is a separate class from `Game`, and it is worth reporting here *because*
no pattern is claimed for it.

The bidding loop is an interactive dialogue with its own lifecycle — who is still in, what the
current bid is, whose turn it is to answer — and it changes when the auction rules change, which
is a different axis of variation from the rest of the Model. Holding it inside `Game` would give `Game`
one more reason to change and diluted its cohesion; extracting it restores functional cohesion on
both sides.

That is a Single Responsibility argument, and it needs no pattern to be valid. Patterns are a
vocabulary for recurring structural problems, not a prerequisite for splitting a class — reaching
for a pattern name here (Mediator, say, or a Strategy over bidding rules) would have been the
name-dropping the course warns against, since there is exactly one auction procedure and no second
occurrence to generalise from. An ordinary class with a clear responsibility is the right answer.

---

### 3.7 Patterns considered and deliberately not used

- **Decorator** — core problem: add responsibilities to objects dynamically without subclassing.
  Rejected for upgrades; the reasoning is Design Question 3.
- **Iterator** — core problem: sequential access without exposing internal structure. The board is
  a `vector` indexed directly by square number and movement is arithmetic modulo 40, so there is
  no internal structure to hide and no traversal to abstract.

## 4. Answers to Design Questions

### Design Question 1 — Observer Pattern for the Circuit Board

> *After reading the Districts subsection of the specification, would the Observer Pattern be a
> good pattern to use when implementing the Circuit Board? Why or why not?*

The Observer Pattern itself would not be a good pattern to use when implementing the Circuit
Board.

The pattern tells objects that something has changed, so that a display never shows stale data.
That is a problem when a display has to react to changes it cannot predict. However, the board is
redrawn after every command, so there is nothing for the Model to announce.

If Observer was used the way the question suggests, the Subject would be Game and the Observer
would be TextView. The notifications would be triggered by the mutating operations inside Game,
and that does not fit this game: a single roll can move an Artificer, charge a toll and change the
turn phase, so notifying once per mutation would redraw the board three times in the middle of one
command.

Direct method calls are better suited to these requirements. Calling view.render() from the main
loop after each line of input would produce exactly the same output, with one less level of
indirection and no Subject or Observer machinery to maintain.

Observer would become the better choice if the requirements were different. If there were more
than one view, or if only the squares that changed were repainted, or if the Model could change
outside the command loop, then the Model would need to announce its changes.

A discrepancy in my answer however is that I implement MVC and think it to be the optimal pattern
as stated in section 3.1. MVC is defined as Observer + Strategy. Regardless, the combined pattern
pays off due to the separation around it: Game never names a view class, which is what lets the
test harness construct a Game over string streams and run the rules with no view attached at all.

### Design Question 2 — Card-Deck Mechanics for Wandering Market and Enchantment Deck

> *Suppose we wanted to model the Wandering Market and Enchantment Deck more closely after
> card-deck mechanics, with a shuffled deck drawn one card at a time and reshuffled when
> exhausted. Is there a suitable design pattern you could use? How would you structure it?*

Yes, there is a suitable pattern, and the deck itself is implemented as an optional enhancement
enabled with -deck.

A deck is a different mechanic from what the spec asks for rather than a reskin of it. The spec
defines both squares as probability tables, and a table is memoryless, so -200 can come up twice
in a row. A deck is stateful: each card leaves the deck when it is drawn, which shifts the odds on
the cards still in it and makes counting them a real strategy. Over one full pass a deck deals
Table 3 exactly.

For the deck I actually built, no pattern is needed. Every Enchantment effect in the spec is only
a change in Reagents, so the deck is a vector of eighteen integers, one per weight unit in Table
3, dealt from the front and reshuffled when it runs out. Wrapping seven integers in seven classes
would be notation rather than design. The table and the deck meet behind one function,
Game::drawEnchantmentValue(), so the square itself does not know which of the two it is using.

The pattern becomes worth having once the cards stop being interchangeable. If cards had different
kinds of effects, such as "advance to the Grand Entrance", "receive a Free-Pass Token" or "pay
every other Artificer 50", then Command is the pattern I would use. Each card becomes an object
with one apply(Game&, Player&) operation that encapsulates its own effect, and the deck becomes a
vector of unique_ptr<EnchantmentCard>. Adding a kind of card is then one new class and one line
where the deck is built, and the drawing and reshuffling code does not change at all.

CardDeck could be refactored into that by changing its element type alone, since nothing outside
it depends on the element being an int. I did not do it because, with only Reagent deltas in the
spec, it would be a class hierarchy with exactly one real behaviour in it.

### Design Question 3 — Decorator Pattern for Upgrades

> *Is the Decorator Pattern a good pattern to use when implementing Upgrades on Workshop
> districts? Why or why not? Consider how Toll Fees are calculated with and without upgrades, and
> how selling upgrades affects the object structure.*

No, the Decorator Pattern is not a good pattern to use for Upgrades.

Decorator would model an upgraded Workshop as a chain: a base Workshop wrapped by one decorator
per upgrade, with each computeToll calling the one it wraps and adjusting the answer. That only
works if each upgrade contributes something composable.

Upgrades do not. Tinder's tolls run 2, 10, 30, 90, 160, 250, which is neither adding a fixed
amount nor multiplying by a fixed factor. Each decorator would have to know the absolute toll for
its own level and throw away the value it got from the object underneath it, so the chain would
exist only to record how many layers deep it was. A single integer records that better.

Selling an upgrade is where it breaks structurally. An Artificer may sell one level at a time,
which means taking a decorator back out of the chain, and Decorator has no protocol for removing a
wrapper. The district would have to be unwrapped and rebuilt, and the rebuilt district is a
different object. Ownership in this game is recorded by pointer, so every Player and the Board
would be left pointing at the old wrapper, and tolls, mortgaging and the save file all read
ownership that way. That makes it decisive rather than merely awkward.

What is implemented instead is an int upgrades_ on Workshop that indexes a six-entry toll table.
computeToll is one array lookup, buying is +1, selling is -1, and the object's identity never
changes. Decorator earns its place on open-ended composition, and upgrades stop at five.

The one thing a chain could not express anyway is the doubled toll on an unimproved Workshop in a
completed Guild. That is not an upgrade, and it depends on who owns the rest of the Guild rather
than on the district itself, so it is applied as a rule when the toll is computed.


---

## 5. Challenges & How You Addressed Them

These are ordered by how much they changed the project, not by when they happened.

### 5.1 Auditing the design against the course's principles

**The challenge.** The game worked, but working is not the same as being well designed. I went
back over the code against the course's summary of its own principles and found four structural
faults. None of them would have been caught by any test.

- `Game` wrote to `std::cout` in 83 places. Every public commitment constrains future change, and
  those were an undeclared commitment to a console: no other View could reuse the Model without
  dragging them along.
- The auction's bidding loop sat inside `Game`, which gave the Model a second reason to change.
- Eleven `dynamic_cast`s and eight duplicated board sweeps each worked out again which squares
  were ownable. That is dispatch on a type tag, which is the canonical OCP violation, and it is a
  DRY failure as well, because one piece of knowledge had eight representations.
- `game_.turn().roll()` appeared throughout, which is the `a.getB().getC()` chain the Principle of
  Least Knowledge warns about.

**Approaches considered.** The first option was to leave the code alone and write the gaps up in
this report, which is what had been happening: the console problem was recorded as an honest
limitation and defended on the grounds that Chapter 16's own `GumballMachine` prints from the
Model. That defends a violation instead of removing one. A full rewrite was too risky with the
deadline close. What I did instead was a targeted pass, one principle at a time, re-running the
whole test suite after each.

**Resolution.**

1. The console left the Model. `Game` and `TextView` take `std::istream&` and `std::ostream&` by
   constructor injection, which is the shape the summary prescribes: the high-level logic depends
   on an abstraction and the concrete implementation is injected through constructor parameters.
   `main.cpp` is now the only file that names `std::cin` or `std::cout`, and `game.cpp` names
   neither. This is also what made the headless test harnesses possible, since they need to
   capture the Model's output rather than have it disappear into a terminal.
2. The auction became its own class. `Auction` owns the bidding dialogue and `Game` keeps the
   rules, so each has one reason to change. This is described in section 3.6.
3. Type dispatch left the sweeps. `Board` classifies its districts once at construction into a
   registry held in circuit order, and every ownership sweep — total worth, transfers, insolvency,
   save and load, assets — now iterates `board.ownables()`. Two new virtual hooks, `buyPrice` on
   the base and `upgradeInvestment` on ownables, removed the rest, which is the summary's
   prescribed cure of replacing a type-tag test with virtual dispatch. Eleven downcasts became
   one, in `board.cpp` at the point of classification, with its reason written beside it.
4. The chains became forwarders. `Game` no longer exposes its `Turn`; callers use `game_.roll()`
   and the states use `turn_.out()` and `turn_.endTurn()`. Formatting an Artificer's assets moved
   out of the Model and into `AssetsCommand` and `AllCommand`, which build their output from
   `Game::districtsOwnedBy()`.

The result is measurable rather than asserted: 83 console writes in the Model down to none, eleven
downcasts down to one, and a `Game` that no longer reaches through its collaborators. What makes
this worth recording is that I found all four by reading the code against a checklist rather than
by running it. No test failed before the audit, and none failed after it.

### 5.2 One debt slot silently forgave obligations

**The challenge.** I was fixing a smaller bug — transferring a mortgaged district subtracted its
10% fee without checking whether the receiver could pay, which took one Artificer to −22 Reagents
— and it exposed something larger. The whole debt system was three fields on `Game`, so a second
debt overwrote the first. That is normally safe, because a turn cannot end while the current
Artificer owes anything. But insolvency charges the mortgage fees to the creditor, who is not the
Artificer taking the turn, so that one obligation outlives the turn that created it. I reproduced
it: a creditor's 26-Reagent debt was erased by an unrelated one, which left them richer than they
should have been and the Treasury unpaid, with nothing printed to say so.

**Approaches considered.** Leaving it and documenting the limitation; settling the creditor's fee
immediately with a raise-funds loop in the middle of somebody else's turn; or tracking debts per
Artificer.

**Resolution.** A `map<Player*, Debt>`, together with a guard that an Artificer who owes anything
may not roll. The guard is what makes one debt per Artificer enough: without it, someone could
begin a turn already owing the Treasury, roll into a toll and end up owing two creditors at once,
which is a situation insolvency has no answer for. It is also closer to the spec, which resolves a
shortfall on the spot rather than letting it accumulate.

### 5.3 A rule spanning two subsystems left an Artificer stuck forever

**The challenge.** On the third turn in The Lockup the spec makes the fine or a token compulsory.
My code checked whether the Artificer could afford it first and printed "cannot pay", so an
Artificer holding 10 Reagents stayed detained indefinitely. They could not even declare insolvency,
because no debt had ever been registered against them.

**Approaches considered.** Special-casing the fine as its own kind of obligation; releasing the
Artificer for free when they cannot pay; or routing the fine through the debt mechanism that
already existed.

**Resolution.** The third. The fine goes through `payTreasury`, which parks a debt when it cannot
be paid, and the debt then blocks the turn until it is settled or insolvency is declared. Fixing
that exposed a second defect underneath it: settling the debt released nobody and never made the
move the fine had been paid for, so the Artificer would be charged again on the next turn. The
`Debt` record now carries the fact that it came from a Lockup fine along with the roll it owes,
and settling it completes the exit.

### 5.4 Saving and loading brought eliminated Artificers back

**The challenge.** The save format has no field for insolvency, and `save` was writing every
Artificer in the roster. A bankrupt one reloaded as an ordinary player holding 0 Reagents and,
being first in the file, took the first turn. Loading is the spec's top marking priority, so this
one mattered twice over.

**Approaches considered.** Adding an insolvency field, which would break the required format;
keeping eliminated Artificers in the file and filtering them out on load; or leaving them out at
save time.

**Resolution.** Leaving them out at save time, with `numPlayers` written as the number of solvent
Artificers. The format stays exactly as the spec defines it, and an eliminated Artificer no longer
exists in the saved game, which is what being eliminated should mean.

### 5.5 The specification contradicts itself

**The challenge.** The board figure and Table 1 disagree in two places. The figure prices
Veil-watch at 180 where Table 1 says 170, and the figure's legend lists five square numbers for
four Forges, which implies square 14 is a Forge when the save-file ordering puts Westgate Depot
there, between Coal-gate at 13 and Mist-forge at 16. Building from the wrong source would have
corrupted every toll downstream.

**Approaches considered.** Following the figure; following Table 1; or judging each case
separately on which source looked more authoritative.

**Resolution.** I resolved both in favour of Table 1 and the save-file ordering, because those two
agree with each other and with the upgrade costs and the six toll tiers, while the figure does not
agree with any of them. I recorded the decision in my project notes rather than making it
silently, so that I can defend it at the demo.

### 5.6 Testing rules that playing cannot exercise

**The challenge.** Some of this game cannot be verified by playing it. Tables 2 and 3 are
probability distributions, and a transposed weight produces plausible output forever. A save file
that should be rejected is only ever seen when someone hands the program a broken one. Neither
shows up in a normal game.

**Approaches considered.** Re-reading the tables by eye; asserting on individual draws; or
measuring behaviour in bulk with harnesses that do not need a console.

**Resolution.** Three harnesses, all run by `make test`, plus compile-time checks. The weights are
asserted to sum to 24 and 18 at compile time, so a mistyped entry fails the build.
`tests/tabletest.cpp` draws 200,000 outcomes from each table and compares the observed frequencies
against the spec within a tolerance, then verifies the shuffled deck's defining property, which is
that 100 consecutive passes each deal Table 3 exactly. `tests/gametest.cpp` constructs a `Game`
over string streams with no view attached at all and asserts on the Model's rules directly — the
four toll formulas, mortgages, the Grand Entrance bonus, parked debts, insolvency and a
byte-identical save and load round trip — in 56 checks. `tests/systemtest.cpp` goes in through the
front door instead, building a `GameController` and feeding it command lines, which covers the
parser, the turn state machine, the three Lockup exits, trading, upgrades, the auction, both
insolvency destinations and twenty-seven malformed save files that must each be rejected with a
reason and with the running game left untouched, in a further 226 checks.

### 5.7 Telling a forward lap from a backward move

**The challenge.** `passedGrandEntrance(from, to)` tested whether the destination was lower than
the origin, which is true of any backward move. A Wandering Market sending an Artificer from
square 5 back to square 2 therefore paid them the 200-Reagent bonus.

**Approaches considered.** Comparing the destination against the origin, which is the broken
version; tracking a lap count on `Player`; or passing the direction of travel alongside the
destination.

**Resolution.** The destination on its own cannot carry this information, so the function takes
the step count instead and pays only when the steps are positive. I later changed it from a
boolean to a count of laps, because `-testing` accepts dice totals large enough to cross the Grand
Entrance twice in a single move.

### 5.8 Silent failure on malformed input

**The challenge.** `roll 1 2` outside `-testing` rolled random dice and ignored the arguments, and
so did `roll 1`, `roll abc` and `roll 4x 2` inside testing mode. Doing something other than what
was typed, without saying so, is worse than refusing to do anything.

**Approaches considered.** Letting the parser print diagnostics directly; returning `nullptr` and
accepting the generic "I don't understand"; or introducing a command whose job is to report a
refusal.

**Resolution.** The third. `MessageCommand` carries a specific diagnostic, so the parser itself
stays free of console output while the user still gets a precise message. A related fix belongs
here: an unrecognised command-line argument used to abort the program outright, which is the wrong
failure mode to have at a demo, so unknown arguments now warn and the game continues.

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
