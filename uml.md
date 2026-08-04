# ArcaneMarket — UML class diagrams

Reflects the final submitted code. Render at <https://mermaid.live> and export PNG/SVG for `report1.pdf`.

## Notation (Chapter 8)

| Symbol | Meaning | C++ realisation here |
|---|---|---|
| `+ - #` | public / private / protected | |
| *italic* | pure virtual | `= 0` |
| <u>underline</u> | static | `static` |
| `{query}` | const member function | `... const` |
| ◁—— | generalization | `class B : public A` |
| ◆—— | **composition** — part belongs to exactly one whole, lifetime tied | by value or `unique_ptr` |
| ◇—— | **aggregation** — part is independent, may belong to several wholes | `shared_ptr` or raw pointer |
| ——▶ | association with navigation | non-owning reference or raw pointer |
| ⋯▶ | dependency | creates / uses |

Chapter 8 warns that *"a final diagram should not leave multiplicities blank"*, so every
association below carries multiplicities at both ends.

Boxes are drawn at two levels of abstraction, which the chapter sanctions
(*"match the level of detail to the conversation"*): the overview names classes and
relationships, the per-pattern diagrams are fully decorated.

---

## 1. Overview — the whole structure

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
    class ControllerInterface { <<abstract>> }
    class GameController { <<Controller>> }
    class District { <<abstract>> }
    class OwnableDistrict { <<abstract>> }
    class TurnState { <<abstract>> }
    class Command { <<abstract>> }

    Subject <|-- Game
    Observer <|-- TextView
    ControllerInterface <|-- GameController
    District <|-- OwnableDistrict

    Game "1" *-- "1" Board : circuit
    Game "1" *-- "2..6" Player : artificers
    Game "1" *-- "1" Turn : turn
    Game "1" *-- "1" Dice : dice
    Game "1" *-- "0..1" CardDeck : enhancement only
    Board "1" *-- "40" District : districts
    Turn "1" *-- "4" TurnState : owns all, one active

    Subject "1" --> "*" Observer : notifies
    TextView "1" --> "1" Game : queries on redraw
    TextView "1" o-- "1" ControllerInterface : strategy
    GameController "1" --> "1" Game : mutates
    Turn "1" --> "1" Game : context
    TurnState "1" --> "1" Turn : context
    Command "*" --> "1" Game : receiver
    OwnableDistrict "*" --> "0..1" Player : owner

    GameController ..> CommandParser : uses
    CommandParser ..> Command : creates
    Board ..> DistrictFactory : uses
    DistrictFactory ..> District : creates
    Game ..> SaveGame : load / save
```

**Why each diamond is the shape it is**, using the chapter's decision procedure:

- `Game ◆— Board`, `◆— Dice` — held by value, destroyed with the Game. Composition.
- `Game ◆— Player`, `◆— Turn`, `◆— CardDeck`, `Board ◆— District`, `Turn ◆— TurnState` —
  `unique_ptr`, which the chapter calls *"the key signal of composition"*.
- `TextView ◇— ControllerInterface` — `shared_ptr`. The Controller does not die with the
  View, so this is aggregation, matching Ch 17's own diagram.
- `Subject —▶ Observer` — a plain association, not a diamond: the observer list is
  non-owning raw pointers, so the Subject is not *made of* its Observers.
- `OwnableDistrict —▶ Player` — plain association with role name `owner`, multiplicity
  `0..1` because an unowned district holds `nullptr`.

---

## 2. District hierarchy — Template Method

`OwnableDistrict::landOn` is `final`: it fixes the order of a visit and defers only the
price to `computeToll`, the one varying step.

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
        +computeToll(Game&, Player&) int const
        +setUpgrades(int)
        +upgradeCost() int const
        +tollAt(int) int const
        -guild_ : Guild
        -tolls_ : int[6]
        -upgrades_ : int
    }
    class Forge {
        +computeToll(Game&, Player&) int const
    }
    class Depot {
        +computeToll(Game&, Player&) int const
    }
    class GuildLevy {
        +landOn(Game&, Player&)
    }
    class WanderingMarket {
        +landOn(Game&, Player&)
    }
    class EnchantmentDeck {
        +landOn(Game&, Player&)
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

    OwnableDistrict "*" --> "0..1" Player : owner
```

---

## 3. State — the turn phases

