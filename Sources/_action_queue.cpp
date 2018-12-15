//
//  _action_queue.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-11-09.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <atomic>
#include <cassert>
#include <cerrno>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <system_error>
#include <thread>
#include <vector>

#include <syslog.h>

#include "_action_queue.hpp"

using namespace std;
using namespace kss::io::_private;


struct ActionQueue::Impl {
    thread          actionThread;
    size_t          maxPending = 0;
    atomic<bool>    stopping { false };
    atomic<bool>    waiting { false };
    atomic<bool>    runningActions { false };

    // The following are all protected by the lock and condition variable.
    mutex               lock;
    condition_variable  cv;
    vector<action_t>    pendingActions;

    void runActionThread() {
        vector<action_t> currentActions;
        while (true) {
            // While protected by the lock and condition variable, wait for there to be
            // pending actions, then move them to a local list.
            {
                unique_lock<mutex> l(lock);
                while (!stopping && pendingActions.empty()) { cv.wait(l); }

                // If we are flagged to be stopping, break out of the loop so we exit
                // the thread.
                if (stopping) {
                    break;
                }

                // Dequeue any pending actions and run them.
                assert(currentActions.empty());
                if (!pendingActions.empty()) {
                    currentActions.swap(pendingActions);
                }
            }

            // Run the items.
            runningActions = true;
            for (auto& action : currentActions) {
                action();
            }
            currentActions.clear();
            runningActions = false;
            cv.notify_all();
         }
    }
};

ActionQueue::ActionQueue(ActionQueue&&) = default;
ActionQueue& ActionQueue::operator=(ActionQueue &&) noexcept = default;

ActionQueue::ActionQueue(size_t maxPending) : impl(new Impl()) {
    impl->maxPending = maxPending;
    impl->actionThread = thread { [this]{ impl->runActionThread(); }};
}

ActionQueue::~ActionQueue() noexcept {
    try {
        impl->stopping = true;
        impl->cv.notify_all();
        impl->actionThread.join();
    }
    catch (const exception& e) {
        // Best we can do is log the error and continue.
        syslog(LOG_ERR, "[%s] Exception shutting down: %s", __PRETTY_FUNCTION__, e.what());
    }
}

void ActionQueue::addAction(const action_t &action) {
    if (!impl->waiting && !impl->stopping) {
        unique_lock<mutex> l(impl->lock);
        if (impl->pendingActions.size() >= impl->maxPending) {
            throw system_error(EAGAIN, system_category(), "addAction");
        }
        
        impl->pendingActions.emplace_back(action);
        impl->cv.notify_all();
    }
}

void ActionQueue::addAction(action_t &&action) {
    if (!impl->waiting && !impl->stopping) {
        unique_lock<mutex> l(impl->lock);
        if (impl->pendingActions.size() >= impl->maxPending) {
            throw system_error(EAGAIN, system_category(), "addAction");
        }
        
        impl->pendingActions.emplace_back(action);
        impl->cv.notify_all();
    }
}

void ActionQueue::wait() {
    unique_lock<mutex> l(impl->lock);
    if (!impl->pendingActions.empty() || impl->runningActions) {
        impl->waiting = true;
        while (!impl->stopping
               && (!impl->pendingActions.empty() || impl->runningActions))
        {
            impl->cv.wait(l);
        }
        impl->waiting = false;
    }
}
