# Design notes

Working material for `report1.pdf`. Written as the code lands rather than reconstructed at the end.

## Pattern inventory

What is claimed, and how strongly. Phase 2 peer review classifies each claimed pattern as *actually realized*, *partially realized*, or *merely named*, so nothing is claimed beyond what the code does.

| Pattern | Where | Status |
|---|---|---|
| MVC | `Game` / `TextView` / `GameController` | Realized. Ch 17's arrangement: View owns the Controller, observes the Model, and queries the Model directly on redraw. |
| Observer | `observer.h`, `Game : Subject`, `TextView : Observer` | Realized. `Game` notifies at the end of every mutator and never references a View. |
| Template Method | `OwnableDistrict::landOn` (`final`) with `computeToll()` hook | Structure realized; the hook bodies fill in with the movement step. |
| Simple Factory | `DistrictFactory::create` | Used, and **named as the Simple Factory idiom, not GoF Factory Method**. See below. |
| Decorator | — | **Deliberately rejected.** See Design Question 3. |
| State, Command, Iterator | planned | Not yet written; do not claim until they exist. |

### Why Factory Method is not claimed

Ch 12 is explicit that Simple Factory "is not a pattern" — the GoF never named it — and that it suits "a small, fixed set of product types that you do not expect to grow." The 40-square circuit is exactly that: the district kinds are fixed by the spec and will never grow. Building an abstract creator hierarchy purely to make Factory Method applicable would be the over-engineering the same chapter warns against ("don't reach for factories when a constructor would do"). So `DistrictFactory` is a single static creation function, honestly labelled. If a real creator hierarchy appears later (card creation is the plausible candidate), Factory Method gets claimed there.

### Note on Strategy

Ch 17 describes the Controller as the View's Strategy, and `TextView` does hold a `shared_ptr<ControllerInterface>`. But there is currently only **one** concrete controller, so nothing yet demonstrates swappability. Present this as part of MVC's compound structure rather than as a separately claimed pattern, unless a second controller earns its place. Note that mode-specific command legality (auction, trade response, Guild Levy) is State's job, not a second Controller's — so a second Controller may never be warranted.

## Design Question 1 — Observer for the Circuit Board?

**Yes, but the granularity matters more than the yes.**

The payoff is that `Game` holds no reference to any view: it calls `notifyObservers()` and is done. That is what makes the graphical-display bonus feature additive rather than invasive — a second view attaches to the same Model with no Model change, and both redraw from one notification.

The caveat worth stating in the report: the spec redraws the **entire** board after every command. So per-district Subjects would be pure overhead — a change to one district triggers a full redraw regardless, and there is no partial repaint to optimize. The right shape is one Subject (`Game`) with the whole `TextView` as a single Observer. Ch 17's worked example puts a Composite inside the Model so notifications climb leaf-to-root; we deliberately do not, because that machinery buys nothing when every update repaints everything.

**A real consequence we hit:** because every mutator notifies, six `addPlayer` calls during setup print six boards. Observer is behaving correctly; the fix belongs in the setup flow (attach the view after setup, or suppress during it). Worth reporting as a concrete trade-off actually encountered rather than a hypothetical.

## Design Question 3 — Decorator for Upgrades?

**No.** Four reasons, in descending strength:

1. **Tolls are a table, not a composition.** Tinder's tolls run 2, 10, 30, 90, 160, 250 — neither additive nor multiplicative. A decorator chain composes behaviour, but there is no per-upgrade delta to compose; each level is an independent table entry. The pattern would wrap five objects only to index an array.
2. **Selling breaks the chain.** An Artificer may sell one upgrade at a time. Decorator offers no removal protocol — you would have to unwrap and rebuild the chain, which is not what the pattern is for.
3. **Identity churn.** Re-wrapping produces a *different object*. `Player` and `Board` hold pointers to the district; every sale or purchase would invalidate them or leave them pointing at a stale wrapper.
4. **Bounded at five.** Decorator earns its keep with open-ended composition. Upgrades cap at 4 Forge Stations plus a Master Atelier.

**Implemented instead:** an `int upgrades_` on `Workshop` indexing a fixed toll table. Guild-complete doubling on unimproved Workshops is a *rule* evaluated at toll time, not a decoration of the object.

## Design Question 2 — card-deck mechanics

Not yet implemented. The spec's Wandering Market and Enchantment Deck are probability tables; a real shuffled deck is a listed bonus feature. Intended answer: each card becomes a **Command** object encapsulating its effect, and the deck is a collection traversed by an **Iterator** that reshuffles on exhaustion. Both patterns have chapters in `resources/`. Do not claim either until written.

## Correctness notes worth reporting under "Challenges"

Two defects found by deliberately testing the foundation rather than eyeballing it:

- **Backward movement paid the Grand Entrance bonus.** `passedGrandEntrance(from, to)` tested `to < from`, which is true for *any* backward move — so a Wandering Market sending an Artificer from square 5 to 2 wrongly paid 200 Reagents. The destination alone cannot distinguish a forward lap from a backward step; the signature now takes the step count and only pays for `steps > 0`.
- **The cell renderer threw `std::length_error`.** When the token string exceeded the cell width, the padding length underflowed as an unsigned value. The root cause was that nothing enforced the spec's 2–6 player cap, so `Game::addPlayer` now rejects a seventh Artificer and a duplicate token, and the renderer clamps as a second line of defence.
