 /*---------------------------------------------------------------------------\
 |                                                                            |
 |  bz2stream -- C++ Stream Classes for the BZ2 compression algorithm.        |
 |  Copyright (C) 2002 Aaron Isotton.                                         |
 |                                                                            |
 |  This program is free software; you can redistribute it and/or modify      |
 |  it under the terms of the GNU General Public License as published by      |
 |  the Free Software Foundation; either version 2 of the License, or         |
 |  (at your option) any later version.                                       |
 |                                                                            |
 |  This program is distributed in the hope that it will be useful,           |
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of            |
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             |
 |  GNU General Public License for more details.                              |
 |                                                                            |
 |  You should have received a copy of the GNU General Public License         |
 |  along with this program; if not, write to the Free Software               |
 |  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  |
 |                                                                            |
 |  ------------------------------------------------------------------------  |
 |                                                                            |
 |  The home page of this software is http://www.isotton.com/bz2stream/.      |
 |  You can contact me anytime at <aaron@isotton.com>.                        |
 |                                                                            |
 \---------------------------------------------------------------------------*/

// $Id: bz2stream.h 15968 2013-04-09 13:49:51Z sloot $

#ifndef BZ2STREAM_BZ2STREAM_HPP
#define BZ2STREAM_BZ2STREAM_HPP

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <streambuf>
#include <vector>

#include <bzlib.h>

/// \mainpage Bz2 Stream Classes
/// bz2stream is a set of four classes making it easy to use the bz2
/// compression algorithm from C++.  The four classes are two stream
/// buffers -- one for compression and one for decompression -- and
/// two streams -- one for input and one for output.
///
/// The input stream uses the decompression stream buffer, and the
/// output stream uses the compression stream buffer; this makes it
/// possible to read from a bz2 input stream just as you would read
/// from std::cin or from a std::ifstream, and to write to a bz2
/// output stream just the way you would write to any other output
/// stream, like std::cout or a std::ofstream without worrying about
/// compression and decompression at all.
///
/// The streams do not write the data to any default destination, but
/// to another stream buffer.  This gives you the maximun possible
/// flexibility.  You just create a "normal" input or output stream,
/// and then use pass its stream buffer to the bz2 stream.  The bz2
/// stream acts as a filter, compressing or decompressing all the data
/// you pass it.
///
/// For example, to read a file, compress it and write it to another
/// file a program as the following would be enough:
/// \code
/// #include "bz2stream.hpp"
/// #include <iostream>
/// #include <fstream>
///
/// int main(int argc, char** argv) {
///     std::ifstream infile(argv[1], std::ios::binary);
///     std::ofstream outfile(argv[2], std::ios::binary);
///     bz2ostream out(outfile.rdbuf());
///     out << infile.rdbuf();
/// }
/// \endcode
///
/// The performance loss because of the use of the stream classes and
/// not of direct calls to libbzip2's API is virtually non existent.
/// This small program is as fast as bzip2.  There is a bit a more
/// extensive test program included in the archive.
///
/// Notice that the stream buffer you pass to the ::bz2istream or
/// ::bz2ostream constructor must be in binary mode, because the
/// compressed data is binary and the conversions which happen in text
/// mode will corrupt it.
///
/// The use of the bz2 stream classes is especially easy because there
/// are no libraries you need to link to; you only need to include the
/// bz2stream.hpp header.  (Of course you must link against libbz2).
///
/// Notice that this is an early release; this code is tested, but not
/// ready for applications where absolute reliability is a must.

/// \file bz2stream.hpp
/// This file declares and defines all of the bz2stream class set.  It
/// is currently the only source code file.

extern "C" {
    /// \brief A pointer to a custom memory allocation function, to be
    /// used by libbzip2.
    ///
    /// The opaque pointer specified in the class
    /// constructor is passed as first parameter.  The function is
    /// expected to return a pointer to n * m bytes of memory.  Notice
    /// that this function should be declared as 'extern "C"'.
    typedef void* (*bzalloc_ptr)(void* opaque, int n, int m);

    /// \brief A pointer to a custom memory deallocation function, to
    /// be used by libbzip2.
    ///
    /// The opaque pointer specified in the class
    /// constructor is passed as first parameter.  The function is
    /// expected to free the memory pointed to by p.  Notice that this
    /// function should be declared as 'extern "C"'.
    typedef void (*bzfree_ptr)(void* opaque, void* p);
}

