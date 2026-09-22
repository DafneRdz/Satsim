#include "test_framework.hpp"
#include "sim/EventQueue.hpp"

using namespace satsim;

TEST_CASE(events_pop_in_ascending_time_order) {
    EventQueue queue;
    std::vector<double> observedOrder;

    queue.schedule(5.0, [&]() { observedOrder.push_back(5.0); });
    queue.schedule(1.0, [&]() { observedOrder.push_back(1.0); });
    queue.schedule(3.0, [&]() { observedOrder.push_back(3.0); });

    while (!queue.empty()) {
        auto entry = queue.popNext();
        entry.action();
    }

    CHECK_TRUE(observedOrder.size() == 3);
    CHECK_NEAR(observedOrder[0], 1.0, 1e-12);
    CHECK_NEAR(observedOrder[1], 3.0, 1e-12);
    CHECK_NEAR(observedOrder[2], 5.0, 1e-12);
}

TEST_CASE(events_at_the_same_time_run_in_scheduling_order) {
    EventQueue queue;
    std::vector<int> observedOrder;

    queue.schedule(2.0, [&]() { observedOrder.push_back(1); });
    queue.schedule(2.0, [&]() { observedOrder.push_back(2); });
    queue.schedule(2.0, [&]() { observedOrder.push_back(3); });

    while (!queue.empty()) {
        auto entry = queue.popNext();
        entry.action();
    }

    CHECK_TRUE(observedOrder.size() == 3);
    CHECK_TRUE(observedOrder[0] == 1);
    CHECK_TRUE(observedOrder[1] == 2);
    CHECK_TRUE(observedOrder[2] == 3);
}

TEST_CASE(an_action_can_schedule_further_events) {
    // Confirms the queue correctly interleaves newly-scheduled events with
    // ones that were already pending - the core mechanic a simulator
    // depends on (e.g. a packet's arrival event scheduling its next hop).
    EventQueue queue;
    std::vector<double> observedOrder;

    queue.schedule(10.0, [&]() { observedOrder.push_back(10.0); });
    queue.schedule(1.0, [&]() {
        observedOrder.push_back(1.0);
        queue.schedule(5.0, [&]() { observedOrder.push_back(5.0); });
    });

    while (!queue.empty()) {
        auto entry = queue.popNext();
        entry.action();
    }

    CHECK_TRUE(observedOrder.size() == 3);
    CHECK_NEAR(observedOrder[0], 1.0, 1e-12);
    CHECK_NEAR(observedOrder[1], 5.0, 1e-12);
    CHECK_NEAR(observedOrder[2], 10.0, 1e-12);
}

TEST_CASE(empty_queue_reports_empty) {
    EventQueue queue;
    CHECK_TRUE(queue.empty());
    queue.schedule(1.0, []() {});
    CHECK_TRUE(!queue.empty());
}
