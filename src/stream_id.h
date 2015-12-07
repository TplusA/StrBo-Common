/*
 * Copyright (C) 2016, 2019  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of the T+A Streaming Board software stack ("StrBoWare").
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef STREAM_ID_H
#define STREAM_ID_H

#include <stdint.h>

/*!
 * Type to use to represent a stream ID.
 *
 * This is the type used in D-Bus communication. It should be used instead of
 * a raw integer type wherever a stream is referenced by ID.
 *
 * In C++, the #ID::Stream and #ID::SourcedStream classes should be used. These
 * are very lightweight wrappers (no memory overhead, only inline utility
 * functions) that simplify handling stream IDs a lot, and add type safety for
 * application-specific IDs vs foreign IDs by the way.
 *
 * In plain C, the #stream_id_t type should be wrapped into two structs to
 * allow the compiler distinguish own IDs from foreign IDs. Use one struct for
 * IDs obtained from an external source, use the other struct for IDs
 * originating from the application, convert where necessary.
 */
typedef uint16_t stream_id_t;

#ifdef __cplusplus
/*!\internal
 * Helper macro for type conversion in C++.
 */
#define STREAM_ID_TYPE_CAST_ANY(T, V)   static_cast<T>(V)
#else /* !__cplusplus */
/*!\internal
 * Helper macro for type conversion in C.
 */
#define STREAM_ID_TYPE_CAST_ANY(T, V)   ((T)(V))
#endif /* __cplusplus */

/*!\internal
 * Helper macro for type conversion.
 */
#define STREAM_ID_TYPE_CAST(V)          STREAM_ID_TYPE_CAST_ANY(stream_id_t, V)

/*!
 * Number of bits reserved to encode the source of the stream.
 *
 * See also \c STREAM_ID_SOURCE_* macros such as #STREAM_ID_SOURCE_UI.
 */
#define STREAM_ID_SOURCE_BITS   9U

/*! Where to put the stream source bits. */
#define STREAM_ID_SOURCE_SHIFT \
    STREAM_ID_TYPE_CAST_ANY(unsigned int, \
                            sizeof(stream_id_t) * 8U - STREAM_ID_SOURCE_BITS)

/*! Mask for extracting the stream source from a #stream_id_t. */
#define STREAM_ID_SOURCE_MASK \
    STREAM_ID_TYPE_CAST(((1U << STREAM_ID_SOURCE_BITS) - 1U) << \
                        STREAM_ID_SOURCE_SHIFT)

#define STREAM_ID_SOURCE_MIN    STREAM_ID_MAKE_SOURCE(1U)
#define STREAM_ID_SOURCE_MAX    STREAM_ID_SOURCE_MASK

/*! Mask for extracting the stream index (cookie) from a #stream_id_t. */
#define STREAM_ID_COOKIE_MASK   STREAM_ID_TYPE_CAST(~STREAM_ID_SOURCE_MASK)

#define STREAM_ID_COOKIE_INVALID  STREAM_ID_TYPE_CAST(0U)
#define STREAM_ID_COOKIE_MIN    STREAM_ID_TYPE_CAST(1U)
#define STREAM_ID_COOKIE_MAX    STREAM_ID_COOKIE_MASK

/*!
 * Helper macro for defining stream sources.
 */
#define STREAM_ID_MAKE_SOURCE(ID) \
    STREAM_ID_TYPE_CAST((ID) << STREAM_ID_SOURCE_SHIFT)

/*! Stream source: The invalid source. */
#define STREAM_ID_SOURCE_INVALID        STREAM_ID_MAKE_SOURCE(0)

/*! Stream source: Main user interface, i.e., remote control. */
#define STREAM_ID_SOURCE_UI             STREAM_ID_MAKE_SOURCE(1)

/*! Stream source: Smartphone app, i.e., passed via DCP registers. */
#define STREAM_ID_SOURCE_APP            STREAM_ID_MAKE_SOURCE(2)

/*! Stream source: Roon Ready. */
#define STREAM_ID_SOURCE_ROON_READY     STREAM_ID_MAKE_SOURCE(3)

#endif /* !STREAM_ID_H */
