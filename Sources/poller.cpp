//
//  poller.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2017-10-02.
//  Copyright © 2017 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <functional>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <poll.h>
#include <syslog.h>

#include "_contract.hpp"
#include "poller.hpp"

using namespace std;
using namespace kss::io;

namespace contract = kss::io::_private::contract;

using std::chrono::milliseconds;


///
/// MARK: Internal Utilities
///

namespace {
	// Convert our event enumeration into the pollfd events bitmask.
	short eventsFromResourceEvent(PolledResource::Event ev) noexcept {
		switch (ev) {
			case PolledResource::Event::read:	return POLLIN;
			case PolledResource::Event::write:	return POLLOUT;
			case PolledResource::Event::any:	return (POLLIN | POLLOUT);
		}
        // should never get here
        assert(false);
        return 0;
	}

	// Convert our interval into the timeout integer needed by poll.
	int timeoutFromInterval(milliseconds ms) noexcept {
		if (ms == milliseconds::zero()) {
			return 0;			// poll() special value indicating no waiting.
		}
		else if (ms == milliseconds::max()) {
			return -1;			// poll() special value indicating wait forever.
		}
		else {
			// Since we can only return an int, and milliseconds::value may be larger
			// we need to do a bounds check before we can safely cast. If we fail the
			// bounds check we return the largest value we can (i.e. the value closest
			// to what the user requested).
			const auto count = ms.count();
			if (count > numeric_limits<int>::max()) {
				return numeric_limits<int>::max();
			}
			return static_cast<int>(count);
		}
	}

    // "borrowed" from kssutil
    template <class Container, class UnaryPredicate>
    void eraseIf(Container& c, UnaryPredicate pred) {
        c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
    }

}

///
/// MARK: Poller::Impl Implementation
///

struct Poller::Impl {
	Poller*			parent { nullptr };
	PollerDelegate*	delegate { nullptr };

	// Note that the set of resources to be monitored must be protected by a
	// mutex to handle add and remove calls while run is still executing.
	vector<PolledResource> 	resources;
	bool					resourcesHaveChanged { false };
	mutex	 				resourceLock;

	// Trigger the callbacks for a single resource.
	void triggerCallbacksForResource(const struct pollfd& fd,
                                     const PolledResource& resource) noexcept
    {
        if (fd.revents & POLLERR)	{ fireErrorHasOccurred(resource); }
        if (fd.revents & POLLHUP)	{ fireHasDisconnected(resource); }
        if (fd.revents & POLLIN)	{ fireReadIsReady(resource); }
        if (fd.revents & POLLOUT)	{ fireWriteIsReady(resource); }
	}

	// Handle the results of a poll call. Returns true if we should keep polling and false
	// if we should exit the main loop.
	bool handlePollResult(int res,
                          const vector<struct pollfd>& fds,
                          const vector<PolledResource>& currentResources)
    {
		// Check for the exceptional conditions.
		if (res == -1) {
			assert(errno != 0);
			switch (errno) {
				case EAGAIN:
					// Not a problem, we log a warning and try again.
					syslog(LOG_WARNING, "Internal allocation failure, in %s. Trying again.", __func__);
					return true;

				case EINTR:
					// Not a problem, but we exit the loop and hence the run, after logging
					// the fact that we were interrupted.
					syslog(LOG_INFO, "Poll was interrupted, in %s.", __func__);
					return false;

				default:
					// Problem. Throw a system_error exception.
					throw system_error(error_code(errno, system_category()), "poll");
			}
		}

		// Handle the case where we timed out with no events occurring. We do nothing but
		// try again.
		else if (res == 0) {
			return true;;
		}

		// Trigger the callbacks.
		else {
			const auto len = fds.size();
			for (size_t i = 0; i < len; ++i) {
				triggerCallbacksForResource(fds[i], currentResources[i]);
			}
			return true;
		}
	}

	// Wrap any exceptions in a syslog and call the delegate.
	void firePollerCallback(const function<void(Poller&)>& cb) noexcept {
        try {
            cb(*parent);
        }
        catch (const exception& e) {
            syslog(LOG_ERR, "Error in poller callback, exception=%s", e.what());
        }
	}

	inline void fireHasStarted() noexcept {
        firePollerCallback([&](Poller& p){ delegate->pollerHasStarted(p); });
    }
	inline void fireWillStop() noexcept {
        firePollerCallback([&](Poller& p) { delegate->pollerWillStop(p); });
    }

	void fireResourceCallback(const PolledResource& resource,
                              const function<void(Poller&, const PolledResource&)> cb)& noexcept
    {
        try {
            cb(*parent, resource);
        }
        catch (const exception& e) {
            syslog(LOG_ERR, "Error with poller resource callback, resource=%s, exception=%s",
                   resource.name.c_str(), e.what());
        }
	}

	inline void fireReadIsReady(const PolledResource& resource) {
		fireResourceCallback(resource, [&](Poller& p, const PolledResource& r) {
			delegate->pollerResourceReadIsReady(p, r);
		});
	}

	inline void fireWriteIsReady(const PolledResource& resource) {
		fireResourceCallback(resource, [&](Poller& p, const PolledResource& r) {
			delegate->pollerResourceWriteIsReady(p, r);
		});
	}