/// \brief A stream buffer which takes any form of input, compresses
/// it using the bz2 algorithm and writes it to another stream buffer.
///
/// There is one special aspect of this class.  The bz2 algorithm
/// works on blocks of data; it waits until a block is full and then
/// compresses it.  As long as you only feed data to the stream
/// without flushing it this works normally.  If you flush the stream,
/// you force a premature end of the data block; this will cause a
/// worse compression factor.  You should avoid flushing a bz2outbuf
/// buffer at all costs.
class bz2outbuf : public std::streambuf {
protected:
    std::streambuf* dest;
    std::vector<char> buffer;
    std::vector<char> out_buffer;
    bz_stream cstream;

    bool process_block() {
        int num = pptr() - pbase();
        cstream.next_in = pbase();
        cstream.avail_in = num;

        while (cstream.avail_in) {
            // set the pointers to the output buffer
            cstream.next_out = &out_buffer[0];
            cstream.avail_out = out_buffer.size();

            // compress the data; we don't need to check for errors
            // here because there is no possibility that they can
            // happen here; it is explained in the libbzip2 docs.
            BZ2_bzCompress(&cstream, BZ_RUN);

            // write the data to the underlying stream buffer
            int out_size = out_buffer.size() - cstream.avail_out;
            if (dest->sputn(&out_buffer[0], out_size) != out_size)
                return false;
        }

        pbump(-num);
        return true;
    }

    bool flush() {
        bool flushed = false;
        cstream.next_in = NULL;
        cstream.avail_in = 0;

        do {
            // set the pointers to the output buffer
            cstream.next_out = &out_buffer[0];
            cstream.avail_out = out_buffer.size();

            // here no errors should happen, too
            flushed = BZ2_bzCompress(&cstream, BZ_FLUSH) == BZ_RUN_OK;

            // write the data to the underlying stream buffer
            int out_size = out_buffer.size() - cstream.avail_out;
            if (dest->sputn(&out_buffer[0], out_size) != out_size)
                return false;
        } while (!flushed);

        return true;
    }

    bool finish() {
        bool finished = false;
        cstream.next_in = NULL;
        cstream.avail_in = 0;

        do {
            // set the pointers to the output buffer
            cstream.next_out = &out_buffer[0];
            cstream.avail_out = out_buffer.size();

            // here no errors should happen, too
            finished = BZ2_bzCompress(&cstream, BZ_FINISH) == BZ_STREAM_END;

            // write the data to the underlying stream buffer
            int out_size = out_buffer.size() - cstream.avail_out;
            if (dest->sputn(&out_buffer[0], out_size) != out_size)
                return false;
        } while (!finished);

        return true;
    }

    virtual int_type overflow(int_type c) {
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            // put this character in the last position
            // (this function is called when pptr() == eptr() but we
            // have reserved one byte more in the constructor, thus
            // *epptr() and now *pptr() point to valid positions)
            *pptr() = c;
            pbump(1);
        }

        return process_block() ? traits_type::not_eof(c) : traits_type::eof();
    }

    virtual int sync() {
        return process_block() && flush() ? 0 : -1;
    }

    virtual std::streamsize xsputn(const char* p, std::streamsize num) {
        // the number of chars copied
        std::streamsize done = 0;

        // loop until no data is left
        while (done < num) {
            // get the number of chars to write in this iteration
            // (we've got one more char than epptr thinks)
            int block_size = std::min(num - done, epptr() - pptr() + 1);

            // write them
            std::memcpy(pptr(), p + done, block_size);

            // update the write pointer
            pbump(block_size);

            // process_block if necessary
            if (pptr() >= epptr()) {
                if (!process_block())
                    break;
            }

            // update the yet-to-do count
            done += block_size;
        }

        return done;
    }