`Turn` is the Context. It owns all four states for its whole life and swaps a non-owning
`state_` pointer, so a transition allocates nothing. `resume_` records the phase a trade
offer interrupted, because an offer may be made before *or* after rolling.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction LR

    class Turn {
        +roll(int, int)
        +next()
        +beginTurn()
        +setState(TurnState*)
        +preRollState() TurnState* const
        +postRollState() TurnState* const
        +lockupState() TurnState* const
        +enterTradeResponse()
        +resumeAfterTrade()
        +consecutiveDoubles() int const
        -game_ : Game&
        -state_ : TurnState* [1]
        -resume_ : TurnState* [0..1]
        -consecutiveDoubles_ : int
    }
    class TurnState {
        <<abstract>>
        +roll(int, int)*
        +next()*
        +name() string_view const*
    }
    class PreRollState {
        +roll(int, int)
        +next()
        -turn_ : Turn&
    }
    class PostRollState {
        +roll(int, int)
        +next()
        -turn_ : Turn&
    }
    class LockupState {
        +roll(int, int)
        +next()
        -turn_ : Turn&
    }
    class AwaitingTradeResponseState {
        +roll(int, int)
        +next()
        -turn_ : Turn&
    }

    TurnState <|-- PreRollState
    TurnState <|-- PostRollState
    TurnState <|-- LockupState
    TurnState <|-- AwaitingTradeResponseState
    Turn "1" *-- "4" TurnState : owns all
    Turn "1" --> "1" TurnState : active state
    TurnState "4" --> "1" Turn : context
    Turn "1" --> "1" Game : context
```

Behaviour, as a UML state diagram:

```mermaid
stateDiagram-v2
    [*] --> PreRoll : beginTurn / not detained
    [*] --> Lockup : beginTurn / detained
    PreRoll --> PreRoll : roll doubles
    PreRoll --> PostRoll : roll, or third double
    Lockup --> PostRoll : doubles, fine, token, or stay
    PreRoll --> AwaitingTrade : trade offered
    PostRoll --> AwaitingTrade : trade offered
    AwaitingTrade --> PreRoll : accept or reject / resume_
    AwaitingTrade --> PostRoll : accept or reject / resume_
    PostRoll --> [*] : next
```

---

## 4. Command — the interpreter

`GameController` is the Invoker and knows only the interface. `CommandParser` binds each
command's receiver and arguments at construction, per Ch 20. A `nullptr` return is the
graceful path for a misspelled verb.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction LR

    class Command {
        <<abstract>>
        +execute()*
    }
    class CommandParser {
        +parse(Game&, string) Command*$
    }
    class GameController {
        +handleCommand(string)
        -game_ : Game&
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
    class UpgradeCommand {
        +execute()
        -game_ : Game&
        -district_ : string
        -buying_ : bool
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

    ControllerInterface <|-- GameController
    GameController ..> CommandParser : uses
    CommandParser ..> Command : creates
    GameController "1" --> "0..1" Command : invokes
    RollCommand "*" --> "1" Game : receiver
```

`MessageCommand` is the only command with no receiver — it reports a refusal, which keeps
console output in the Command layer instead of pushing it into the parser.

---

## 5. MVC + Observer

Ch 17's arrangement, which is **not** the common one: the View owns the Controller as its
Strategy, the View registers as an Observer of the Model, and the View queries the Model
directly when redrawing. The Controller only handles user-initiated events.

```mermaid
---
config:
  class:
    hideEmptyMembersBox: true
---
classDiagram
    direction LR

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
        +movePlayer(Player&, int)
        +offerPurchase(OwnableDistrict&, Player&)
        +runAuction(OwnableDistrict&)
        +offerTrade(string, string, string)
        +declareInsolvency()
        +owes(Player&) bool const
        +winner() Player* const
        -current_ : size_t
        -treasury_ : int
        -debts_ : map of Player to Debt
    }
    class TextView {
        <<View>>
        +update()
        +render(ostream&) void const
        +handleInput(string)
        -game_ : Game&
        -controller_ : ControllerInterface [1]
    }
    class ControllerInterface {
        <<abstract>>
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
    Subject "1" --> "*" Observer : notifies, never names a View
    TextView "1" --> "1" Game : queries on redraw
    TextView "1" o-- "1" ControllerInterface : strategy
    GameController "1" --> "1" Game : mutates
```
