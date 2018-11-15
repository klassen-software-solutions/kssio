//
//  poller.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2017-10-02.
//  Copyright © 2017 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include <unistd.h>

#include <sys/types.h>
#include <sys/uio.h>

#include <kss/io/fileutil.hpp>
#include <kss/io/poller.hpp>

#include "ksstest.hpp"

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

		ssize_t numRead() const noexcept { return numBytesRead; }

		virtual bool pollerShouldStop() const { return shouldStop; }

		virtual void pollerResourceReadIsReady(Poller& p, const PolledResource& resource) {
			log("read is ready: " + resource.name);
			char buffer[100];
			auto numRead = read(resource.filedes, buffer, 100);
			if (numRead > 0) numBytesRead += numRead;
			while (numRead == 100) {
				numRead = read(resource.filedes, buffer, 100);
				if (numRead > 0) numBytesRead += numRead;
			}
			if (numRead == 0) {
				// Indicates eof, so let's remove the resource.
				p.remove(resource.name);
			}

			if (resource.name == "readTest") {
                if (resource.payload != (void*)0x0A) {
                    throw runtime_error("wrong payload");
                }
			}
		}

		virtual void pollerResourceWriteIsReady(Poller& p, const PolledResource& resource) {
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

		virtual void pollerResourceErrorHasOccurred(Poller& p, const PolledResource& resource) {
			log("error on: " + resource.name);
		}

		virtual void pollerResourceHasDisconnected(Poller& p, const PolledResource& resource) {
			log("disconnect on: " + resource.name);
		}

	private:
		bool shouldStop { false };
		ssize_t numBytesRead { 0 };
		ssize_t numBytesWritten { 0 };
	};
}


static TestSuite ts("io::poller", {
    make_pair("basic tests", [](TestSuite&) {
        Poller p;
        KSS_ASSERT(throwsException<runtime_error>([&] { p.run(); }));

        MyDelegate d;
        p.setDelegate(&d);
        d.stop();
        p.run();
        KSS_ASSERT(true);    // Nothing to check other than it exited.
    }),
    make_pair("file read test", [](TestSuite&) {
        // Setup the file.
        file::FileGuard fg(tmpfile());

        Poller p;
        MyDelegate d;
        p.setDelegate(&d);

        PolledResource r;
        r.name = "readTest";
        r.filedes = fileno(fg.file());
        r.event = PolledResource::Event::read;
        r.payload = reinterpret_cast<void*>(0x0A);
        p.add(r);

        thread th { [&] {
            p.run();
        }};

        const string mydata = "this is my write test data";
        fwrite(mydata.data(), mydata.size(), 1, fg.file());
        fwrite(mydata.data(), mydata.size(), 1, fg.file());
        fwrite(mydata.data(), mydata.size(), 1, fg.file());
        fwrite(mydata.data(), mydata.size(), 1, fg.file());
        fwrite(mydata.data(), mydata.size(), 1, fg.file());
        fflush(fg.file());
        rewind(fg.file());

        th.join();
        KSS_ASSERT(d.numRead() == mydata.size() * 5);
    })
});
