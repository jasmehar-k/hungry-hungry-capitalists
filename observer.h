#ifndef OBSERVER_H
#define OBSERVER_H

#include <algorithm>
#include <vector>

class Observer {
public:
    virtual ~Observer() = default;
    virtual void update() = 0;

protected:
    Observer() = default;
};

class Subject {
public:
    virtual ~Subject() = default;

    void addObserver(Observer *observer) {
        if (observer && std::find(observers_.begin(), observers_.end(), observer) ==
                            observers_.end()) {
            observers_.push_back(observer);
        }
    }

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
    std::vector<Observer *> observers_;  // non-owning
};

#endif  // OBSERVER_H
