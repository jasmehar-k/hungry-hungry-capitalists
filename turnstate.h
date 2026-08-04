#ifndef TURNSTATE_H
#define TURNSTATE_H

#include <string_view>

// Forward declaration breaks the cycle: states act on the Turn, and the Turn
// holds states.
class Turn;

// One pure virtual per action the Turn exposes. Which commands are legal is a
// property of the phase, so each phase owns its own answers rather than the
// Turn carrying a cascade of conditionals.
class TurnState {
public:
    virtual ~TurnState() = default;

    // die1/die2 of -1 mean "use random dice"; -testing supplies real values.
    virtual void roll(int die1, int die2) = 0;
    virtual void next() = 0;
    virtual std::string_view name() const = 0;

protected:
    TurnState() = default;
};

// A roll is pending. Rolling moves and resolves; ending the turn is refused.
class PreRollState final : public TurnState {
public:
    explicit PreRollState(Turn &turn) : turn_{turn} {}
    void roll(int die1, int die2) override;
    void next() override;
    std::string_view name() const override { return "may roll"; }

private:
    Turn &turn_;
};

// The roll is spent. The Artificer may still deal, then end the turn.
class PostRollState final : public TurnState {
public:
    explicit PostRollState(Turn &turn) : turn_{turn} {}
    void roll(int die1, int die2) override;
    void next() override;
    std::string_view name() const override { return "may end turn"; }

private:
    Turn &turn_;
};

// Detained. Rolling attempts the exit procedure rather than moving.
class LockupState final : public TurnState {
public:
    explicit LockupState(Turn &turn) : turn_{turn} {}
    void roll(int die1, int die2) override;
    void next() override;
    std::string_view name() const override { return "in The Lockup"; }

private:
    Turn &turn_;
};

// A fourth state, AwaitingTradeResponse, belongs here once the trade command
// exists (spec priority 8). It is deliberately absent until then: an
// unreachable state would pad the pattern without implementing it.

#endif  // TURNSTATE_H
