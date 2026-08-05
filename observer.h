#ifndef OBSERVER_H
#define OBSERVER_H

#include <algorithm>
#include <vector>

// The Observer pattern in the shape Chapter 17 gives it: a Subject keeps a
// list of Observers and tells each of them that something changed, without
// knowing what any of them will do about it.
//
// Here the Subject is Game and the sole Observer is TextView. The Model is
// therefore able to announce a change without naming a view class, which is
// what lets the test harnesses construct a Game with no view attached at all.
//
// Notification is fired by GameController once per command rather than by each
// mutator, because the spec's unit of redraw is the command; the rationale is
// with Game itself.

class Observer {
public:
    virtual ~Observer() = default;
    // Pull, not push: the Observer is told only that something changed and
    // queries the Subject for whatever it needs. TextView redraws the whole
    // board, so a payload would be discarded anyway.
    virtual void update() = 0;

protected:
    // An interface, so only a derived class may construct one.
    Observer() = default;
};

class Subject {
public:
    virtual ~Subject() = default;

    // Registering twice would redraw the board twice per command, so a
    // duplicate is silently ignored rather than being stored again.
    void addObserver(Observer *observer) {
        if (observer && std::find(observers_.begin(), observers_.end(), observer) ==
                            observers_.end()) {
            observers_.push_back(observer);
        }
    }

    // Erase-remove: std::remove shuffles the matches to the end and returns
    // the new logical end, and erase then drops that tail. TextView calls this
    // from its destructor, so an observer can never outlive its registration.
    void removeObserver(Observer *observer) {
        observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                         observers_.end());
    }

    void notifyObservers() {
        for (auto *observer : observers_) {
            observer->update();
        }
    }

protected:
    Subject() = default;

private:
    // Non-owning: an observer is owned by whoever created it (main owns the
    // TextView), and a Subject that deleted its observers would be claiming an
    // ownership it was never given.
    std::vector<Observer *> observers_;
};

#endif  // OBSERVER_H