public:
    /// \brief Constructs a new bz2outbuf object.
    ///
    /// @param _dest The stream buffer to write the compressed data
    /// to.  Notice that this stream buffer must be in binary and not
    /// in text mode, because if the newline characters are processed
    /// the data will be corrupted.
    ///
    /// @param block_size_100K The size of the blocks in 100K the
    /// bz2 algorithm should compress.  The bigger the blocks are, the
    /// slower and the more memory intensive the compression gets, but
    /// the data will be compressed more.  Valid values range from 1
    /// to 9.
    ///
    /// @param verbostity The amount of debugging information libbz2
    /// should print to stderr.  Ranges from 0 (quiet) to 4 (very
    /// talkative).
    ///
    /// @param work_factor The "work factor" of the compression
    /// algorithm.  It is described in the libbz2 documentation; leave
    /// it 0 if you don't know what you're doing.  Ranges from 1 to
    /// 250; 0 is a special value which will be internally replaced by
    /// the default value of 30.
    ///
    /// @param bzalloc A pointer to a custom memory allocation
    /// routine, to be used by libbz2.  If NULL, malloc() will be
    /// used.  See also ::bzalloc_ptr.
    ///
    /// @param bzfree A pointer to a custom memory deallocation
    /// routine, to be used by libbz2.  If NULL, free() will be used.
    /// See also ::bzfree_ptr.
    ///
    /// @param opaque A pointer which will be passed to the custom
    /// routines bzalloc and bzfree, if you specified them.  See also
    /// ::bzalloc_ptr and ::bzfree_ptr.
    ///
    /// @param buffer_size The size of the stream buffer used by
    /// bz2outbuf.  Leave it alone if you don't know what you're
    /// doing.  (Bigger values are not necessarily faster).  Must be
    /// positive.
    ///
    /// @param out_buffer_size The size of the output buffer used by
    /// bz2outbuf.  Definitely leave it alone if you don't know what
    /// you're doing.  Must be positive, too.
    ///
    /// Currently, this constructor throws any of the following
    /// exceptions:
    ///
    /// - std::range_error if one of the parameters is out of range.
    /// - std::bad_alloc if it runs out of memory.
    /// - std::runtime_error for other errors.
    ///
    /// For future compatibility, you should expect it to throw any
    /// std::exception derived exception.  All exceptions but
    /// std::bad_alloc include descriptions of what went bad.  Thus
    /// using the what() member makes sense.
    bz2outbuf(std::streambuf* _dest, unsigned int block_size_100K = 9,
              unsigned int verbosity = 0, unsigned int work_factor = 0,
              bzalloc_ptr bzalloc = NULL, bzfree_ptr bzfree = NULL,
              void* opaque = NULL, size_t stream_buffer_size = 2048,
              size_t out_buffer_size = 2048)
        : dest(_dest)
    {
        // check the parameters
        if (block_size_100K > 9)
            throw std::range_error("Block size out of range.");
        if (verbosity > 4)
            throw std::range_error("Verbosity level out of range.");
        if (work_factor > 250)
            throw std::range_error("Work factor out of range.");
        if (stream_buffer_size < 1)
            throw std::range_error("Stream buffer size must be positive.");
        if (out_buffer_size < 1)
            throw std::range_error("Output buffer size must be positive.");

        // allocate the buffer (we don't do that in the initializer
        // because if one of the parameters is out of range it isn't
        // necessary to allocate at all)
        buffer.resize(stream_buffer_size);

        // allocate memory for the output buffer
        out_buffer.resize(out_buffer_size);

        // set the buffer pointers; use one character less for the
        // stream buffer than the really available one
        setp(&buffer[0], &*--buffer.end());

        // initialize the compressor stream
        memset(&cstream, 0, sizeof(cstream));
        cstream.bzalloc = bzalloc;
        cstream.bzfree = bzfree;
        cstream.opaque = opaque;

        // create a bz2 compressor stream
        int ret = BZ2_bzCompressInit(&cstream, block_size_100K, verbosity,
                                     work_factor);
        // BZ_PARAM_ERROR won't happen here because we checked before
        switch (ret) {
        case BZ_OK:
            break;
        case BZ_CONFIG_ERROR:
            throw std::runtime_error("libbz2 was not compiled correctly.");
        case BZ_MEM_ERROR:
            throw std::bad_alloc();
        default:
            throw std::runtime_error("Unknown error creating bz2 compressor "
                                     "stream buffer.");
        }
    }

    /// Flush the buffer and destroy the object.  Notice that there is
    /// no way for the destructor to report a failure to write to the
    /// underlying stream buffer.  Thus you might want to check the
    /// underlying stream buffer for errors \em after the bz2outbuf
    /// object has been destroyed.
    virtual ~bz2outbuf() {
        // finish compression
        process_block();
        finish();

        // delete the compressor stream
        BZ2_bzCompressEnd(&cstream);
    }
};

