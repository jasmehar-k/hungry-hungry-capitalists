# ArcaneMarket — UML class diagrams

Reflects the final submitted code.

## How these are organised

**Figures 1A and 1B together are the class diagram.** It is too large for one page, so it is
split into two clearly labelled parts, as permitted. Every major class appears in exactly one
part with its key attributes and its full public interface; where a relationship crosses the
split, the far class appears as a name-only box marked with the part that details it.

- **Part A — Application and control:** Observer infrastructure, View, Controller, the command
  interpreter, and the turn state machine.
- **Part B — Domain model:** `Game` and its economy, the circuit, the district hierarchy, and
  persistence.

**Figures 2–7 are supplementary per-pattern views**, referenced from the Design Patterns section
of the report. They repeat classes already in 1A/1B, reduced to just the participants in one
pattern so the pattern's shape is visible without tracing it out of the full diagram.

Following Chapter 8, method **parameters are not associations** — `landOn(Game&, Player&)` passes
references at call time, which the chapter explicitly excludes. Only stored references and
pointers are drawn as lines.

## Notation (Chapter 8)

| Symbol | Meaning | C++ here |
|---|---|---|
| `+ - #` | public / private / protected | |
| *italics* | pure virtual | `= 0` |
| <u>underline</u> | static | `static` |
| `const` | query operation | `... const` |
| ◁—— | **inheritance** (generalization) | `class B : public A` |
| ◆—— | **composition** | by value or `unique_ptr` |
| ◇—— | **aggregation** | `shared_ptr` |
| ——▶ | association | stored reference or non-owning pointer |
| ⋯▶ | **dependency** | creates / uses |

---

## Figure 1A — Class diagram, Part A: application and control

