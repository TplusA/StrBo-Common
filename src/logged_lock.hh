/*
 * Copyright (C) 2016, 2017, 2019--2021  T+A elektroakustik GmbH & Co. KG
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

#ifndef LOGGED_LOCK_HH
#define LOGGED_LOCK_HH

#include <mutex>
#include <condition_variable>

#include "messages.h"

#define LOGGED_LOCKS_ENABLED            0
#define LOGGED_LOCKS_ABORT_ON_BUG       1
#define LOGGED_LOCKS_THREAD_CONTEXTS    1

#if LOGGED_LOCKS_ENABLED
/*
 * Using pthreads directly is not portable. Whatever.
 */
#include <pthread.h>

#if LOGGED_LOCKS_ABORT_ON_BUG
#include <stdlib.h>

#define LOGGED_LOCK_BUG(...) \
    do \
    { \
        BUG(__VA_ARGS__); \
        abort(); \
    } \
    while(0)
#else /* !LOGGED_LOCKS_ABORT_ON_BUG */
#define LOGGED_LOCK_BUG(...) BUG(__VA_ARGS__)
#endif /* LOGGED_LOCKS_ABORT_ON_BUG */

#endif /* LOGGED_LOCKS_ENABLED */

namespace LoggedLock
{

#if LOGGED_LOCKS_ENABLED

#if LOGGED_LOCKS_THREAD_CONTEXTS

/*!
 * An application context for a lock.
 *
 * This is a position in code which makes sense to associate a locking action
 * with. The last set context is reported in the log when a locking action
 * takes place, making log messages more useful in case a lock is taken inside
 * some helper function which returns the lock to the caller.
 *
 * A context is set by using the macro #LOGGED_LOCK_CONTEXT_HINT in that
 * place. It can be cleared by macro #LOGGED_LOCK_CONTEXT_HINT_CLEAR. These
 * two macros will do nothing in case #LOGGED_LOCKS_THREAD_CONTEXTS is not
 * enabled.
 */
class Context
{
  private:
    char thread_name_[32];
    char trace_hint_text_[512];
    uint32_t trace_hint_uint_;
    mutable unsigned int trace_hint_age_;

  public:
    Context(const Context &) = delete;
    Context(Context &&) = default;
    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;

    explicit Context():
        thread_name_{0},
        trace_hint_uint_(0),
        trace_hint_age_(0)
    {
        set_thread_name(nullptr);
    }

    void set_thread_name(const char *name)
    {
        if(name != nullptr && name[0] != '\0')
        {
            snprintf(thread_name_, sizeof(thread_name_), "%s", name);
            msg_info("Thread ID <%08lx> is now known as \"%s\"",
                     pthread_self(), thread_name_);
        }
        else
            snprintf(thread_name_, sizeof(thread_name_), "%08lx", pthread_self());
    }

    void set_hint(const char *hint)
    {
        if(hint != nullptr)
            snprintf(trace_hint_text_, sizeof(trace_hint_text_), "%s", hint);
        else
            trace_hint_text_[0] = '\0';

        trace_hint_age_ = 0;
    }

    void set_hint(const uint32_t hint)
    {
        trace_hint_uint_ = hint;
        trace_hint_age_ = 0;
    }

    const char *get_thread_name() const { return thread_name_; }

    const std::string &get_hints_as_text() const
    {
        static thread_local std::string hint;
        hint = thread_name_;
        hint += '~';
        hint += trace_hint_text_;
        hint += "[+";
        hint += std::to_string(trace_hint_age_) + ']';

        if(trace_hint_uint_ > 0)
            hint += ':' + std::to_string(trace_hint_uint_);

        ++trace_hint_age_;

        return hint;
    }

    void clear_hints()
    {
        trace_hint_text_[0] = '\0';
        trace_hint_uint_ = 0;
        trace_hint_age_ = 0;
    }
};

/*!
 * Per-thread instance of #LoggedLock::Context.
 *
 * This instance of #LoggedLock::Context must be defined somewhere in case
 * #LOGGED_LOCKS_THREAD_CONTEXTS is defined, otherwise you'll run into linker
 * errors.
 */
extern thread_local Context context;

static inline const std::string &get_context_hints() { return context.get_hints_as_text(); }

static inline void set_context_name(const char *name)
{
    context.set_thread_name(name);
}

#define LOGGED_LOCK_CONTEXT_HINT \
    do { \
        LoggedLock::context.set_hint(__func__); \
        LoggedLock::context.set_hint(__LINE__); \
    } while(0)

#define LOGGED_LOCK_CONTEXT_HINT_CLEAR \
    do { \
        LoggedLock::context.clear_hints(); \
    } while(0)

#else /* !LOGGED_LOCKS_THREAD_CONTEXTS */

static inline const std::string &get_context_hints()
{
    static thread_local std::string hint;

    if(hint.empty())
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%08lx", pthread_self());
        hint = buffer;
    }