/// \brief An output stream compressing all data with the bz2
/// algorithm.
///
/// As the actual compression is achieved in the underlying
/// ::bz2outbuf stream buffer, you should read its documentation
/// before using this class.
///
/// A bz2ostream object writes the output data to a stream buffer you
/// specify in the constructor; it can thus be used to write to any
/// stream buffer you want.
class bz2ostream : public std::ostream {
protected:
    bz2outbuf buf;
public:
    /// \brief Creates a new bz2ostream object.  See
    /// bz2outbuf::bz2outbuf for an explanation of the parameters.
    bz2ostream(std::streambuf* dest, unsigned int block_size_100K = 9,
               unsigned int verbosity = 0, unsigned int work_factor = 0,
               bzalloc_ptr bzalloc = NULL, bzfree_ptr bzfree = NULL,
               void* opaque = NULL, size_t buffer_size = 1024,
               size_t out_buffer_size = 1024)
        : std::ostream(&buf),
        buf(dest, block_size_100K, verbosity, work_factor, bzalloc, bzfree,
            opaque, buffer_size, out_buffer_size)
    {}
};

/// \brief A stream buffer reading from another stream buffer and
/// decompressing the read data using the bz2 algorithm.
///
/// You specify the underlying "source" stream buffer in the
/// constructor.
class bz2inbuf : public std::streambuf {
protected:
    std::streambuf* source;
    std::vector<char> buffer;
    char* putback_end;
    std::vector<char> in_buffer;
    char* in_begin;
    char* in_end;
    bz_stream dstream;

