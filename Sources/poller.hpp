//
//  poller.hpp
//  kssio
//
//  Created by Steven W. Klassen on 2017-10-02.
//  Copyright © 2017 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssio_poller_hpp
#define kssio_poller_hpp

#include <chrono>
#include <memory>
#include <string>

namespace kss {
    namespace io {

        class Poller;


        /*!
         This structure is used to describe each resource that the poller should monitor.
         */
        struct PolledResource {

            /*!
             This enumeration is used to describe what type of events we are interested
             in for a given resource.
             */
            enum class Event {
                read,				//!< We want to know when we can read.
                write,				//!< We want to know when we can write.
                any					//!< We want to know when we can read or write.
            };

            std::string	name;					///< A name used to identify the resource.
            int			filedes { 0 };			///< The resource file descriptor.
            Event		event;					///< The type of events we wish to monitor.
            void*		payload { nullptr };	///< An optional pointer to be passed with the resource record.
        };


        /*!
         This is the interface for the poller delegate. A delegate must be provided
         for a poller to do anything useful. The delegate is divided into two sections,
         one that controls the operation of the poller and one that responds to the
         availability of the resources.

         It is best that the methods of this interface do not throw exceptions. However
         if they do they will be automatically caught and ignored.
         */
        class PollerDelegate {
        public:

            // The following methods are called by Poller in order to control the
            // polling mechanism.

            /*!
             Should return true when the poller should stop.
             */
            virtual bool pollerShouldStop() const = 0;

            /*!
             Returns the maximum time duration that each call to the internal poll
             (or select) call should wait before returning a result. The smaller the
             value, the more responsive the system is to stopping and to monitored
             resources being added and removed. However the smaller the value is the
             closer Poller::run() is to a "busy wait." For most applications the
             default given here (100ms) is likely a fairly good value.

             Although it probably isn't a great idea, this method can return different
             values at different times. The new value will come into effect the next
             time that the internal poll completes and is called again.

             The following values have special meaning to the system:
             std::chrono::duration::none - No waiting is performed. The poller becomes
             a "busy wait."
             std::chrono::milliseconds::max - No timeout is used. The poller will
             wait forever unless these is an event on one of the resources
             forcing it to check again.
             */
            virtual std::chrono::milliseconds pollerMaximumWaitInterval() const {
                using namespace std::chrono_literals;
                return 100ms;
            }


            // The following methods are called by Poller in order to allow the
            // delegate to react to the various things that happen.

            /*!
             Called just after Poller::poll() has begun.
             */
            virtual void pollerHasStarted(Poller& p) {}

            /*!
             Called just before Poller::poll() exits.
             */
            virtual void pollerWillStop(Poller& p) {}

            /*!
             Called when a resource is available for reading.
             */
            virtual void pollerResourceReadIsReady(Poller& p,
                                                   const PolledResource& resource) {}

            /*!
             Called when a resource is available for writing.
             */
            virtual void pollerResourceWriteIsReady(Poller& p,
                                                    const PolledResource& resource) {}

            /*!
             Called when an error has occurred on a resource. If the system has detected
             an error but it also available for a read or write, the error occurred
             callback will be called before the read is ready or write is ready callback.
             */
            virtual void pollerResourceErrorHasOccurred(Poller& p,
                                                        const PolledResource& resource) {}

            /*!
             Called when a resource has disconnected.
             */
            virtual void pollerResourceHasDisconnected(Poller& p,
                                                       const PolledResource& resource) {}
        };


        /*!
         This class is used to provide multiplexed reading and writing to IO devices.
         It is essentially a wrapper around the poll method, but you shouldn't depend
         on that. (It would be possible to provide an implementation based on select
         if necessary, but at present poll is available on all the architectures that
         are of interest to me.)
         */
        class Poller final {
        public:

            Poller();
            ~Poller() noexcept;


            Poller(Poller&& p) noexcept;
            Poller& operator=(Poller&& p) noexcept;

            Poller(const Poller&) = delete;
            Poller& operator=(const Poller&) = delete;

            /*!
             Set the delegate. This needs to be done before run() is called. If called
             while run() is still executing, undefined behaviour could result.

             Note that the delegate must remain valid throughout the life of the
             poller. (The typical way to accomplish this is if the object that
             owns the poller is also the delegate.)
             */
            void setDelegate(PollerDelegate* delegate) noexcept;

            /*!
             Add a resource to be monitored. If called while run() is still executing,
             the change will not take place until the internal poll completes and
             is called again. Note that the resources names are really for information
             only and they need not be unique if you really don't care to distinguish
             one from the other. (Or if you wanted to group them by some sort of
             class criteria.)

             @throws any exception that std::mutex handling can cause.
             @throws any exception that std::vector insertion may throw.
             */
            void add(const PolledResource& resource);

            /*!
             Remove the resource of the given name from being monitored. If called while
             run() is still executing, the change will not take place until the internal
             poll call completes and is called again. Note that removing a resource
             that does not exist is not an error, it just does nothing.

             If the resource names are not unique, then remove will remove all the
             resource settings of the given name. If desired you could use this to
             group resources by some sort of class criteria.

             @throws any exception that std::mutex handling can cause.
             */
            void remove(const std::string& resourceName);

            /*!
             Remove all the monitored resources. If called while run() is still executing,
             the change will not take place until the internal poll call completes and
             is called again. This will also cause run() to exit.
             */
            void removeAll();

            /*!
             Run the poller. This will not exit until one of the following occurs:

             - The delegate's pollerShouldStop() returned true and the internal poll
             was completed either due to a timeout or an event occuring. (In the
             latter case, the delegate callbacks will be called before pollerShouldStop()
             will be examined. Assuming that the maximum wait interval is not infinite,
             this should be the normal exit condition.

             - There are no resources to monitor. This would be unusual, but if you
             are calling add and remove during the run, it is possible. You would then
             need to manually determine when to call run again, presumably after
             adding at least one resource to monitor.

             - The internal poll call reports an EINTR signal. This would be the case
             if you have your thead configured to be interruptable and it got
             interrupted. If you really need to check for this (say because you want
             to throw an exception), you can see if (errno == EINTR) as soon as
             run() exits and before you make any system calls.

             - An error occurs during the internal poll call. In this case run will
             exit by throwing a system_error exception.

             @throws runtime_error if no delegate has been assigned or there is some other
             problem while running.
             @throws system_error if there is a problem with the internal system calls
             (typically, but not limited to, the internal poll call).
             */
            void run();

        private:
            struct Impl;
            std::unique_ptr<Impl> _impl;
        };
    }
}

#endif