    return hint;
}

static inline void set_context_name(const char *name) {}

#define LOGGED_LOCK_CONTEXT_HINT            do {} while(0)
#define LOGGED_LOCK_CONTEXT_HINT_CLEAR      do {} while(0)

#endif /* LOGGED_LOCKS_THREAD_CONTEXTS */

/*!
 * Wrapper around \c std::mutex.
 */
class Mutex
{
  private:
    std::mutex lock_;
    const char *name_;
    std::string name_buffer_;
    pthread_t owner_;
    MessageVerboseLevel log_level_;

  public:
    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    explicit Mutex():
        name_("(unnamed)"),
        owner_(0),
        log_level_(MESSAGE_LEVEL_NORMAL)
    {}

    void about_to_lock(bool is_direct) const
    {
        if(owner_ == pthread_self())
            LOGGED_LOCK_BUG("Mutex %s: DEADLOCK for <%s> (%sdirect)",
                            name_, get_context_hints().c_str(), is_direct ? "" : "in");
    }

    void lock()
    {
        msg_vinfo(log_level_, "<%s> Mutex %s: lock", get_context_hints().c_str(), name_);

        about_to_lock(true);
        lock_.lock();
        set_owner();
        msg_vinfo(log_level_, "<%s> Mutex %s: locked", get_context_hints().c_str(), name_);
    }

    bool try_lock()
    {
        msg_vinfo(log_level_, "<%s> Mutex %s: try lock", get_context_hints().c_str(), name_);

        const bool is_locked = lock_.try_lock();

        if(is_locked)
        {
            set_owner();
            msg_vinfo(log_level_, "<%s> Mutex %s: locked on try",
                      get_context_hints().c_str(), name_);
        }
        else
            msg_vinfo(log_level_, "<%s> Mutex %s: try locking failed (%s)",
                      get_context_hints().c_str(), name_,
                      owner_ == pthread_self() ? "avoided deadlock" : "different owner");

        return is_locked;
    }

    void unlock()
    {
        clear_owner();
        lock_.unlock();
    }

    std::mutex &get_raw_mutex(bool log_this = true)
    {
        if(log_this)
            msg_vinfo(log_level_, "<%s> Mutex %s: get raw mutex",
                      get_context_hints().c_str(), name_);

        return lock_;
    }

    void set_owner()
    {
        if(owner_ != 0)
            LOGGED_LOCK_BUG("Mutex %s: replace owner <%08lx> by <%08lx> <%s>",
                            name_, owner_, pthread_self(), get_context_hints().c_str());

        owner_ = pthread_self();
    }

    void clear_owner()
    {
        msg_vinfo(log_level_, "<%s> Mutex %s: unlock", get_context_hints().c_str(), name_);

        if(owner_ == 0)
            LOGGED_LOCK_BUG("Mutex %s: <%s> clearing unowned",
                            name_, get_context_hints().c_str());
        else if(owner_ != pthread_self())
            LOGGED_LOCK_BUG("Mutex %s: <%s> stealing from owner <%08lx>",
                            name_, get_context_hints().c_str(), owner_);

        owner_ = 0;
    }

    pthread_t get_owner() const { return owner_; }

    void configure(const char *name, MessageVerboseLevel log_level)
    {
        name_ = name;
        log_level_ = log_level;
    }

    void configure(std::string &&name, MessageVerboseLevel log_level)
    {
        name_buffer_ = std::move(name);
        name_ = name_buffer_.c_str();
        log_level_ = log_level;
    }

    const char *get_name() const { return name_; }

    MessageVerboseLevel get_log_level() const { return log_level_; }
};

/*!
 * Wrapper around \c std::recursive_mutex.
 */
class RecMutex
{
  private:
    std::recursive_mutex lock_;
    size_t lock_count_;
    const char *name_;
    std::string name_buffer_;
    pthread_t owner_;
    MessageVerboseLevel log_level_;

