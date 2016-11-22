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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <signal.h>

#include "messages.h"
#include "messages_signal.h"

#define SIG_LOG_LEVEL_DEFAULT   SIGRTMIN
#define SIG_LOG_LEVEL_MIN       (SIG_LOG_LEVEL_DEFAULT + 1)
#define SIG_LOG_LEVEL_MAX       (SIG_LOG_LEVEL_MIN + (MESSAGE_LEVEL_MAX - MESSAGE_LEVEL_MIN))

static void set_debug_level(int signum, siginfo_t *info, void *ucontext)
{
    static enum MessageVerboseLevel default_level = MESSAGE_LEVEL_IMPOSSIBLE;

    if(default_level == MESSAGE_LEVEL_IMPOSSIBLE)
        default_level = msg_get_verbose_level();

    if(signum == SIG_LOG_LEVEL_DEFAULT)
        msg_set_verbose_level(default_level);
    else if(signum >= SIG_LOG_LEVEL_MIN && signum <= SIG_LOG_LEVEL_MAX)
        msg_set_verbose_level(signum - SIG_LOG_LEVEL_MIN + MESSAGE_LEVEL_MIN);
}

void msg_install_debug_level_signals(void)
{
    static struct sigaction action =
    {
        .sa_sigaction = set_debug_level,
        .sa_flags = SA_SIGINFO | SA_RESTART,
    };

    sigemptyset(&action.sa_mask);

    sigaction(SIG_LOG_LEVEL_DEFAULT, &action, NULL);

    for(int i = SIG_LOG_LEVEL_MIN; i <= SIG_LOG_LEVEL_MAX; ++i)
        sigaction(i, &action, NULL);
}
