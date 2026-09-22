#pragma once

#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

namespace satsim {

// A discrete-event simulation is built around one core idea: instead of
// stepping forward in small fixed time increments (which wastes effort on
// long stretches where nothing happens), we jump directly from one
// scheduled event to the next. This queue holds all pending events,
// ordered by when they should occur.
class EventQueue {
public:
    using Action = std::function<void()>;

    struct Entry {
        double time;
        Action action;
    };

    // Schedules an action to run at the given simulated time (seconds).
    void schedule(double time, Action action) {
        heap_.push(HeapEntry{time, nextSequence_++, std::move(action)});
    }

    bool empty() const { return heap_.empty(); }
    size_t size() const { return heap_.size(); }

    // Removes and returns the earliest-scheduled event. The caller is
    // responsible for advancing its own notion of "current time" to
    // entry.time before invoking entry.action() - the queue itself has no
    // concept of "now", it just orders events.
    Entry popNext() {
        HeapEntry top = heap_.top();
        heap_.pop();
        return Entry{top.time, std::move(top.action)};
    }

private:
    struct HeapEntry {
        double time;
        uint64_t sequence; // tie-breaker: events at the same time run in the order they were scheduled
        Action action;
    };

    struct Compare {
        bool operator()(const HeapEntry& a, const HeapEntry& b) const {
            if (a.time != b.time) return a.time > b.time; // min-heap on time
            return a.sequence > b.sequence;                // FIFO among equal times
        }
    };

    std::priority_queue<HeapEntry, std::vector<HeapEntry>, Compare> heap_;
    uint64_t nextSequence_ = 0;
};

} // namespace satsim