  public:
    RecMutex(const RecMutex &) = delete;
    RecMutex &operator=(const RecMutex &) = delete;

    explicit RecMutex():
        lock_count_(0),
        name_("(unnamed)"),
        owner_(0),
        log_level_(MESSAGE_LEVEL_NORMAL)
    {}

    void about_to_lock(bool is_direct) const
    {
        if(owner_ == pthread_self())
            msg_vinfo(log_level_,
                      "<%s> RecMutex %s: re-lock, lock count %zu (%sdirect)",
                      get_context_hints().c_str(), name_, lock_count_,
                      is_direct ? "" : "in");
    }

    void lock()
    {
        msg_vinfo(log_level_, "<%s> RecMutex %s: lock", get_context_hints().c_str(), name_);

        about_to_lock(true);
        lock_.lock();
        ref_owner();
        msg_vinfo(log_level_, "<%s> RecMutex %s: locked", get_context_hints().c_str(), name_);
    }

    bool try_lock()
    {
        msg_vinfo(log_level_, "<%s> RecMutex %s: try lock", get_context_hints().c_str(), name_);

        const bool is_locked = lock_.try_lock();

        if(is_locked)
        {
            if(lock_count_ > 0 && owner_ != pthread_self())
                LOGGED_LOCK_BUG("RecMutex %s: lock attempt by <%s> "
                                "succeeded, but shouldn't have",
                                name_, get_context_hints().c_str());

            ref_owner();
            msg_vinfo(log_level_, "<%s> RecMutex %s: locked",
                      get_context_hints().c_str(), name_);
        }
        else
        {
            msg_vinfo(log_level_, "<%s> RecMutex %s: locking failed",
                      get_context_hints().c_str(), name_);

            if(lock_count_ == 0 || owner_ == pthread_self())
                LOGGED_LOCK_BUG("RecMutex %s: lock attempt by <%s> "
                                "should have succeeded "
                                "(owner <%08lx>, lock count %zu)",
                                name_, get_context_hints().c_str(), owner_, lock_count_);
        }

        return is_locked;
    }

    void unlock()
    {
        unref_owner();
        lock_.unlock();
    }

    std::recursive_mutex &get_raw_mutex(bool log_this = true)
    {
        if(log_this)
            msg_vinfo(log_level_, "<%s> RecMutex %s: get raw mutex",
                      get_context_hints().c_str(), name_);

        return lock_;
    }

    void ref_owner()
    {
        ++lock_count_;

        if(owner_ == pthread_self())
        {
            if(lock_count_ <= 1)
                LOGGED_LOCK_BUG("RecMutex %s: <%s> sets owner for "
                                "lock count %zu",
                                name_, get_context_hints().c_str(), lock_count_);

            return;
        }

        if(owner_ != 0)
            LOGGED_LOCK_BUG("RecMutex %s: replace owner <%08lx> by <%08lx> <%s>, "
                            "lock count %zu",
                            name_, owner_, pthread_self(),
                            get_context_hints().c_str(), lock_count_);

        owner_ = pthread_self();
    }

    void unref_owner()
    {
        msg_vinfo(log_level_, "<%s> RecMutex %s: unlock, lock count %zu -> %zu",
                  get_context_hints().c_str(), name_,
                  lock_count_, lock_count_ - 1);

        if(owner_ == 0)
            LOGGED_LOCK_BUG("RecMutex %s: <%s> clearing unowned, "
                            "lock count %zu",
                            name_, get_context_hints().c_str(), lock_count_);
        else if(owner_ != pthread_self())
            LOGGED_LOCK_BUG("RecMutex %s: <%s> stealing from owner "
                            "<%08lx>, lock count %zu",
                            name_, get_context_hints().c_str(), owner_, lock_count_);
        else if(lock_count_ == 0)
            LOGGED_LOCK_BUG("RecMutex %s: <%s> unref with lock count 0, "
                            "owner <%08lx>",
                            name_, get_context_hints().c_str(), owner_);

        --lock_count_;

        if(lock_count_ == 0)
        {
            msg_vinfo(log_level_, "<%s> RecMutex %s: unlocked, drop owner <%08lx>",
                      get_context_hints().c_str(), name_, owner_);
            owner_ = 0;
        }
    }

    pthread_t get_owner() const { return owner_; }

    void configure(const char *name, MessageVerboseLevel log_level)
    {
        name_buffer_.clear();
        name_ = name;
        log_level_ = log_level;
    }

