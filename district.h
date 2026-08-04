#ifndef DISTRICT_H
#define DISTRICT_H

#include <string>

#include "boarddata.h"

class Game;
class Player;

// A district is an entity: it has identity on the circuit and is never copied.
class District {
public:
    virtual ~District() = default;

    District(const District &) = delete;
    District &operator=(const District &) = delete;

    const std::string &name() const { return name_; }
    int square() const { return square_; }

    virtual DistrictKind kind() const = 0;
    virtual Guild guild() const { return Guild::None; }
    virtual bool isOwnable() const { return false; }

    // Display hooks. Non-ownable districts keep the defaults, so no caller
    // ever needs to cast a square just to draw it.
    virtual char ownerInitial() const { return ' '; }
    virtual int upgradeCount() const { return 0; }
    virtual bool isMortgaged() const { return false; }
    virtual int buyPrice() const { return 0; }

    virtual void landOn(Game &game, Player &visitor) = 0;

protected:
    District(int square, std::string name);

private:
    int square_;
    std::string name_;
};

// Workshops, Forges and Depots share everything about being owned; they differ
// only in how a visit is priced. That difference is the Template Method hook.
class OwnableDistrict : public District {
public:
    void landOn(Game &game, Player &visitor) final;

    bool isOwnable() const override { return true; }

    const Player *owner() const { return owner_; }
    Player *owner() { return owner_; }
    void setOwner(Player *owner) { owner_ = owner; }

    bool isMortgaged() const override { return mortgaged_; }
    void setMortgaged(bool mortgaged) { mortgaged_ = mortgaged; }

    char ownerInitial() const override;
    int buyPrice() const override { return buyPrice_; }

    // Printed cost of every upgrade bought here; the Guild Levy counts it as
    // part of an Artificer's worth. Forges and Depots take no upgrades.
    virtual int upgradeInvestment() const { return 0; }

protected:
    OwnableDistrict(int square, std::string name, int buyPrice);

    // The one varying step of a visit.
    virtual int computeToll(const Game &game, const Player &visitor) const = 0;

private:
    Player *owner_ = nullptr;  // non-owning; the Game owns its Players
    bool mortgaged_ = false;
    int buyPrice_;
};

#endif  // DISTRICT_H
