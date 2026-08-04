# ArcaneMarket — UML class diagrams

Reflects the final submitted code. Rendered images in `diagrams/`.

## How these are organised

**Figure 1 is a master class diagram**: every class in the program, names and relationships
only. It answers "what exists and what talks to what", so it carries no members.

**Figures 2–8 are detailed diagrams**, one per pattern or logical grouping. Each is complete for
its subject — every participating class, with the members that matter at that level of
abstraction. Nothing is dropped for tidiness: the Command figure shows all thirteen commands,
the district figure shows all thirteen districts.

Following Chapter 8, method **parameters are not associations** — `landOn(Game&, Player&)` passes
references at call time, which the chapter explicitly excludes ("the argument that one routine
passes to another … none of that is an association"). Only stored references and pointers appear
as lines.

## Notation (Chapter 8)

| Symbol | Meaning | C++ here |
|---|---|---|
| `+ - #` | public / private / protected | |
| *italics* | pure virtual | `= 0` |
| <u>underline</u> | static | `static` |
| `const` | query operation | `... const` |
| ◁—— | generalization | `class B : public A` |
| ◆—— | composition | by value or `unique_ptr` |
| ◇—— | aggregation | `shared_ptr` |
| ——▶ | association | stored reference or non-owning pointer |
| ⋯▶ | dependency | creates / uses |

Multiplicities appear at both ends of every association, since Chapter 8 notes that a final
diagram should not leave them blank.

---

## Figure 1 — Master class diagram

Every class in the program. Names and relationships only.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction TB

    class Subject { <<abstract>> }
    class Observer { <<abstract>> }
    class Game { <<Model>> }
    class TextView { <<View>> }
    class ControllerInterface { <<interface>> }
    class GameController { <<Controller>> }
    class District { <<abstract>> }
    class OwnableDistrict { <<abstract>> }
    class TurnState { <<interface>> }
    class Command { <<interface>> }
    class SaveGame { <<namespace>> }
    class Setup { <<namespace>> }
    class DistrictSpec { <<struct>> }

    Subject <|-- Game
    Observer <|-- TextView
    ControllerInterface <|-- GameController

    District <|-- OwnableDistrict
    OwnableDistrict <|-- Workshop
    OwnableDistrict <|-- Forge
    OwnableDistrict <|-- Depot
    District <|-- GrandEntrance
    District <|-- Lockup
    District <|-- FreeRest
    District <|-- GoToLockup
    District <|-- GuildLevy
    District <|-- TransitToll
    District <|-- MarketFestival
    District <|-- WanderingMarket
    District <|-- EnchantmentDeck

    TurnState <|-- PreRollState
    TurnState <|-- PostRollState
    TurnState <|-- LockupState
    TurnState <|-- AwaitingTradeResponseState

    Command <|-- RollCommand
    Command <|-- NextCommand
    Command <|-- MortgageCommand
    Command <|-- UnmortgageCommand
    Command <|-- UpgradeCommand
    Command <|-- TradeCommand
    Command <|-- AcceptCommand
    Command <|-- RejectCommand
    Command <|-- InsolventCommand
    Command <|-- AssetsCommand
    Command <|-- AllCommand
    Command <|-- SaveCommand
    Command <|-- MessageCommand

    Game "1" *-- "1" Board : circuit
    Game "1" *-- "2..6" Player : artificers
    Game "1" *-- "1" Turn : turn
    Game "1" *-- "1" Dice : dice
    Game "1" *-- "0..1" CardDeck : enhancement
    Board "1" *-- "40" District : districts
    Turn "1" *-- "4" TurnState : states

    Subject "1" --> "*" Observer : notifies
    TextView "1" o-- "1" ControllerInterface : strategy
    TextView "1" --> "1" Game : observes and queries
    GameController "1" --> "1" Game : mutates
    Turn "1" --> "1" Game : context
    TurnState "4" --> "1" Turn : context
    Command "*" --> "1" Game : receiver
    OwnableDistrict "*" --> "0..1" Player : owner

    GameController ..> CommandParser : uses
    CommandParser ..> Command : creates
    Board ..> DistrictFactory : uses
    DistrictFactory ..> DistrictSpec : reads
    DistrictFactory ..> District : creates
    SaveGame ..> Game : loads and saves
    Setup ..> Game : builds roster
```

---

## Figure 2 — MVC + Observer

Chapter 17 treats MVC as a compound pattern, so it is one figure. What makes this MVC rather
than three arbitrary classes is the direction of every link: the **View owns the Controller** as
its strategy, the **View observes the Model**, the **View queries the Model directly** on
redraw, and the **Model names no View at all**.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction TB

    class Subject {
        <<abstract>>
        +addObserver(Observer*)
        +removeObserver(Observer*)
        +notifyObservers()
        -observers_ : Observer* [*]
    }
    class Observer {
        <<abstract>>
        +update()*
    }
    class Game {
        <<Model>>
        +notifyObservers()
    }
    class TextView {
        <<View>>
        +update()
        +render(ostream&) const
        +handleInput(string)
        -game_ : Game&
        -controller_ : ControllerInterface
    }
    class ControllerInterface {
        <<interface>>
        +handleCommand(string)*
    }
    class GameController {
        +handleCommand(string)
        -game_ : Game&
    }

    Subject <|-- Game
    Observer <|-- TextView
    ControllerInterface <|-- GameController
    Subject "1" --> "*" Observer : notifies
    TextView "1" o-- "1" ControllerInterface : strategy
    TextView "1" --> "1" Game : queries on redraw
    GameController "1" --> "1" Game : mutates
```

`TextView` registers itself as an Observer in its constructor and deregisters in its destructor.
Notification fires once per command, from `GameController::handleCommand`, rather than from every
Model mutator — the spec redraws the whole board after each command, so the command is the real
unit of change.

---

## Figure 3 — District hierarchy and Template Method

All thirteen district classes. The Template Method lives on the ownable branch: `landOn` is
`final` and fixes the sequence of a visit, `computeToll` is the only step that varies. The nine
non-ownable districts override `landOn` directly and take no part in it.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction TB

    class District {
        <<abstract>>
        +name() string const
        +square() int const
        +kind() DistrictKind const*
        +guild() Guild const
        +isOwnable() bool const
        +upgradeCount() int const
        +isMortgaged() bool const
        +landOn(Game&, Player&)*
        -square_ : int
        -name_ : string
    }
    class OwnableDistrict {
        <<abstract>>
        +landOn(Game&, Player&) final
        +owner() Player* const
        +setOwner(Player*)
        +setMortgaged(bool)
        +buyPrice() int const
        #computeToll(Game&, Player&) int const*
        -owner_ : Player* [0..1]
        -mortgaged_ : bool
        -buyPrice_ : int
    }
    class Workshop {
        #computeToll(Game&, Player&) int const
        +setUpgrades(int)
        +upgradeCost() int const
        +tollAt(int) int const
        -guild_ : Guild
        -tolls_ : int[6]
        -upgrades_ : int
    }
    class Forge {
        #computeToll(Game&, Player&) int const
    }
    class Depot {
        #computeToll(Game&, Player&) int const
    }
    class GrandEntrance { +landOn(Game&, Player&) }
    class Lockup { +landOn(Game&, Player&) }
    class FreeRest { +landOn(Game&, Player&) }
    class GoToLockup { +landOn(Game&, Player&) }
    class GuildLevy { +landOn(Game&, Player&) }
    class TransitToll { +landOn(Game&, Player&) }
    class MarketFestival { +landOn(Game&, Player&) }
    class WanderingMarket { +landOn(Game&, Player&) }
    class EnchantmentDeck { +landOn(Game&, Player&) }

    District <|-- OwnableDistrict
    OwnableDistrict <|-- Workshop
    OwnableDistrict <|-- Forge
    OwnableDistrict <|-- Depot
    District <|-- GrandEntrance
    District <|-- Lockup
    District <|-- FreeRest
    District <|-- GoToLockup
    District <|-- GuildLevy
    District <|-- TransitToll
    District <|-- MarketFestival
    District <|-- WanderingMarket
    District <|-- EnchantmentDeck

    OwnableDistrict "*" --> "0..1" Player : owner
```

The three `computeToll` overrides are genuinely different: `Workshop` indexes a six-entry table
by upgrade level and doubles an unimproved toll when the Guild is complete; `Forge` indexes a
levy table by how many Forges the owner holds; `Depot` multiplies the dice total by 4, or 10 when
one Artificer holds both.

---

## Figure 4 — State

The Context **owns every state for its whole life** and swaps a non-owning pointer, rather than
allocating on transition. `resume_` records the phase a trade offer interrupted, because an offer
may be made before *or* after rolling.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction TB

    class Turn {
        <<Context>>
        +roll(int, int)
        +next()
        +beginTurn()
        +setState(TurnState*)
        +preRollState() TurnState* const
        +postRollState() TurnState* const
        +lockupState() TurnState* const
        +enterTradeResponse()
        +resumeAfterTrade()
        +countDouble()
        +consecutiveDoubles() int const
        -game_ : Game&
        -preRoll_ : TurnState
        -postRoll_ : TurnState
        -lockup_ : TurnState
        -awaitingTrade_ : TurnState
        -state_ : TurnState*
        -resume_ : TurnState* [0..1]
        -consecutiveDoubles_ : int
    }
    class TurnState {
        <<interface>>
        +roll(int, int)*
        +next()*
        +name() string_view const*
    }
    class PreRollState {
        +roll(int, int)
        +next()
        +name() string_view const
        -turn_ : Turn&
    }
    class PostRollState {
        +roll(int, int)
        +next()
        +name() string_view const
        -turn_ : Turn&
    }
    class LockupState {
        +roll(int, int)
        +next()
        +name() string_view const
        -turn_ : Turn&
    }
    class AwaitingTradeResponseState {
        +roll(int, int)
        +next()
        +name() string_view const
        -turn_ : Turn&
    }

    TurnState <|-- PreRollState
    TurnState <|-- PostRollState
    TurnState <|-- LockupState
    TurnState <|-- AwaitingTradeResponseState
    Turn "1" *-- "4" TurnState : owns all four
    Turn "1" --> "1" TurnState : state_, the active one
    TurnState "4" --> "1" Turn : context
    Turn "1" --> "1" Game : context
```

---

## Figure 5 — Turn phase transitions

```mermaid
stateDiagram-v2
    [*] --> PreRoll : beginTurn / not detained
    [*] --> Lockup : beginTurn / detained
    PreRoll --> PreRoll : roll doubles
    PreRoll --> PostRoll : roll, or third double
    Lockup --> PostRoll : doubles, fine, token, or stay
    PreRoll --> AwaitingTrade : trade offered
    PostRoll --> AwaitingTrade : trade offered
    AwaitingTrade --> PreRoll : accept or reject
    AwaitingTrade --> PostRoll : accept or reject
    PostRoll --> [*] : next
```

---

## Figure 6 — Command

All thirteen commands. Each binds its receiver and its arguments as data members at
construction, which is what lets the Invoker fire any request through one interface without
knowing what it is.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction TB

    class GameController {
        <<Invoker>>
        +handleCommand(string)
        -game_ : Game&
    }
    class CommandParser {
        +parse(Game&, string) Command*$
    }
    class Command {
        <<interface>>
        +execute()*
    }
    class Game { <<Receiver>> }

    class RollCommand {
        +execute()
        -game_ : Game&
        -die1_ : int
        -die2_ : int
    }
    class NextCommand {
        +execute()
        -game_ : Game&
    }
    class MortgageCommand {
        +execute()
        -game_ : Game&
        -district_ : string
    }
    class UnmortgageCommand {
        +execute()
        -game_ : Game&
        -district_ : string
    }
    class UpgradeCommand {
        +execute()
        -game_ : Game&
        -district_ : string
        -buying_ : bool
    }
    class TradeCommand {
        +execute()
        -game_ : Game&
        -target_ : string
        -give_ : string
        -receive_ : string
    }
    class AcceptCommand {
        +execute()
        -game_ : Game&
    }
    class RejectCommand {
        +execute()
        -game_ : Game&
    }
    class InsolventCommand {
        +execute()
        -game_ : Game&
    }
    class AssetsCommand {
        +execute()
        -game_ : Game&
    }
    class AllCommand {
        +execute()
        -game_ : Game&
    }
    class SaveCommand {
        +execute()
        -game_ : Game&
        -filename_ : string
    }
    class MessageCommand {
        +execute()
        -message_ : string
    }

    Command <|-- RollCommand
    Command <|-- NextCommand
    Command <|-- MortgageCommand
    Command <|-- UnmortgageCommand
    Command <|-- UpgradeCommand
    Command <|-- TradeCommand
    Command <|-- AcceptCommand
    Command <|-- RejectCommand
    Command <|-- InsolventCommand
    Command <|-- AssetsCommand
    Command <|-- AllCommand
    Command <|-- SaveCommand
    Command <|-- MessageCommand

    GameController ..> CommandParser : uses
    CommandParser ..> Command : creates
    GameController "1" --> "0..1" Command : execute()
    Command "*" --> "1" Game : receiver
```

`Command` declares `execute()` only. Chapter 20 adds `undo()` **when undo is required**, and
ArcaneMarket has none. `MessageCommand` is the single command with no receiver: it reports a
refusal, which keeps console output in the Command layer rather than in the parser.

---

## Figure 7 — Simple Factory

One concrete creator with a static operation and **no creator hierarchy**. That absence is the
point: it is what separates the Simple Factory idiom from the GoF Factory Method (Chapter 12).

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction TB

    class DistrictFactory {
        +create(DistrictSpec&) District*$
    }
    class DistrictSpec {
        <<struct>>
        +square : int
        +name : string_view
        +kind : DistrictKind
        +guild : Guild
        +buyPrice : int
        +upgradeCost : int
        +tolls : int[6]
    }
    class District { <<abstract>> }
    class Board {
        +at(int) District&
        -districts_ : District [40]
    }

    District <|-- Workshop
    District <|-- Forge
    District <|-- Depot
    District <|-- OwnableDistrict
    Board ..> DistrictFactory : uses
    DistrictFactory ..> DistrictSpec : reads
    DistrictFactory ..> District : creates
    Board "1" *-- "40" District : districts
```

---

## Figure 8 — Model internals

`Game` is the Model, and none of the pattern figures show what it actually holds. This is the
economy: Reagents, the Treasury, the per-Artificer debt ledger, and the pending trade offer.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction TB

    class Game {
        <<Model>>
        +movePlayer(Player&, int)
        +sendToLockup(Player&)
        +offerLockupExit(Player&) bool
        +forceLockupExit(Player&) bool
        +payTreasury(Player&, int)
        +payPlayer(Player&, Player&, int)
        +owes(Player&) bool const
        +amountOwed(Player&) int const
        +declareInsolvency()
        +offerPurchase(OwnableDistrict&, Player&)
        +runAuction(OwnableDistrict&)
        +mortgage(string) bool
        +unmortgage(string) bool
        +buyUpgrade(string) bool
        +sellUpgrade(string) bool
        +offerTrade(string, string, string) bool
        +acceptTrade()
        +rejectTrade()
        +guildComplete(Guild, Player&) bool const
        +totalWorth(Player&) int const
        +winner() Player* const
        -current_ : size_t
        -treasury_ : int
        -testingMode_ : bool
        -rng_ : mt19937
        -debts_ : Debt [*]
        -offer_ : TradeOffer
    }
    class Debt {
        <<struct>>
        -creditor : Player* [0..1]
        -amount : int
        -fromLockupFine : bool
        -lockupMove : int
    }
    class TradeOffer {
        <<struct>>
        -from : Player*
        -to : Player*
        -giveDistrict : OwnableDistrict* [0..1]
        -giveReagents : int
        -receiveDistrict : OwnableDistrict* [0..1]
        -receiveReagents : int
        -active : bool
    }
    class Player {
        +name() string const
        +token() char const
        +reagents() int const
        +addReagents(int)
        +position() int const
        +setPosition(int)
        +freePasses() int const
        +inLockup() bool const
        +lockupTurns() int const
        +isInsolvent() bool const
        +declareInsolvent()
        -name_ : string
        -token_ : char
        -reagents_ : int
        -position_ : int
        -freePasses_ : int
        -inLockup_ : bool
        -lockupTurns_ : int
        -insolvent_ : bool
    }
    class Board {
        +at(int) District&
        +advance(int, int) int$
        +grandEntrancePasses(int, int) int$
        +findOwnable(string) OwnableDistrict*
    }
    class Dice {
        +roll()
        +set(int, int)
        +total() int const
        +isDoubles() bool const
    }
    class CardDeck {
        +draw() int
        +remaining() size_t const
        -cards_ : int [18]
        -next_ : size_t
    }
    class SaveGame {
        <<namespace>>
        +load(Game&, string, string&) bool
        +save(Game&, string) bool
    }

    Game "1" *-- "2..6" Player : artificers
    Game "1" *-- "1" Board : circuit
    Game "1" *-- "1" Dice : dice
    Game "1" *-- "0..1" CardDeck : enhancement
    Game "1" *-- "*" Debt : ledger, one per debtor
    Game "1" *-- "1" TradeOffer : pending offer
    Debt "*" --> "0..1" Player : creditor
    TradeOffer "1" --> "2" Player : parties
    SaveGame ..> Game : loads and saves
```

A debt per Artificer, rather than one globally, is required because the insolvency settlement
charges mortgage fees to the **creditor** — who is not the Artificer taking the turn — so that
obligation has to outlive the turn that created it.