    void configure(std::string &&name, MessageVerboseLevel log_level)
    {
        name_buffer_ = std::move(name);
        name_ = name_buffer_.c_str();
        log_level_ = log_level;
    }

    const char *get_name() const { return name_; }

    MessageVerboseLevel get_log_level() const { return log_level_; }
};

template <typename MutexType> struct MutexTraits;

template <>
struct MutexTraits<::LoggedLock::Mutex>
{
    using StdMutexType = std::mutex;

    static void set_owner(Mutex &m) { m.set_owner(); }
    static void clear_owner(Mutex &m) { m.clear_owner(); }
    static pthread_t get_owner(const Mutex &m) { return m.get_owner(); }
    static void destroy_owned(Mutex &m) { m.clear_owner(); }
};

template <>
struct MutexTraits<::LoggedLock::RecMutex>
{
    using StdMutexType = std::recursive_mutex;

    static void set_owner(RecMutex &m) { m.ref_owner(); }
    static void clear_owner(RecMutex &m) { m.unref_owner(); }
    static pthread_t get_owner(const RecMutex &m) { return m.get_owner(); }
    static void destroy_owned(RecMutex &m) { m.unref_owner(); }
};

/*!
 * Wrapper around \c std::unique_lock.
 *
 * Note that there is no wrapper for \c std::lock_guard. Feel free to use
 * \c std::lock_guard with one of the wrapped mutexes.
 */
template <typename MutexType>
class UniqueLock
{
  private:
    using MTraits = MutexTraits<MutexType>;

    MutexType &logged_mutex_;
    std::unique_lock<typename MTraits::StdMutexType> lock_;
    const char *lock_name_;
    std::string lock_name_buffer_;
    bool moved_from_;

  public:
    UniqueLock(const UniqueLock &) = delete;
    UniqueLock &operator=(const UniqueLock &) = delete;

    explicit UniqueLock(MutexType &mutex):
        logged_mutex_(mutex),
        lock_(logged_mutex_.get_raw_mutex(false), std::defer_lock),
        lock_name_(logged_mutex_.get_name()),
        moved_from_(false)
    {
        msg_vinfo(logged_mutex_.get_log_level(),
                  "<%s> UniqueLock %p: create, attempt to lock %s",
                  get_context_hints().c_str(),
                  static_cast<const void *>(this), lock_name_);
        lock();
    }

    explicit UniqueLock(MutexType &mutex, std::defer_lock_t t) noexcept:
        logged_mutex_(mutex),
        lock_(logged_mutex_.get_raw_mutex(false), t),
        lock_name_(logged_mutex_.get_name()),
        moved_from_(false)
    {
        msg_vinfo(logged_mutex_.get_log_level(),
                  "<%s> UniqueLock %p: created with unlocked %s",
                  get_context_hints().c_str(),
                  static_cast<const void *>(this), lock_name_);
    }

    explicit UniqueLock(MutexType &mutex, std::try_to_lock_t t):
        logged_mutex_(mutex),
        lock_(logged_mutex_.get_raw_mutex(false), t),
        lock_name_(logged_mutex_.get_name()),
        moved_from_(false)
    {
        msg_vinfo(logged_mutex_.get_log_level(),
                  "<%s> UniqueLock %p: created, tried to lock and %s, %s",
                  get_context_hints().c_str(),
                  static_cast<const void *>(this),
                  lock_.owns_lock() ? "succeeded" : "failed", lock_name_);

        if(lock_.owns_lock())
            MTraits::set_owner(logged_mutex_);
    }


    UniqueLock(UniqueLock &&src) noexcept:
        logged_mutex_(src.logged_mutex_),
        lock_(std::move(src.lock_)),
        lock_name_(src.lock_name_),
        moved_from_(src.moved_from_)
    {
        src.moved_from_ = true;
        msg_vinfo(logged_mutex_.get_log_level(), "<%s> UniqueLock %p: moved to %p",
                  get_context_hints().c_str(), static_cast<const void *>(&src),
                  static_cast<const void *>(this));
    }

    ~UniqueLock()
    {
        if(moved_from_)
            return;

        msg_vinfo(logged_mutex_.get_log_level(),
                  "<%s> UniqueLock %p: destroy with %s owned by <%08lx>",
                  get_context_hints().c_str(), static_cast<const void *>(this),
                  lock_name_, get_mutex_owner());

        if(lock_.owns_lock())
            MTraits::destroy_owned(logged_mutex_);
    }

