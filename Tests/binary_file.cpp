//
//  binary_file.cpp
//  kssio
//
//  Created by Steven W. Klassen on 2015-02-20.
//  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <algorithm>
#include <cstring>
#include <iostream>

#include <fcntl.h>

#include <kss/io/binary_file.hpp>
#include <kss/io/fileutil.hpp>
#include <kss/io/utility.hpp>

#include "ksstest.hpp"

using namespace std;
using namespace kss::io::file;
using namespace kss::test;

namespace {
    // A simple record for testing.
    struct srec {
        int i;
        long l;
    };
}


static TestSuite ts("file::binary_file", {
    make_pair("Constructors", [] {
        int filedes = open(temporaryFilename("/tmp/filedes").c_str(),
                           O_WRONLY | O_CREAT | O_APPEND,
                           S_IRUSR | S_IWUSR);
        FILE* fp = temporaryFile("/tmp/fp");
        FiledesGuard g1(filedes);
        FileGuard g2(fp);

        BinaryFile bf1;
        BinaryFile bf2(temporaryFilename("/tmp/bf2"), BinaryFile::writing);
        BinaryFile bf3(temporaryFilename("/tmp/bf3"), BinaryFile::writing | BinaryFile::updating);

        KSS_ASSERT(throwsException<invalid_argument>([] {
            BinaryFile(temporaryFilename("/tmp/bf4"), 200);
        }));

        BinaryFile bf5(fp);
        KSS_ASSERT(throwsException<invalid_argument>([] { BinaryFile(nullptr); }));
        BinaryFile bf6(filedes);
        KSS_ASSERT(throwsException<invalid_argument>([] { BinaryFile(-1); }));
        BinaryFile bf7(move(bf3));
        bf1 = move(bf2);

        FileOf<srec> fo1;
        FileOf<srec> fo2(temporaryFilename("/tmp/fo2"), BinaryFile::writing);
        FileOf<srec> fo3(temporaryFilename("/tmp/fo3"),
                         BinaryFile::writing | BinaryFile::updating);

        KSS_ASSERT(throwsException<invalid_argument>([] {
            FileOf<srec>(temporaryFilename("/tmp/fo4"), 0);
        }));

        FileOf<srec> fo5(fp);
        KSS_ASSERT(throwsException<invalid_argument>([] { FileOf<srec>(nullptr); }));
        FileOf<srec> fo6(filedes);
        KSS_ASSERT(throwsException<invalid_argument>([] { FileOf<srec>(-100); }));
        FileOf<srec> fo7(move(fo3));
        fo1 = move(fo2);
    }),
    make_pair("BinaryFile read/write", [] {
        const size_t nitems = 100;
        uint16_t arout[nitems];
        uint16_t arin[nitems];
        size_t nbytes = sizeof(arout);
        for (uint16_t i = 0; i < nitems; ++i) {
            arout[i] = i;
        }

        string filename = temporaryFilename("/tmp/bf");
        // read/write mode
        {
            BinaryFile bf(filename, BinaryFile::writing | BinaryFile::updating);
            KSS_ASSERT(bf.tell() == 0);
            KSS_ASSERT(bf.handle() != nullptr);
            size_t remain = nbytes;
            size_t pos = 0;
            while (remain > 0) {
                size_t numwritten = bf.write(arout+pos, remain);
                KSS_ASSERT(numwritten > 0);
                remain -= numwritten;
                pos += numwritten;
            }

            KSS_ASSERT(isEqualTo<size_t>(nbytes, [&bf] {
                bf.flush();
                return static_cast<size_t>(bf.tell());
            }));

            KSS_ASSERT(isEqualTo<size_t>(nbytes*2, [&] {
                bf.writeFully(arout, nbytes);
                bf.flush();
                return static_cast<size_t>(bf.tell());
            }));

            memset(arin, 0, nbytes);
            bf.seek(0);
            remain = nbytes;
            pos = 0;
            while (remain > 0) {
                size_t numread = bf.read(arin + pos, remain);
                KSS_ASSERT(numread > 0);
                remain -= numread;
                pos += numread;
            }
            KSS_ASSERT(!memcmp(arout, arin, nbytes));
            KSS_ASSERT(static_cast<size_t>(bf.tell()) == nbytes);

            memset(arin, 0, nbytes);
            bf.readFully(arin, nbytes);
            KSS_ASSERT(!memcmp(arout, arin, nbytes));
            KSS_ASSERT(static_cast<size_t>(bf.tell()) == (nbytes*2));

            KSS_ASSERT(throwsException<invalid_argument>([&] { bf.write(nullptr, 1); }));
            KSS_ASSERT(throwsException<invalid_argument>([&] { bf.writeFully(arout, 0); }));
            KSS_ASSERT(throwsException<invalid_argument>([&] { bf.read(nullptr, 10); }));
            KSS_ASSERT(throwsException<invalid_argument>([&] { bf.readFully(arin, 0); }));

            KSS_ASSERT(isEqualTo<size_t>((nbytes*2)+(5*sizeof(long)), [&bf] {
                for (size_t i = 0; i < 5; ++i) {
                    write<long>(bf, 1L);
                }
                return static_cast<size_t>(bf.tell());
            }));

            bf.seek((long)(nbytes*2));
            for (size_t i = 0; i < 5; ++i) {
                KSS_ASSERT(read<long>(bf) == 1L);
            }

            bf.seek(10*sizeof(uint16_t));
            uint16_t u16 = 0U;
            bf.readFully(&u16, sizeof(uint16_t));
            KSS_ASSERT(u16 == 10U);
            KSS_ASSERT(bf.tell() == (11*sizeof(uint16_t)));
            bf.move(2*sizeof(uint16_t));
            KSS_ASSERT(bf.tell() == (13*sizeof(uint16_t)));
            bf.move(-3*(long)sizeof(uint16_t));
            u16 = 0U;
            bf.readFully(&u16, sizeof(uint16_t));
            KSS_ASSERT(u16 == 10U);
        }

        // read-only mode
        {
            uint8_t bytes[100];
            BinaryFile bf(filename, BinaryFile::reading);
            KSS_ASSERT(bf.tell() == 0);
            while (!bf.eof()) {
                bf.read(bytes, 100);
            }
            KSS_ASSERT(bf.eof());
            KSS_ASSERT(static_cast<size_t>(bf.tell()) == (nbytes*2)+(5*sizeof(long)));

            bf.seek((long)nbytes);
            KSS_ASSERT(!bf.eof());
            KSS_ASSERT(static_cast<size_t>(bf.tell()) == nbytes);
            bf.readFully(arin, nbytes);
            KSS_ASSERT(!bf.eof());
            KSS_ASSERT(static_cast<size_t>(bf.tell()) == nbytes*2);
            KSS_ASSERT(!memcmp(arout, arin, nbytes));

            KSS_ASSERT(isEqualTo<size_t>(5, [&bf] {
                size_t counter = 0;
                long l;
                try {
                    while (true) {
                        bf.readFully(&l, sizeof(long));
                        ++counter;
                    }
                }
                catch (kss::io::Eof& e) {
                }
                return counter;
            }));
        }
    }),
    make_pair("FileOf read/write", [] {
        // read/write mode
        string filename = temporaryFilename("/tmp/fo");
        {
            srec sr;
            FileOf<srec> fo(filename, BinaryFile::writing | BinaryFile::updating);

            KSS_ASSERT(isEqualTo<size_t>(5, [&] {
                for (size_t i = 0; i < 5; ++i) {
                    sr.i = (int)i;
                    sr.l = (long)i;
                    fo.write(sr);
                }
                return fo.position();
            }));

            KSS_ASSERT(isEqualTo<size_t>(10, [&] {
                for (size_t i = 5; i < 10; ++i) {
                    sr.i = (int)i;
                    sr.l = (long)i;
                    fo << sr;
                }
                return fo.position();
            }));

            KSS_ASSERT(isEqualTo<size_t>(4, [&] {
                sr.i = 3;
                sr.l = -3;
                fo.write(sr, 3);
                fo.flush();
                return fo.position();
            }));

            fo.setPosition(0);
            for (size_t i = 0; i < 5; ++i) {
                sr = fo.read();
                KSS_ASSERT(sr.i == (int)i);
                KSS_ASSERT(sr.l == (i == 3 ? -3L : (long)i));
            }
            KSS_ASSERT(fo.position() == 5);

            for (size_t i = 5; i < 10; ++i) {
                fo >> sr;
                KSS_ASSERT(sr.i == (int)i && sr.l == (long)i);
            }
            KSS_ASSERT(fo.position() == 10);

            sr = fo.read(3);
            KSS_ASSERT(sr.i == 3 && sr.l == -3L);

            sr.i = 3;
            sr.l = 3L;
            fo.write(sr, 3);

            int i = 0;
            for (FileOf<srec>::input_iterator it = fo.begin(); it != fo.end(); ++it) {
                KSS_ASSERT(it->i == i && it->l == i);
                ++i;
            }
            KSS_ASSERT(i == 10);

            fo.setPosition(1);
            srec recs[5] { { 10, 10L }, { 11, 11L }, { 12, 12L }, { 13, 13L }, { 14, 14L } };
            copy(recs, recs+5, fo.obegin());

            i = 0;
            for (FileOf<srec>::input_iterator it = fo.ibegin(); it != fo.iend(); ++it) {
                KSS_ASSERT(it->i == i && it->l == i);
                ++i;
            }
            KSS_ASSERT(i == 15);
        }

        // append mode
        {
            srec sr;
            FileOf<srec> fo(filename, BinaryFile::appending | BinaryFile::updating);
            KSS_ASSERT(fo.position() == 15);

            KSS_ASSERT(isEqualTo<size_t>(16, [&] {
                sr.i = sr.l = 15;
                fo.write(sr);
                return fo.position();
            }));

            sr.i = sr.l = 16;
            fo << sr;

            int i = 0;
            for (const srec& r : fo) {
                KSS_ASSERT(r.i == i && r.l == (long)i);
                ++i;
            }

            KSS_ASSERT(fo.eof());
            KSS_ASSERT(i == 17);
        }

        // read only mode
        {
            srec sr;
            FileOf<srec> fo(filename);

            fo.setPosition(10);
            sr = fo.read();
            KSS_ASSERT(sr.i == 10 && sr.l == 10L);

            sr = fo.read(3);
            KSS_ASSERT(sr.i == 3 && sr.l == 3L);

            fo.setPosition(15);
            fo >> sr;
            KSS_ASSERT(sr.i == 15 && sr.l == 15L);

            fo.rewind();
            int i = 0;
            try {
                while (true) {
                    fo >> sr;
                    KSS_ASSERT(sr.i == i && sr.l == i);
                    ++i;
                }
            }
            catch (kss::io::Eof& e) {
            }
            KSS_ASSERT(i == 17);
        }
    })
});