    virtual int_type underflow() {
        // calculate the new size of the putback area
        int new_putback_num = std::min(gptr() - eback(),
                                       putback_end - &buffer[0]);

        // copy the new putback data into the putback area
        std::memcpy(putback_end - new_putback_num,
                    gptr() - new_putback_num, new_putback_num);

        // shovel data into the bzip stream until there is something
        // in the output buffer
        do {
            // refill the input buffer if necessary
            if (in_begin == in_end) {
                std::streamsize read_num = source->sgetn(&in_buffer[0],
                                               in_buffer.size());
                if (read_num == 0) {
                    // we can't read anymore
                    return traits_type::eof();
                }
                in_begin = &in_buffer[0];
                in_end = in_begin + read_num;
            }

            // decompress the data
            dstream.next_in = in_begin;
            dstream.avail_in = in_end - in_begin;
            dstream.next_out = putback_end;
            dstream.avail_out = &*buffer.end() - putback_end;
            int ret = BZ2_bzDecompress(&dstream);
            switch (ret) {
            case BZ_OK:
                break;
            case BZ_STREAM_END:
                if (&*buffer.end() - putback_end == (int) dstream.avail_out)
                    return traits_type::eof();
                break;
            case BZ_DATA_ERROR:
            case BZ_DATA_ERROR_MAGIC:
            case BZ_MEM_ERROR:
            default:
                // TODO: handle these errors separately
                return traits_type::eof();
            }

            // update the input buffer pointers
            in_begin = in_end - dstream.avail_in;

        } while (dstream.avail_out + putback_end == &*buffer.end());

        // update the stream buffer pointers
        setg(putback_end - new_putback_num,
             putback_end,
             &*buffer.end() - dstream.avail_out);

        // return the next character
        return traits_type::to_int_type(*gptr());
    }

public:
    /// \brief Creates a new bz2inbuf object, using _source as
    /// underlying stream buffer.
    ///
    /// @param _source The stream buffer this object should read
    /// from.  Notice that this stream buffer must be in binary and
    /// not in text mode, because transformation of newlines and/or
    /// other characters would invariably lead to data corruption.
    ///
    /// @param verbosity The verbosity of the libbzip2.  Ranges from 0
    /// (quiet) to 4 (very verbose).  Messages are printed to stderr.
    ///
    /// @param small_but_slow If false, use default algorithm.  If
    /// true, use a less memory intensive but slower algorithm.  Not
    /// recommended.
    ///
    /// @param bzalloc A pointer to a custom memory allocation
    /// function.  If NULL, malloc() is used. See also ::bzalloc_ptr.
    ///
    /// @param bzfree A pointer to a custom memory deallocation
    /// function.  If NULL, free() is used.  See also ::bzfree_ptr.
    ///
    /// @param opaque A pointer to be passed to the custom memory
    /// allocation and deallocation functions.  See also ::bzalloc_ptr
    /// and ::bzfree_ptr.
    ///
    /// @param stream_buffer_size The size of the buffer of this
    /// stream buffer.  Leave it alone if you don't know what you're
    /// doing.  Must be greater than zero.  Bigger buffers are not
    /// always faster.
    ///
    /// @param in_buffer_size The size of the input buffer.
    /// Definitely leave it alone if you don't know what you're
    /// doing.  Must be greater than zero, and \e should not be
    /// greater than stream_buffer_size.
    ///
    /// @param max_putback_size The maximum size of the stream's
    /// putback area.  Ranges from 0 to stream_buffer_size - 1. Leave
    /// it alone if you don't know what you're doing.
    ///
    /// Currently, this constructor throws any of the following
    /// exceptions:
    ///
    /// - std::range_error if one of the arguments is out of range.
    /// - std::bad_alloc if it runs out of memory.
    /// - std::runtime_error for any other error.
    ///
    /// All thrown exceptions except std::bad_alloc include a
    /// description of the encountered problem which you can access
    /// using the what() member function.
    ///
    /// For future compatibility, expect this constructor to throw any
    /// std::exception derived exception.
    bz2inbuf(std::streambuf* _source, unsigned int verbosity = 0,
             bool small_but_slow = false, bzalloc_ptr bzalloc = NULL,
             bzfree_ptr bzfree = NULL, void* opaque = NULL,
             size_t stream_buffer_size = 1024, size_t in_buffer_size = 1024,
             size_t max_putback_size = 64)
        : source(_source)
    {
        // check the parameters
        if (verbosity > 4)
            throw std::range_error("Verbosity level out of range.");
        if (stream_buffer_size < 1)
            throw std::range_error("Stream buffer size must be positive.");
        if (in_buffer_size < 1)
            throw std::range_error("Input buffer size must be positive.");
        if (max_putback_size >= stream_buffer_size)
            throw std::range_error("The maximum size of the putback area must "
                                   "be less than the stream size.");

        // allocate the buffers
        buffer.resize(stream_buffer_size);
        in_buffer.resize(in_buffer_size);
        in_begin = in_end = &in_buffer[0];

        // set the get pointers (we force an underflow on the first
        // read)
        putback_end = &buffer[0] + max_putback_size;
        setg(putback_end, putback_end, putback_end);

        // set up a decompressor stream
        std::memset(&dstream, 0, sizeof(dstream));
        dstream.bzalloc = bzalloc;
        dstream.bzfree = bzfree;
        dstream.opaque = opaque;

        // init a decompressor stream
        int ret = BZ2_bzDecompressInit(&dstream, verbosity,
                                       small_but_slow ? 1 : 0);
        // we don't need to handle BZ_PARAM_ERROR because we've
        // already checked the parameters
        switch (ret) {
        case BZ_OK:
            break;
        case BZ_CONFIG_ERROR:
            throw std::runtime_error("libbz2 was not compiled correctly.");
        case BZ_MEM_ERROR:
            throw std::bad_alloc();
        default:
            throw std::runtime_error("Unknow error creating bz2 decompressor "
                                     "stream buffer.");
        }
    }

    /// \brief Destroys the bz2inbuf object.
    ~bz2inbuf() {
        // uninit the bz2 stream
        BZ2_bzDecompressEnd(&dstream);
    }
};

/// \brief An input stream decompressing data compressed with the bz2
/// algorithm.
///
/// The actual decompression is achieved in the underlying ::bz2inbuf
/// stream buffer; you should read its documentation before using this
/// class.
///
/// The data is read from a supplied stream buffer.
class bz2istream : public std::istream {
protected:
    bz2inbuf buf;
public:
    /// \brief Creates a new bz2istream, using source as the stream
    /// buffer to read data from.
    ///
    /// See bz2inbuf::bz2inbuf for an explanation of the parameters.
    bz2istream(std::streambuf* source, unsigned int verbosity = 0,
               bool small_but_slow = false, bzalloc_ptr bzalloc = NULL,
               bzfree_ptr bzfree = NULL, void* opaque = NULL,
               size_t buffer_size = 1024, size_t in_buffer_size = 1024,
               size_t max_putback_size = 64)
        : std::istream(&buf),
          buf(source, verbosity, small_but_slow, bzalloc, bzfree, opaque,
              buffer_size, in_buffer_size, max_putback_size)
    {}
};

#endif // !BZ2STREAM_BZ2STREAM_HPP