    void configure() { lock_name_ = logged_mutex_.get_name(); }

    void configure(const char *name)
    {
        lock_name_buffer_.clear();
        lock_name_ = name;
    }

    void configure(std::string &&name)
    {
        lock_name_buffer_ = std::move(name);
        lock_name_ = lock_name_buffer_.c_str();
    }

    void lock()
    {
        msg_vinfo(logged_mutex_.get_log_level(),
                  "<%s> UniqueLock %p: lock %s",
                  get_context_hints().c_str(),
                  static_cast<const void *>(this), lock_name_);
        logged_mutex_.about_to_lock(false);
        lock_.lock();
        MTraits::set_owner(logged_mutex_);
        msg_vinfo(logged_mutex_.get_log_level(),
                  "<%s> UniqueLock %p: locked %s",
                  get_context_hints().c_str(),
                  static_cast<const void *>(this), lock_name_);
    }

    void unlock()
    {
        msg_vinfo(logged_mutex_.get_log_level(),
                  "<%s> UniqueLock %p: unlock %s",
                  get_context_hints().c_str(),
                  static_cast<const void *>(this), lock_name_);
        MTraits::clear_owner(logged_mutex_);
        lock_.unlock();
    }

    std::unique_lock<std::mutex> &get_raw_unique_lock()
    {
        msg_vinfo(logged_mutex_.get_log_level(),
                  "<%s> UniqueLock %p: get raw unique_lock for %s",
                  get_context_hints().c_str(),
                  static_cast<const void *>(this), lock_name_);
        return lock_;
    }

    bool owns_lock() const noexcept { return lock_.owns_lock(); }

    const char *get_mutex_name() const { return lock_name_; }

    void set_mutex_owner() { MTraits::set_owner(logged_mutex_); }
    void clear_mutex_owner() { MTraits::clear_owner(logged_mutex_); }
    pthread_t get_mutex_owner() const { return MTraits::get_owner(logged_mutex_); }
};

/*
 * Wrapper around \c std::condition_variable.
 */
class ConditionVariable
{
  private:
    std::condition_variable var_;
    const char *name_;
    std::string name_buffer_;
    MessageVerboseLevel log_level_;

  public:
    ConditionVariable(const ConditionVariable &) = delete;
    ConditionVariable &operator=(const ConditionVariable &) = delete;

    explicit ConditionVariable():
        name_("(unnamed)"),
        log_level_(MESSAGE_LEVEL_NORMAL)
    {}

    template <class Predicate>
    void wait(UniqueLock<Mutex> &lock, Predicate pred)
    {
        if(lock.get_mutex_owner() != pthread_self())
            LOGGED_LOCK_BUG("Cond %s: <%s> waiting with foreign mutex "
                            "owned by <%08lx>",
                            name_, get_context_hints().c_str(), lock.get_mutex_owner());

        msg_vinfo(log_level_, "<%s> Cond %s: wait for %s",
                  get_context_hints().c_str(), name_, lock.get_mutex_name());
        lock.clear_mutex_owner();
        var_.wait(lock.get_raw_unique_lock(), pred);
        lock.set_mutex_owner();
        msg_vinfo(log_level_, "<%s> Cond %s: waited for %s",
                  get_context_hints().c_str(), name_, lock.get_mutex_name());
    }

    template <class Rep, class Period, class Predicate>
    bool wait_for(UniqueLock<Mutex> &lock,
                  const std::chrono::duration<Rep, Period>& rel_time,
                  Predicate pred)
    {
        if(lock.get_mutex_owner() != pthread_self())
            LOGGED_LOCK_BUG("Cond %s: <%s> waiting with foreign mutex "
                            "owned by <%08lx> with timeout",
                            name_, get_context_hints().c_str(), lock.get_mutex_owner());

        msg_vinfo(log_level_, "<%s> Cond %s: wait for %s with timeout",
                  get_context_hints().c_str(), name_, lock.get_mutex_name());
        lock.clear_mutex_owner();
        const bool result = var_.wait_for(lock.get_raw_unique_lock(), rel_time, pred);
        lock.set_mutex_owner();
        msg_vinfo(log_level_, "<%s> Cond %s: waited for %s with timeout -> %s",
                  get_context_hints().c_str(), name_, lock.get_mutex_name(),
                  result ? "OK" : "timed out");
        return result;
    }

