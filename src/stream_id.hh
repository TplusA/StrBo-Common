/*
 * Copyright (C) 2016  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of the T+A Streaming Board software stack ("StrBoWare").
 *
 * StrBoWare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3 as
 * published by the Free Software Foundation.
 *
 * StrBoWare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StrBoWare.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STREAM_ID_HH
#define STREAM_ID_HH

#include <iostream>

#include "stream_id.h"

namespace ID
{

/*!
 * Representation of a structured stream ID.
 *
 * Streams IDs are composed of two parts: the source and the cookie. The source
 * part identifies the origin of the stream, i.e., which software component has
 * sent the stream to the player. The cookie part is a serial number that
 * allows the source to distinguish various streams from each other.
 *
 * Any software component that wants to start playing a stream
 * * must know about the ID structure;
 * * must have a unique source ID assigned; and
 * * must use their source ID in the IDs they are assigning to streams.
 *
 * By design, not all software components need to know about the structure of a
 * stream ID. Those that don't know treat the ID as an opaque integer value and
 * usually only store them away and pass them around (e.g., streamplayer does
 * this).
 */
class Stream
{
  private:
    stream_id_t id_;

    constexpr explicit Stream(stream_id_t raw_id) throw():
        id_(raw_id)
    {}

  public:
    constexpr static inline Stream make_complete(stream_id_t source,
                                                 stream_id_t cookie) throw()
    {
        return Stream((source & STREAM_ID_SOURCE_MASK) | (cookie & STREAM_ID_COOKIE_MASK));
    }

    constexpr static inline Stream make_for_source(stream_id_t source) throw()
    {
        return make_complete(source, STREAM_ID_COOKIE_MIN);
    }

    constexpr static inline Stream make_invalid() throw()
    {
        return Stream(STREAM_ID_SOURCE_INVALID | STREAM_ID_COOKIE_INVALID);
    }

    constexpr static inline Stream make_from_raw_id(stream_id_t id) throw()
    {
        return Stream(id);
    }

    constexpr stream_id_t get_source() const throw()
    {
        return id_ & STREAM_ID_SOURCE_MASK;
    }

    constexpr stream_id_t get_cookie() const throw()
    {
        return id_ & STREAM_ID_COOKIE_MASK;
    }

    constexpr stream_id_t get_raw_id() const throw()
    {
        return id_;
    }

    constexpr bool is_valid() const throw()
    {
        return ((id_ & STREAM_ID_SOURCE_MASK) != STREAM_ID_SOURCE_INVALID &&
                (id_ & STREAM_ID_COOKIE_MASK) >= STREAM_ID_COOKIE_MIN);
    }

    constexpr bool operator<(const Stream &other) const throw()
    {
        return id_ < other.id_;
    }

    constexpr bool operator>(const Stream &other) const throw()
    {
        return id_ > other.id_;
    }

    constexpr bool operator!=(const Stream &other) const throw()
    {
        return id_ != other.id_;
    }

    constexpr bool operator==(const Stream &other) const throw()
    {
        return id_ == other.id_;
    }

    friend std::ostream &operator<<(std::ostream &os, const Stream &id)
    {
        os << '(' << id.get_source() << ':' << id.get_cookie() << ')';
        return os;
    }

    Stream &operator++() throw()
    {
        static_assert(STREAM_ID_COOKIE_MIN > 0,
                      "Mininum cookie ID must be positive");

        stream_id_t cookie = id_ & STREAM_ID_COOKIE_MASK;

        if(++cookie > STREAM_ID_COOKIE_MAX)
            cookie = STREAM_ID_COOKIE_MIN;

        id_ = (id_ & STREAM_ID_SOURCE_MASK) | cookie;

        return *this;
    }
};

/*!
 * Convenience class for handling stream IDs with a fixed source ID.
 *
 * Applications should create a type alias for a specialization of this class
 * template. For creating new IDs, that type alias should be used. For working
 * with IDs from potentially different sources, the generic #ID::Stream should
 * be used. Use #ID::SourcedStream::make_from_generic_id() to convert from a
 * generic ID to an application-specific one, and check validity with
 * #ID::Stream::is_valid() on the object returned by #ID::SourcedStream::get().
 */
template <stream_id_t Source>
class SourcedStream
{
  private:
    Stream id_;

    constexpr explicit SourcedStream(const Stream &id) throw():
        id_(id)
    {}

  public:
    constexpr static inline SourcedStream make() throw()
    {
        return SourcedStream(Stream::make_for_source(Source));
    }

    constexpr static inline SourcedStream make(stream_id_t cookie) throw()
    {
        return SourcedStream(Stream::make_complete(Source, cookie));
    }

    constexpr static inline SourcedStream make_invalid() throw()
    {
        return SourcedStream(Stream::make_complete(Source,
                                                   STREAM_ID_COOKIE_INVALID));
    }

    constexpr static inline SourcedStream make_from_generic_id(Stream id) throw()
    {
        return (id.get_source() == Source)
            ? SourcedStream(Stream::make_from_raw_id(id.get_raw_id()))
            : SourcedStream::make_invalid();
    }

    constexpr const Stream &get() const throw() { return id_; }

    constexpr bool operator<(const SourcedStream &other) const throw()
    {
        return id_ < other.id_;
    }

    constexpr bool operator>(const SourcedStream &other) const throw()
    {
        return id_ > other.id_;
    }

    constexpr bool operator!=(const SourcedStream &other) const throw()
    {
        return id_ != other.id_;
    }

    constexpr bool operator==(const SourcedStream &other) const throw()
    {
        return id_ == other.id_;
    }

    SourcedStream &operator++() throw()
    {
        ++id_;
        return *this;
    }
};

}

#endif /* !STREAM_ID_HH */