	inline void fireErrorHasOccurred(const PolledResource& resource) {
		fireResourceCallback(resource, [&](Poller& p, const PolledResource& r) {
			delegate->pollerResourceErrorHasOccurred(p, r);
		});
	}

	inline void fireHasDisconnected(const PolledResource& resource) {
		fireResourceCallback(resource, [&](Poller& p, const PolledResource& r) {
			delegate->pollerResourceHasDisconnected(p, r);
		});
	}
};


///
/// MARK: Poller Implementation
///

Poller::Poller() : _impl(new Impl()) {
	_impl->parent = this;

    contract::postconditions({
        KSS_EXPR(_impl->parent == this),
        KSS_EXPR(_impl->delegate == nullptr),
        KSS_EXPR(_impl->resources.empty()),
        KSS_EXPR(_impl->resourcesHaveChanged == false)
    });
}

Poller::~Poller() noexcept = default;

Poller::Poller(Poller&& p) noexcept : _impl(move(p._impl)) {
	_impl->parent = this;

    contract::postconditions({
        KSS_EXPR(_impl->parent == this),
        KSS_EXPR(!p._impl)
    });
}

Poller& Poller::operator=(Poller&& p) noexcept {
    contract::preconditions({
        KSS_EXPR(_impl->parent == this)
    });

    if (&p != this) {
        _impl = move(p._impl);
        _impl->parent = this;
    }

    contract::postconditions({
        KSS_EXPR(_impl->parent == this),
        KSS_EXPR(!p._impl)
    });
	return *this;
}

void Poller::setDelegate(kss::io::PollerDelegate *delegate) noexcept {
    contract::preconditions({
        KSS_EXPR(_impl->parent == this)
    });

	_impl->delegate = delegate;

    contract::postconditions({
        KSS_EXPR(_impl->parent == this),
        KSS_EXPR(_impl->delegate == delegate)
    });
}


void Poller::add(const kss::io::PolledResource &resource) {
    contract::preconditions({
        KSS_EXPR(_impl->parent == this)
    });

    lock_guard<mutex> lock(_impl->resourceLock);
    _impl->resources.push_back(resource);
    _impl->resourcesHaveChanged = true;

    contract::postconditions({
        KSS_EXPR(_impl->parent == this),
        KSS_EXPR(!_impl->resources.empty()),
        KSS_EXPR(_impl->resourcesHaveChanged == true)
    });
}


void Poller::remove(const string &resourceName) {
    contract::preconditions({
        KSS_EXPR(_impl->parent == this)
    });

    lock_guard<mutex> lock(_impl->resourceLock);
    const auto n = _impl->resources.size();
    eraseIf(_impl->resources, [&](const PolledResource& resource) {
        return (resource.name == resourceName);
    });

    if (_impl->resources.size() != n) {
        _impl->resourcesHaveChanged = true;
    }

    contract::postconditions({
        KSS_EXPR(_impl->parent == this)
    });
}


void Poller::removeAll() {
    contract::preconditions({
        KSS_EXPR(_impl->parent == this)
    });

	lock_guard<mutex> lock(_impl->resourceLock);
	_impl->resources.clear();
	_impl->resourcesHaveChanged = true;

    contract::postconditions({
        KSS_EXPR(_impl->parent == this),
        KSS_EXPR(_impl->resources.empty()),
        KSS_EXPR(_impl->resourcesHaveChanged == true)
    });
}

void Poller::run() {
    contract::preconditions({
        KSS_EXPR(_impl->parent == this)
    });

	if (!_impl->delegate) {
		throw runtime_error("No delegate has been assigned.");
	}
	_impl->fireHasStarted();

	vector<struct pollfd> fds;
	vector<PolledResource> currentResources;

	while (!_impl->delegate->pollerShouldStop()) {

		// If our current resources are out of date, we need to update them now.
		if (_impl->resourcesHaveChanged) {
			{
				lock_guard<mutex> lock(_impl->resourceLock);
				currentResources = _impl->resources;
				_impl->resourcesHaveChanged = false;
			}

			fds.resize(currentResources.size());
			const auto len = currentResources.size();
			for (size_t i = 0; i < len; ++i) {
				const auto& r = currentResources[i];
				fds[i].fd = r.filedes;
				fds[i].events = eventsFromResourceEvent(r.event);
			}
		}

		// Setup for the poll call. Note that if there are no resources to examine,
		// we exit the loop.
		if (fds.empty()) {
			break;
		}
		else {
			for (auto& fd : fds) {
				fd.revents = 0;
			}
		}

		// There is a possibility that the user may have attempted to add more resources to
		// monitor than possible file descriptors. This could lead to a potential security
		// issue so we explicitly check for that condition and throw an exception.
		if (fds.size() > numeric_limits<nfds_t>::max()) {
			throw runtime_error("Too many resources have been added. "
                                "This could represent an attempt to cause "
                                "an overflow or underflow.");
		}
		const auto fdsize = static_cast<nfds_t>(fds.size());

		// Execute the poll and examine the results.
		errno = 0;
		const auto res = poll(fds.data(), fdsize, timeoutFromInterval(_impl->delegate->pollerMaximumWaitInterval()));
		if (!_impl->handlePollResult(res, fds, currentResources)) {
			break;
		}
	}

	_impl->fireWillStop();
}
