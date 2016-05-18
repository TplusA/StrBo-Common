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

#ifndef ACTOR_ID_H
#define ACTOR_ID_H

enum ActorID
{
    ACTOR_ID_INVALID = 0,
    ACTOR_ID_UNKNOWN,
    ACTOR_ID_LOCAL_UI,
    ACTOR_ID_SMARTPHONE_APP,

    ACTOR_ID_LAST_ID = ACTOR_ID_SMARTPHONE_APP,
};

#endif /* !ACTOR_ID_H */