    void notify_all()
    {
        msg_vinfo(log_level_, "<%s> Cond %s: notify all",
                  get_context_hints().c_str(), name_);
        var_.notify_all();
    }

    void notify_one()
    {
        msg_vinfo(log_level_, "<%s> Cond %s: notify one",
                  get_context_hints().c_str(), name_);
        var_.notify_one();
    }

    void configure(const char *name, MessageVerboseLevel log_level)
    {
        name_ = name;
        log_level_ = log_level;
    }

    void configure(std::string &&name, MessageVerboseLevel log_level)
    {
        name_buffer_ = std::move(name);
        name_ = name_buffer_.c_str();
        log_level_ = log_level;
    }

    const char *get_name() const { return name_; }
};

/*!
 * Configuration of wrapper objects for enhanced logging (static string).
 *
 * Not required to be used, but doing so may help with more readable logs.
 *
 * \param object
 *     The object to be configured.
 *
 * \param name
 *     Name of the object. This should be something which can identify the
 *     instance of the object or which states the purpose of the object. Note
 *     that this is a pointer to a string managed by the caller. Make sure to
 *     avoid dangling pointers or use the other #LoggedLock::configure()
 *     variant which takes a \c std::string rvalue reference.
 *
 * \param log_level
 *     Minimum log level at which logs are emitted for uses of the object.
 */
template <typename T>
static inline void configure(T &object,
                             const char *name, MessageVerboseLevel log_level)
{
    object.configure(name, log_level);
}

/*!
 * Configuration of wrapper objects for enhanced logging (dynamic string).
 *
 * Not required to be used, but doing so may help with more readable logs.
 *
 * \param object
 *     The object to be configured.
 *
 * \param name
 *     Name of the object. This should be something which can identify the
 *     instance of the object or which states the purpose of the object. This
 *     name is moved into the object so that it gets destroyed when the object
 *     is destroyed.
 *
 * \param log_level
 *     Minimum log level at which logs are emitted for uses of the object.
 */
template <typename T>
static inline void configure(T &object,
                             std::string &&name, MessageVerboseLevel log_level)
{
    object.configure(std::move(name), log_level);
}

/*!
 * Configuration of #LoggedLock::UniqueLock objects for enhanced logging.
 *
 * By default, the name of a #LoggedLock::UniqueLock is set to the name of its
 * mutex when it is created. This function does the same, so it may be used to
 * revert any renames by one of the other #LoggedLock::configure functions.
 */
template <typename T>
static inline void configure(LoggedLock::UniqueLock<T> &lk)
{
    lk.configure();
}

/*!
 * Set name of a #LoggedLock::UniqueLock object (static string).
 *
 * This function does not modify the name of the referenced mutex.
 */
template <typename T>
static inline void configure(LoggedLock::UniqueLock<T> &lk, const char *name)
{
    lk.configure(name);
}

/*!
 * Set name of a #LoggedLock::UniqueLock object (dynamic string).
 *
 * This function does not modify the name of the referenced mutex.
 */
template <typename T>
static inline void configure(LoggedLock::UniqueLock<T> &lk, std::string &&name)
{
    lk.configure(std::move(name));
}

#else /* /!LOGGED_LOCKS_ENABLED */

static inline void set_context_name(const char *name) {}
#define LOGGED_LOCK_CONTEXT_HINT            do {} while(0)
#define LOGGED_LOCK_CONTEXT_HINT_CLEAR      do {} while(0)

using Mutex = std::mutex;
using RecMutex = std::recursive_mutex;
template <typename MutexType> using UniqueLock = std::unique_lock<MutexType>;
using ConditionVariable = std::condition_variable;

template <typename T>
static inline void configure(T &object,
                             const char *name, MessageVerboseLevel log_level)
{
    /* nothing */
}

template <typename T>
static inline void configure(T &object,
                             std::string &&name, MessageVerboseLevel log_level)
{
    /* nothing */
}

template <typename T>
static inline void configure(LoggedLock::UniqueLock<T> &lk)
{
    /* nothing */
}

template <typename T>
static inline void configure(LoggedLock::UniqueLock<T> &lk, const char *name)
{
    /* nothing */
}

template <typename T>
static inline void configure(LoggedLock::UniqueLock<T> &lk, std::string &&name)
{
    /* nothing */
}


#endif /* LOGGED_LOCKS_ENABLED */

}

#endif /* !LOGGED_LOCK_HH */
