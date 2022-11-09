/*
 * Copyright (C) 2016, 2018, 2019, 2022  T+A elektroakustik GmbH & Co. KG
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

static void (*extra_handlers[10])(unsigned int);
static const size_t MAX_EXTRA_HANDLERS = sizeof(extra_handlers) / sizeof(extra_handlers[0]);

static void handle_extra(int signum, siginfo_t *info, void *ucontext)
{
    const unsigned int relative_signum = signum - SIG_LOG_LEVEL_MAX - 1;

    extra_handlers[relative_signum](relative_signum);
}

void msg_install_extra_handler(unsigned int relative_signum,
                               void (*handler)(unsigned int))
{
    if(relative_signum >= MAX_EXTRA_HANDLERS)
    {
        msg_error(0, LOG_ERR, "Relative signal number must be less than %zu",
                  MAX_EXTRA_HANDLERS);
        return;
    }

    const int signum = SIG_LOG_LEVEL_MAX + 1 + relative_signum;

    if(signum > SIGRTMAX)
    {
        MSG_BUG("Relative signal number %u > %d", relative_signum, SIGRTMAX);
        return;
    }

    static struct sigaction action =
    {
        .sa_sigaction = handle_extra,
        .sa_flags = SA_SIGINFO | SA_RESTART,
    };

    msg_log_assert(handler != NULL);
    extra_handlers[relative_signum] = handler;

    sigemptyset(&action.sa_mask);
    sigaction(signum, &action, NULL);
}