`Game` is shown name-only here; it is detailed in Part B.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction LR

    class Game {
        <<Model - detailed in Part B>>
    }

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
    class TextView {
        <<View>>
        +update()
        +render(ostream&) const
        +handleInput(string)
        -game_ : Game&
        -controller_ : ControllerInterface
        -out_ : ostream&
    }
    class ControllerInterface {
        <<interface>>
        +handleCommand(string)*
    }
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
        -out_ : ostream&
        -message_ : string
    }
    class Turn {
        <<Context>>
        +roll(int, int)
        +next()
        +beginTurn()
        +out() ostream&
        +endTurn()
        +setState(TurnState*)
        +preRollState() TurnState* const
        +postRollState() TurnState* const
        +lockupState() TurnState* const
        +enterTradeResponse()
        +resumeAfterTrade()
        +countDouble()
        +resetDoubles()
        +consecutiveDoubles() int const
        +stateName() string_view const
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

    Subject <|-- Game
    Observer <|-- TextView
    ControllerInterface <|-- GameController
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
    TurnState <|-- PreRollState
    TurnState <|-- PostRollState
    TurnState <|-- LockupState
    TurnState <|-- AwaitingTradeResponseState

    Turn "1" *-- "4" TurnState : owns all four
    Turn "1" --> "1" TurnState : state_, active
    Turn "1" --> "1" Game : context
    TurnState "4" --> "1" Turn : context
    Subject "1" --> "*" Observer : notifies
    TextView "1" o-- "1" ControllerInterface : strategy
    TextView "1" --> "1" Game : observes and queries
    GameController "1" --> "1" Game : mutates
    Command "*" --> "1" Game : receiver
    GameController ..> CommandParser : uses
    CommandParser ..> Command : creates
```

---

## Figure 1B — Class diagram, Part B: domain model

`Turn`, `TurnState` and `Command` are shown name-only here; they are detailed in Part A.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction LR

    class Turn { <<detailed in Part A>> }
    class TurnState { <<detailed in Part A>> }
    class Command { <<detailed in Part A>> }

    class Game {
        <<Model>>
        +board() Board&
        +addPlayer(string, char) bool
        +tokenTaken(char) bool const
        +playerCount() size_t const
        +player(size_t) Player&
        +currentPlayer() Player&
        +setCurrentPlayer(size_t)
        +endTurn()
        +testingMode() bool const
        +rng() mt19937&
        +dice() Dice&
        +out() ostream& const
        +roll(int, int)
        +next()
        +beginTurn()
        +turnStateName() string_view const
        +movePlayer(Player&, int)
        +sendToLockup(Player&)
        +offerLockupExit(Player&) bool
        +forceLockupExit(Player&) bool
        +payTreasury(Player&, int)
        +payPlayer(Player&, Player&, int)
        +treasury() int const
        +guildComplete(Guild, Player&) bool const
        +countOwned(DistrictKind, Player&) int const
        +totalWorth(Player&) int const
        +districtsOwnedBy(Player&) OwnableDistrict* [*] const
        +drawEnchantmentValue() int
        +usingShuffledDeck() bool const
        +totalFreePasses() int const
        +tryGrantFreePass(Player&) bool
        +prompt(string) string const
        +offerPurchase(OwnableDistrict&, Player&)
        +owes(Player&) bool const
        +amountOwed(Player&) int const
        +declareInsolvency()
        +mortgage(string) bool
        +unmortgage(string) bool
        +buyUpgrade(string) bool
        +sellUpgrade(string) bool
        +offerTrade(string, string, string) bool
        +acceptTrade()
        +rejectTrade()
        +hasTradeOffer() bool const
        +solventCount() size_t const
        +winner() Player* const
        +isOver() bool const
        -current_ : size_t
        -treasury_ : int
        -testingMode_ : bool
        -in_ : istream&
        -out_ : ostream&
        -rng_ : mt19937
        -debts_ : Debt [*]
        -offer_ : TradeOffer
    }
    class Auction {
        +run(OwnableDistrict&)
        -game_ : Game&
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
        +setReagents(int)
        +position() int const
        +setPosition(int)
        +freePasses() int const
        +setFreePasses(int)
        +inLockup() bool const
        +lockupTurns() int const
        +sendToLockup()
        +releaseFromLockup()
        +serveLockupTurn()
        +setLockupState(bool, int)
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
        +size() int$
        +at(int) District&
        +advance(int, int) int$
        +grandEntrancePasses(int, int) int$
        +passedGrandEntrance(int, int) bool$
        +ownables() OwnableDistrict* [22]
        +findOwnable(string) OwnableDistrict*
        +findWorkshop(string) Workshop*
        -districts_ : District [40]
        -ownables_ : OwnableDistrict* [22]
    }
    class District {
        <<abstract>>
        +name() string const
        +square() int const
        +kind() DistrictKind const*
        +guild() Guild const
        +isOwnable() bool const
        +ownerInitial() char const
        +upgradeCount() int const
        +isMortgaged() bool const
        +buyPrice() int const
        +landOn(Game&, Player&)*
        -square_ : int
        -name_ : string
    }
    class OwnableDistrict {
        <<abstract>>
        +landOn(Game&, Player&) final
        +isOwnable() bool const
        +owner() Player* const
        +setOwner(Player*)
        +isMortgaged() bool const
        +setMortgaged(bool)
        +ownerInitial() char const
        +buyPrice() int const
        +upgradeInvestment() int const
        #computeToll(Game&, Player&) int const*
        -owner_ : Player* [0..1]
        -mortgaged_ : bool
        -buyPrice_ : int
    }
    class Workshop {
        +kind() DistrictKind const
        +guild() Guild const
        +upgradeCount() int const
        +setUpgrades(int)
        +upgradeCost() int const
        +upgradeInvestment() int const
        +tollAt(int) int const
        #computeToll(Game&, Player&) int const
        -guild_ : Guild
        -upgradeCost_ : int
        -tolls_ : int[6]
        -upgrades_ : int
    }
    class Forge {
        +kind() DistrictKind const
        #computeToll(Game&, Player&) int const
    }
    class Depot {
        +kind() DistrictKind const
        #computeToll(Game&, Player&) int const
    }
    class GrandEntrance {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class Lockup {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class FreeRest {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class GoToLockup {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class GuildLevy {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class TransitToll {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class MarketFestival {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class WanderingMarket {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class EnchantmentDeck {
        +kind() DistrictKind const
        +landOn(Game&, Player&)
    }
    class Dice {
        +roll()
        +set(int, int)
        +first() int const
        +second() int const
        +total() int const
        +isDoubles() bool const
        -rng_ : mt19937&
        -first_ : int
        -second_ : int
    }
    class CardDeck {
        +draw() int
        +remaining() size_t const
        +size() size_t$
        -rng_ : mt19937&
        -cards_ : int [18]
        -next_ : size_t
    }
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
    class SaveGame {
        <<namespace>>
        +load(Game&, string, string&) bool
        +save(Game&, string) bool
    }
    class Setup {
        <<namespace>>
        +runNewGame(Game&, istream&, ostream&) bool
    }

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

    Game "1" *-- "1" Board : circuit
    Game "1" *-- "2..6" Player : artificers
    Game "1" *-- "1" Dice : dice
    Game "1" *-- "0..1" CardDeck : enhancement
    Game "1" *-- "1" Turn : turn
    Game "1" *-- "*" Debt : ledger, one per debtor
    Game "1" *-- "1" TradeOffer : pending offer
    Board "1" *-- "40" District : districts
    Turn "1" *-- "4" TurnState : states

    Debt "*" --> "0..1" Player : creditor
    TradeOffer "1" --> "2" Player : parties
    TradeOffer "1" --> "0..2" OwnableDistrict : stakes
    OwnableDistrict "*" --> "0..1" Player : owner
    Command "*" --> "1" Game : receiver
    Auction "1" --> "1" Game : bids through
    Game ..> Auction : creates per sale

    Board ..> DistrictFactory : uses
    DistrictFactory ..> DistrictSpec : reads
    DistrictFactory ..> District : creates
    SaveGame ..> Game : loads and saves
    Setup ..> Game : builds roster
```

Crafting Guild membership is a value (`enum class Guild`) carried by each `Workshop`, not a
class, so guild size appears as a rule rather than a multiplicity: Ember has 4 Workshops, Smoke
3, Arcane 4, Gilded 5. `Game::guildComplete` counts the actual membership rather than assuming a
fixed size.

---

## Figure 2 — MVC + Observer (supplementary)

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
    class Game { <<Model>> }
    class TextView {
        <<View>>
        +update()
        +render(ostream&) const
        +handleInput(string)
        -game_ : Game&
        -controller_ : ControllerInterface
        -out_ : ostream&
    }
    class ControllerInterface {
        <<interface>>
        +handleCommand(string)*
    }
    class GameController {
        <<Controller>>
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

---

## Figure 3 — Template Method (supplementary)

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction TB

    class OwnableDistrict {
        <<abstract>>
        +landOn(Game&, Player&) final
        #computeToll(Game&, Player&) int const*
    }
    class Workshop {
        #computeToll(Game&, Player&) int const
    }
    class Forge {
        #computeToll(Game&, Player&) int const
    }
    class Depot {
        #computeToll(Game&, Player&) int const
    }

    OwnableDistrict <|-- Workshop
    OwnableDistrict <|-- Forge
    OwnableDistrict <|-- Depot
```

`landOn` is `final` and fixes the sequence — unowned? owned by the visitor? mortgaged? otherwise
charge — while `computeToll` is the only step that varies.

---

## Figure 4 — State (supplementary)

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
        +setState(TurnState*)
        -preRoll_ : TurnState
        -postRoll_ : TurnState
        -lockup_ : TurnState
        -awaitingTrade_ : TurnState
        -state_ : TurnState*
        -resume_ : TurnState* [0..1]
    }
    class TurnState {
        <<interface>>
        +roll(int, int)*
        +next()*
    }
    class PreRollState {
        +roll(int, int)
        +next()
    }
    class PostRollState {
        +roll(int, int)
        +next()
    }
    class LockupState {
        +roll(int, int)
        +next()
    }
    class AwaitingTradeResponseState {
        +roll(int, int)
        +next()
    }

    TurnState <|-- PreRollState
    TurnState <|-- PostRollState
    TurnState <|-- LockupState
    TurnState <|-- AwaitingTradeResponseState
    Turn "1" *-- "4" TurnState : owns all four
    Turn "1" --> "1" TurnState : state_, active
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

## Figure 6 — Command (supplementary)

Three of the thirteen commands, chosen to show the range of bound arguments. All thirteen appear
in Figure 1A.

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
    }
    class CommandParser {
        +parse(Game&, string) Command*$
    }
    class Command {
        <<interface>>
        +execute()*
    }
    class RollCommand {
        +execute()
        -game_ : Game&
        -die1_ : int
        -die2_ : int
    }
    class TradeCommand {
        +execute()
        -game_ : Game&
        -target_ : string
        -give_ : string
        -receive_ : string
    }
    class MessageCommand {
        +execute()
        -out_ : ostream&
        -message_ : string
    }
    class Game { <<Receiver>> }

    Command <|-- RollCommand
    Command <|-- TradeCommand
    Command <|-- MessageCommand
    GameController ..> CommandParser : uses
    CommandParser ..> Command : creates
    GameController "1" --> "0..1" Command : execute()
    RollCommand "*" --> "1" Game : receiver
    TradeCommand "*" --> "1" Game : receiver
```

---

## Figure 7 — Simple Factory (supplementary)

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
    class DistrictSpec { <<struct>> }
    class District { <<abstract>> }
    class Board

    District <|-- Workshop
    District <|-- Forge
    District <|-- Depot
    Board ..> DistrictFactory : uses
    DistrictFactory ..> DistrictSpec : reads
    DistrictFactory ..> District : creates
    Board "1" *-- "40" District : districts
```

One concrete creator with a static operation and **no creator hierarchy** — that absence is what
separates the Simple Factory idiom from GoF Factory Method.
