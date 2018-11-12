//
//  _action_queue.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2018-11-09.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//

#ifndef kssio_action_queue_hpp
#define kssio_action_queue_hpp

#include <functional>
#include <limits>
#include <memory>

namespace kss {
    namespace io {
        namespace _private {

            // "Borrowed" from kssthread
// TODO: add a "maxPending" option
            /*!
             An action queue provides a single-thread processing queue that performs
             the actions it is given in order. As long as actions are pending, it will
             perform them. When it runs out the thread will sleep until more actions
             are added.
             */
            class ActionQueue {
            public:
                using action_t = std::function<void()>;
                static constexpr size_t noLimit = std::numeric_limits<size_t>::max();

                /*!
                 Construct an action queue.
                 @param maxPending puts a maximum limit on the number of pending actions.
                 */
                explicit ActionQueue(size_t maxPending = noLimit);

                ActionQueue(ActionQueue&&);
                ActionQueue& operator=(ActionQueue&&) noexcept;
                ~ActionQueue() noexcept;

                ActionQueue(const ActionQueue&) = delete;
                ActionQueue& operator=(const ActionQueue&) = delete;

                /*!
                 Add an action to the queue. The action will be performed as soon as
                 possible.
                 @throws std::system_error with a value of EAGAIN if the action queue
                    already has maxPending items, or if we are currently waiting for
                    pending actions to complete. In either case it is recommended that
                    the caller wait a short time, then try again.
                 @throws any exceptions that a condition_variable or a vector may throw.
                 */
                void addAction(const action_t& action);
                void addAction(action_t&& action);

                /*!
                 Wait util all pending actions have completed. Note that no further actions
                 may be added while waiting. But they may be added as soon as wait()
                 has returned.
                 @throws any exceptions that a condition_variable or a vector may throw
                 */
                void wait();

            private:
                struct Impl;
                std::unique_ptr<Impl> impl;
            };
        }
    }
}

#endif /* _action_queue_hpp */
