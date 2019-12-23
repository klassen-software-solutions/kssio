//
//  poller.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2017-10-02.
//  Copyright © 2017 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <atomic>
#include <cstdio>
#include <fstream>
#include <future>
#include <iostream>
#include <string>

#include <unistd.h>

#include <sys/types.h>
#include <sys/uio.h>

#include <kss/io/fileutil.hpp>
#include <kss/io/poller.hpp>
#include <kss/test/all.h>

using namespace std;
using namespace kss::io;
using namespace kss::test;

namespace {
	void log(const string& msg) {
		//cerr << msg << endl;
	}

	class MyDelegate : public PollerDelegate {
	public:

		void stop() { shouldStop = true; }

		void reset() {
			shouldStop = false;
			numBytesRead = 0;
			numBytesWritten = 0;
		}

		size_t numRead() const noexcept { return numBytesRead; }

		bool pollerShouldStop() const override { return shouldStop; }

        void pollerHasStarted(Poller&) override {
            log("poller has started");
        }

        void pollerWillStop(Poller&) override {
            log("poller will stop");
        }

		void pollerResourceReadIsReady(Poller& p, const PolledResource& resource) override {
			log("read is ready: " + resource.name);
			char buffer[100];
			auto numRead = read(resource.filedes, buffer, 100);
            if (numRead > 0) { numBytesRead += numRead; }
			while (numRead == 100) {
				numRead = read(resource.filedes, buffer, 100);
                if (numRead > 0) { numBytesRead += numRead; }
			}
            log("read so far: " + to_string(numBytesRead));
			if (numRead == 0) {
				// Indicates eof, so let's remove the resource.
                log("eof");
				p.remove(resource.name);
			}

			if (resource.name == "readTest") {
                if (resource.payload != (void*)0x0A) {
                    throw runtime_error("wrong payload");
                }
			}
		}

		void pollerResourceWriteIsReady(Poller& p, const PolledResource& resource) override {
			log("write is ready: " + resource.name);
			const string mydata = "this is a test";
			size_t remain = mydata.length();
			ssize_t pos = 0;
			while (remain > 0) {
				auto numWritten = write(resource.filedes, mydata.data()+pos, remain);
				pos += numWritten;
				remain -= (size_t)numWritten;
			}
		}

		void pollerResourceErrorHasOccurred(Poller& p, const PolledResource& resource) override {
			log("error on: " + resource.name);
		}

		void pollerResourceHasDisconnected(Poller& p, const PolledResource& resource) override {
			log("disconnect on: " + resource.name);
		}

	private:
		bool    shouldStop { false };
		size_t  numBytesRead { 0 };
		size_t  numBytesWritten { 0 };
	};
}


static TestSuite ts("poller", {
    make_pair("basic tests", [] {
        Poller p;
        KSS_ASSERT(throwsException<runtime_error>([&] { p.run(); }));

        MyDelegate d;
        p.setDelegate(&d);
        d.stop();
        p.run();
        KSS_ASSERT(true);    // Nothing to check other than it exited.
    }),
    make_pair("file read test", [] {
        static const string mydata = "this is my write test data";
        static constexpr size_t numWrites = 5;

        KSS_ASSERT(isEqualTo<size_t>(mydata.length() * numWrites, [] {
            // Setup the file.
            file::FileGuard fg(tmpfile());
            log("before write");
            for (size_t i = 0; i < numWrites; ++i) {
                fwrite(mydata.data(), mydata.size(), 1, fg.file());
            }
            fflush(fg.file());
            rewind(fg.file());
            log("after write");

            Poller p;
            MyDelegate d;
            p.setDelegate(&d);

            PolledResource r;
            r.name = "readTest";
            r.filedes = fileno(fg.file());
            r.event = PolledResource::Event::read;
            r.payload = reinterpret_cast<void*>(0x0A);
            p.add(r);

            auto fut = async([&] {
                p.run();
                return d.numRead();
            });
            return fut.get();
        }));
    })
});
