/*
 * Copyright (C) 2022, 2023, 2024  T+A elektroakustik GmbH & Co. KG
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

#ifndef TADDYBUS_HH
#define TADDYBUS_HH

#include <string>
#include <list>
#include <functional>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <atomic>

#include <gio/gio.h>

#include "gerrorwrapper.hh"

/*!
 * T+A D-Bus (wrapping GDBus into a sensible C++ interface).
 */
namespace TDBus
{

void log_connect_proxy_bug(const char *object_path, const char *name);
void log_method_done_exception_bug(const char *what);
void log_async_user_data_ptr(bool should_be_null, const char *fn);
void log_bad_async_user_data_ptr();
void log_async_result_leak();
void log_proxy_not_connected(const char *method_name);

/*!
 * Base class for server-side D-Bus interfaces implementations.
 */
class IfaceBase
{
  protected:
    const std::string object_path_;

    explicit IfaceBase(std::string &&object_path):
        object_path_(std::move(object_path))
    {}

  public:
    IfaceBase(const IfaceBase &) = delete;
    IfaceBase(IfaceBase &&) = default;
    IfaceBase &operator=(const IfaceBase &) = delete;
    IfaceBase &operator=(IfaceBase &&) = delete;

    virtual ~IfaceBase() = default;

    /*!
     * Export an object implementing this interface on the given connection.
     */
    bool export_interface(GDBusConnection *connection) const
    {
        GErrorWrapper error;
        if(g_dbus_interface_skeleton_export(get_interface_skeleton(),
                                            connection, object_path_.c_str(),
                                            error.await()))
            return true;

        /* TODO: handle error */
        return false;
    }

    const std::string &get_object_path() const { return object_path_; }

    static void method_fail(GDBusMethodInvocation *invocation,
                            const char *message, ...);

  protected:
    virtual GDBusInterfaceSkeleton *get_interface_skeleton() const = 0;
};

/*!
 * Traits class for server-side D-Bus interface implementation.
 *
 * For use with #TDBus::Iface.
 */
template <typename T> struct IfaceTraits;

/*!
 * Traits class for server-side D-Bus method implementation.
 *
 * For use with #TDBus::Iface.
 */
template <typename T> struct MethodHandlerTraits;


/*!
 * Traits class for client-side D-Bus method calls.
 *
 * For use with #TDBus::Proxy.
 */
template <typename T> struct MethodCallerTraits;

/*!
 * Traits class for client-side D-Bus signal implementation.
 *
 * For use with #TDBus::Proxy.
 */
template <typename T> struct SignalHandlerTraits;

/* Some special trace macros so that the logged context makes more sense. */
#if MSG_TRACE_ENABLED
#define MSG_TRACE_DBUS_SIGNAL() \
    MSG_TRACE_FUNCTION(MSG_TRACE_PREFIX "%s(%d) [%s]" MSG_TRACE_SUFFIX, \
                       signal_type_name(), __LINE__, dbus_signal_name())

#define MSG_TRACE_DBUS_SIGNAL_FORMAT(FMT, ...) \
    MSG_TRACE_FUNCTION(MSG_TRACE_PREFIX "%s(%d) [%s]: " FMT MSG_TRACE_SUFFIX, \
                       signal_type_name(), __LINE__, dbus_signal_name(), \
                       __VA_ARGS__)

#define MSG_TRACE_DBUS_METHOD() \
    MSG_TRACE_FUNCTION(MSG_TRACE_PREFIX "%s(%d) [%s]" MSG_TRACE_SUFFIX, \
                       method_type_name(), __LINE__, dbus_method_name())

#define MSG_TRACE_DBUS_METHOD_FORMAT(FMT, ...) \
    MSG_TRACE_FUNCTION(MSG_TRACE_PREFIX "%s(%d) [%s]: " FMT MSG_TRACE_SUFFIX, \
                       method_type_name(), __LINE__, dbus_method_name(), \
                       __VA_ARGS__)
#else /*! MSG_TRACE_ENABLED */
#define MSG_TRACE_DBUS_SIGNAL()                 MSG_TRACE()
#define MSG_TRACE_DBUS_SIGNAL_FORMAT(FMT, ...)  MSG_TRACE_FORMAT(FMT, __VA_ARGS__)
#define MSG_TRACE_DBUS_METHOD()                 MSG_TRACE()
#define MSG_TRACE_DBUS_METHOD_FORMAT(FMT, ...)  MSG_TRACE_FORMAT(FMT, __VA_ARGS__)
#endif /* MSG_TRACE_ENABLED */

/*!
 * Template for server-side D-Bus interfaces implementations.
 *
 * \tparam T
 *     A type "derived" from \c GDBusInterfaceSkeleton.
 */
template <typename T, typename Traits = IfaceTraits<T>>
class Iface: public IfaceBase
{
  public:
    template <typename Tag, typename... UserDataT>
    class MethodHandlerData
    {
      public:
        Iface &iface;
        std::tuple<UserDataT...> user_data;

        explicit MethodHandlerData(Iface &i, UserDataT &&... d):
            iface(i),
            user_data(std::forward<UserDataT>(d)...)
        {}

        /*!
         * Convenience template for calling #TDBus::Iface::method_done().
         */
        template <typename... Args>
        void done(GDBusMethodInvocation *invocation, Args &&... args)
        {
            iface.method_done<Tag>(invocation, std::forward<Args>(args)...);
        }
    };

  private:
    T *iface_;
    T *iface_fake_;

    template <typename Tag, typename... UserDataT>
    static void delete_handler_data(gpointer data, GClosure *closure)
    {
        delete static_cast<MethodHandlerData<Tag, UserDataT...> *>(data);
    }

  public:
    Iface(Iface &&src) = delete;
    Iface &operator=(Iface &&src) = delete;

    explicit Iface(std::string &&object_path, bool is_fake = false):
        IfaceBase(std::move(object_path)),
        iface_(Traits::skeleton_new()),
        iface_fake_(nullptr)
    {
        if(is_fake)
        {
            iface_fake_ = iface_;
            iface_ = nullptr;
        }
    }

    ~Iface()
    {
        if(iface_ == nullptr)
        {
            iface_fake_ = nullptr;
            return;
        }

        g_object_unref(iface_);
        iface_ = nullptr;
    }

    /*!
     * Connect D-Bus method invocations to its handler.
     *
     * Calling this function means that the application is willing to handle
     * D-Bus calls of the method associated with \p MHTraits. The caller must
     * therefore provide an implementation of the handler declared in
     * \p MHTraits.
     *
     * Note that the handler is shared among \e all connections and \e all
     * objects implementing the D-Bus interface. It is also not possible to
     * pass a user data pointer to the handler using this function. Use
     * #TDBus::Iface::connect_method_handler() if you need more flexibility.
     *
     * \tparam Tag
     *     A type used to identify a D-Bus method, usually a \c struct name
     *     with no definition (incomplete type).
     *
     * \tparam MHTraits
     *     Handler traits declaring the D-Bus method handler.
     */
    template <typename Tag, typename MHTraits = MethodHandlerTraits<Tag>>
    Iface &connect_default_method_handler()
    {
        static_assert(std::is_same<typename MHTraits::IfaceType, T>::value,
                      "Handler is not meant to be used with this interface");
        if(iface_ != nullptr)
            g_signal_connect(iface_, MHTraits::glib_signal_name(),
                             G_CALLBACK(MHTraits::method_handler), this);
        return *this;
    }

    /*!
     * Connect D-Bus method invocations to its handler.
     *
     * Calling this function means that the application is willing to handle
     * D-Bus calls of the method associated with \p MHTraits. The caller must
     * therefore pass a non-null handler of type \p MHTraits::HandlerType.
     *
     * \tparam Tag
     *     A type used to identify a D-Bus method, usually a \c struct name
     *     with no definition (incomplete type).
     *
     * \tparam MHTraits
     *     Handler traits declaring the D-Bus method handler.
     *
     * \param fn
     *     Handler for the D-Bus method call. Its last parameter is a
     *     #TDBus::Iface::MethodHandlerData object which contains a reference
     *     to the #TDBus::Iface instance the method handler refers to. This
     *     allows use of different method handlers for different D-Bus objects.
     *
     * \param user_data
     *     Optional pointer for use inside the handler. This pointer is passed
     *     as part of a #TDBus::Iface::MethodHandlerData object as passed as
     *     the handler's last parameter.
     */
    template <typename Tag, typename... UserDataT,
              typename MHTraits = MethodHandlerTraits<Tag>>
    Iface &connect_method_handler(typename MHTraits::template HandlerType<UserDataT...> fn,
                                  UserDataT &&... user_data)
    {
        static_assert(std::is_same<typename MHTraits::IfaceType, T>::value,
                      "Handler is not meant to be used with this interface");
        if(iface_ != nullptr)
            g_signal_connect_data(
                iface_, MHTraits::glib_signal_name(), G_CALLBACK(fn),
                new MethodHandlerData<Tag, UserDataT...>(
                        *this, std::forward<UserDataT>(user_data)...),
                delete_handler_data<Tag, UserDataT...>,
                GConnectFlags(0));
        return *this;
    }

    /*!
     * Connect D-Bus method invocations to a handler function.
     *
     * Use this version of this function to connect to a method handler
     * function which won't make use of the user data passed from GLib. The
     * pointer will always be \c nullptr if connecting through this function.
     */
    template <typename Tag, typename MHTraits = MethodHandlerTraits<Tag>>
    Iface &connect_method_handler(typename MHTraits::ClassicHandlerType fn)
    {
        static_assert(std::is_same<typename MHTraits::IfaceType, T>::value,
                      "Handler is not meant to be used with this interface");
        if(iface_ != nullptr)
            g_signal_connect(iface_, MHTraits::glib_signal_name(),
                             G_CALLBACK(fn), nullptr);
        return *this;
    }

    /*!
     * Complete handling a D-Bus method call.
     *
     * This function is called from D-Bus method handlers, either directly
     * inside the handler or deferred in different context.
     *
     * \tparam Tag
     *     A type used to identify a D-Bus method, usually a \c struct name
     *     with no definition (incomplete type).
     *
     * \tparam MHTraits
     *     Callers shall pass \c ThisMethod, unqualified so if used directly in
     *     D-Bus handlers, or qualifying it with the correct traits class in
     *     other contexts. In case of a call directly from the D-Bus handler,
     *     the traits class will be in scope and the correct type named
     *     \c ThisMethod is going to be picked up from there.
     *
     * \tparam Args
     *     Any arguments the completion function requires.
     *
     * \param invocation
     *     The D-Bus method invocation as passed in from GLib.
     *
     *  \param args
     *     Forwarded to completion function.
     */
    template <typename Tag, typename MHTraits = MethodHandlerTraits<Tag>,
              typename... Args>
    void method_done(GDBusMethodInvocation *invocation, Args &&... args)
    {
        static_assert(std::is_same<typename MHTraits::IfaceType, T>::value,
                      "Handler is not meant to be used with this interface");
        MHTraits::complete(iface_ != nullptr ? iface_ : iface_fake_, invocation,
                           std::forward<Args>(args)...);
    }

    template <typename Tag, typename MHTraits = MethodHandlerTraits<Tag>,
              typename... Args>
    void method_done_and_flush(GDBusMethodInvocation *invocation, Args &&... args)
    {
        auto *conn = g_dbus_method_invocation_get_connection(invocation);
        method_done<Tag>(invocation, std::forward<Args>(args)...);

        GErrorWrapper error;
        g_dbus_connection_flush_sync(conn, nullptr, error.await());
        error.log_failure("Flush D-Bus connection");
    }

    template <typename FN, typename... Args>
    void emit(FN fn, Args &&... args)
    {
        static_assert(!std::is_function<FN>::value,
                      "First argument must be a function");
        fn(iface_ != nullptr ? iface_ : iface_fake_,
           std::forward<Args>(args)...);
    }

  protected:
    GDBusInterfaceSkeleton *get_interface_skeleton() const final override
    {
        return iface_ != nullptr ? G_DBUS_INTERFACE_SKELETON(iface_) : nullptr;
    }
};

/*!
 * Base class for client-side proxy objects.
 */
class ProxyBase
{
  public:
    using ProxyNewFunction =
        void (*)(GDBusConnection *, GDBusProxyFlags, const char *, const char *,
                 GCancellable *, GAsyncReadyCallback, gpointer);

    template <typename T>
    using ProxyNewFinishFunction = T *(*)(GAsyncResult *res, GError **error);

  private:
    std::string const_name_;
    std::string const_object_path_;

  protected:
    bool is_busy_ = false;

    explicit ProxyBase(std::string &&name, std::string &&object_path):
        const_name_(std::move(name)),
        const_object_path_(std::move(object_path)),
        is_busy_(false)
    {}

  public:
    ProxyBase(const ProxyBase &) = delete;
    ProxyBase(ProxyBase &&) = default;
    ProxyBase &operator=(const ProxyBase &) = delete;
    ProxyBase &operator=(ProxyBase &&) = default;
    virtual ~ProxyBase() = default;

    const std::string &get_name() const { return const_name_; }
    const std::string &get_object_path() const { return const_object_path_; }
};

template <typename T> struct ProxyTraits;

namespace { static std::atomic<uint32_t> next_free_async_call_cookie; }

/*!
 * Client-side proxy for remote object.
 *
 * \tparam T
 *     A type "derived" from \c GDBusProxy.
 */
template <typename T, typename Traits = ProxyTraits<T>>
class Proxy: public ProxyBase
{
  public:
    template <typename Tag, typename... UserDataT>
    class SignalHandlerData
    {
      public:
        Proxy &proxy;
        std::tuple<UserDataT...> user_data;

        explicit SignalHandlerData(Proxy &p, UserDataT &&... d):
            proxy(p),
            user_data(std::forward<UserDataT>(d)...)
        {}
    };

    class AsyncCall
    {
      public:
        /*!
         * Originating proxy, pointer to this, and cookie.
         */
        using UserDataForInvokeType = std::tuple<Proxy *, AsyncCall *, uint32_t>;

      private:
        std::function<void(Proxy &, AsyncCall &)> done_;
        GCancellable *cancellable_;
        GAsyncResult *result_;
        uint32_t cookie_;
        UserDataForInvokeType *user_data_for_invoke_;

      public:
        AsyncCall &operator=(AsyncCall &&src) = delete;
        AsyncCall(const AsyncCall &other) = delete;

        explicit AsyncCall(std::function<void(Proxy &, AsyncCall &)> &&done):
            done_(std::move(done)),
            cancellable_(g_cancellable_new()),
            result_(nullptr),
            cookie_(next_free_async_call_cookie++),
            user_data_for_invoke_(nullptr)
        {}

        AsyncCall(AsyncCall &&src):
            done_(std::move(src.done_)),
            cancellable_(src.cancellable_),
            result_(src.result_),
            cookie_(src.cookie_),
            user_data_for_invoke_(src.user_data_for_invoke_)
        {
            src.cancellable_ = nullptr;
            src.result_ = nullptr;
            src.cookie_ -= 100000;
            src.user_data_for_invoke_ = nullptr;
        }

        ~AsyncCall()
        {
            if(cancellable_ != nullptr)
                g_object_unref(cancellable_);

            if(result_ != nullptr)
                log_async_result_leak();

            if(user_data_for_invoke_ != nullptr)
            {
                log_async_user_data_ptr(true, __func__);
                delete user_data_for_invoke_;
                user_data_for_invoke_ = nullptr;
            }
        }

        void *mk_user_data_for_invoke(Proxy *proxy)
        {
            if(user_data_for_invoke_ != nullptr)
            {
                log_async_user_data_ptr(true, __func__);
                delete user_data_for_invoke_;
            }

            user_data_for_invoke_ = new UserDataForInvokeType(proxy, this,
                                                              cookie_);
            return user_data_for_invoke_;
        }

        UserDataForInvokeType *take_user_data_for_invoke()
        {
            if(user_data_for_invoke_ == nullptr)
                log_async_user_data_ptr(false, __func__);

            auto *result = user_data_for_invoke_;
            user_data_for_invoke_ = nullptr;
            return result;
        }

        bool has_cookie(uint32_t cookie) const { return cookie == cookie_; }
        uint32_t get_cookie() const { return cookie_; }

        /*!
         * Finish and pick up result of a D-Bus call.
         *
         * In case of any failure, the return values passed back via pointer
         * parameters will not have been initialized, and therefore they must
         * \e not be accessed.
         */
        template <typename Tag, typename... Args,
                  typename MCTraits = MethodCallerTraits<Tag>>
        [[nodiscard]]
        bool finish(Proxy &proxy, Args &&... args)
        {
            GErrorWrapper error;
            MCTraits::finish(proxy.proxy_, std::forward<Args>(args)...,
                             result_, error.await());
            result_ = nullptr;
            return !error.log_failure("Async D-Bus call");
        }

        GCancellable *get_cancellable() const { return cancellable_; }

        void put_result(Proxy &proxy, GAsyncResult *res)
        {
            result_ = res;
            done_(proxy, *this);
        }
    };

  private:
    std::function<void(Proxy &proxy, bool)> notify_;
    T *proxy_;

    std::unordered_map<AsyncCall *, std::unique_ptr<AsyncCall>> pending_calls_;

    template <typename Tag, typename... UserDataT>
    static void delete_handler_data(gpointer data, GClosure *closure)
    {
        delete static_cast<SignalHandlerData<Tag, UserDataT...> *>(data);
    }

  public:
    Proxy(Proxy &&) = default;
    Proxy &operator=(Proxy &&) = default;

  private:
    explicit Proxy(std::string &&name, std::string &&object_path,
                   T *proxy = nullptr):
        ProxyBase(std::move(name), std::move(object_path)),
        proxy_(proxy)
    {}

  public:
    /*!
     * Create D-Bus proxy to object at \p object_path at service \p name.
     */
    static Proxy make_proxy(std::string &&name, std::string &&object_path)
    {
        return Proxy(std::move(name), std::move(object_path));
    }

    /*!
     * Create D-Bus proxy to object at \p object_path at the sender who sent
     * the \p invocation.
     */
    static Proxy make_proxy(GDBusMethodInvocation *invocation,
                            std::string &&object_path)
    {
        return Proxy(g_dbus_message_get_sender(g_dbus_method_invocation_get_message(invocation)),
                     std::move(object_path));
    }

    /*!
     * Convenience function template for creating a unique pointer to a D-Bus
     * proxy.
     */
    template <typename RefT>
    static std::unique_ptr<Proxy>
    make_unique_proxy(RefT &&ref, std::string &&object_path)
    {
        return std::make_unique<Proxy>(make_proxy(std::forward<RefT>(ref),
                                                  std::forward<std::string>(object_path)));
    }

    /*!
     * Convenience function template for creating a heap-allocated object to a
     * D-Bus proxy.
     *
     * The caller must either call \c delete on the returned pointer to free
     * it, or turn it into a smart pointer at some point.
     */
    template <typename RefT>
    static Proxy *
    make_new_proxy(RefT &&ref, std::string &&object_path)
    {
        return new Proxy(make_proxy(std::forward<RefT>(ref),
                                    std::forward<std::string>(object_path)));
    }

    static Proxy make_proxy_for_testing(std::string &&name,
                                        std::string &&object_path,
                                        unsigned long pattern)
    {
        return Proxy(std::move(name), std::move(object_path),
                     reinterpret_cast<T *>(pattern));
    }

    /*!
     * Connect to a D-Bus object by creating a proxy object for it.
     *
     * This function returns immediately as the internal proxy object is
     * created asynchronously.
     *
     * \param connection
     *     The D-Bus connection the D-Bus object is expected to be found on.
     *
     * \param notify
     *     An optional callback which is called when the internal D-Bus proxy
     *     object has been created, or its creation has failed from some
     *     reason. When called, a reference to this #TDBus::Proxy object will
     *     be passed in its first argument, and the second argument will tell
     *     whether or not the D-Bus proxy creation was successful.
     */
    void connect_proxy(GDBusConnection *connection,
                       std::function<void(Proxy &proxy, bool)> &&notify = nullptr)
    {
        if(is_busy_)
        {
            log_connect_proxy_bug(get_object_path().c_str(), get_name().c_str());
            return;
        }

        if(proxy_ != nullptr)
            return;

        notify_ = std::move(notify);
        is_busy_ = true;

        (*Traits::proxy_new_fn())(connection, G_DBUS_PROXY_FLAGS_NONE,
                                  get_name().c_str(), get_object_path().c_str(),
                                  nullptr, connect_done, this);
    }

    /*!
     * Connect to a D-Bus object by creating a proxy object for it.
     *
     * This version uses a \c GDBusMethodInvocation to determine the
     * corresponding \c GDBusConnection object.
     */
    void connect_proxy(GDBusMethodInvocation *invocation,
                       std::function<void(Proxy &proxy, bool)> &&notify = nullptr)
    {
        return connect_proxy(g_dbus_method_invocation_get_connection(invocation),
                             std::move(notify));
    }

    /*!
     * Connect D-Bus signal reception to its handler.
     *
     * Like #TDBus::Iface::connect_default_method_handler(), but for D-Bus
     * signals and using #TDBus::SignalHandlerTraits.
     */
    template <typename Tag, typename SHTraits = SignalHandlerTraits<Tag>>
    Proxy &connect_default_signal_handler()
    {
        static_assert(std::is_same<typename SHTraits::IfaceType, T>::value,
                      "Handler is not meant to be used with this interface");
        g_signal_connect(proxy_, SHTraits::glib_signal_name(),
                         G_CALLBACK(SHTraits::signal_handler), this);
        return *this;
    }

    /*!
     * Connect D-Bus signal reception to its handler.
     *
     * Like #TDBus::Iface::connect_method_handler(), but for D-Bus
     * signals and using #TDBus::SignalHandlerTraits.
     */
    template <typename Tag, typename... UserDataT,
              typename SHTraits = SignalHandlerTraits<Tag>>
    Proxy &connect_signal_handler(typename SHTraits::template HandlerType<UserDataT...> fn,
                                  UserDataT &&... user_data)
    {
        static_assert(std::is_same<typename SHTraits::IfaceType, T>::value,
                      "Handler is not meant to be used with this interface");
        g_signal_connect_data(
            proxy_, SHTraits::glib_signal_name(), G_CALLBACK(fn),
            new SignalHandlerData<Tag, UserDataT...>(
                    *this, std::forward<UserDataT>(user_data)...),
            delete_handler_data<Tag, UserDataT...>,
            GConnectFlags(0));
        return *this;
    }

    /*!
     * Connect D-Bus signal reception to a handler function.
     *
     * Use this version of this function to connect to a signal handler
     * function which won't make use of the user data passed from GLib. The
     * pointer will always be \c nullptr if connecting through this function.
     */
    template <typename Tag, typename SHTraits = SignalHandlerTraits<Tag>>
    Proxy &connect_signal_handler(typename SHTraits::ClassicHandlerType fn)
    {
        static_assert(std::is_same<typename SHTraits::IfaceType, T>::value,
                      "Handler is not meant to be used with this interface");
        g_signal_connect(proxy_, SHTraits::glib_signal_name(),
                         G_CALLBACK(fn), nullptr);
        return *this;
    }

    template <typename Tag, typename... Args,
              typename MCTraits = MethodCallerTraits<Tag>>
    void call_and_forget(Args &&... args)
    {
        static_assert(std::is_same<typename MCTraits::IfaceType, T>::value,
                      "Call is not meant to be used with this interface");

        if(proxy_ != nullptr)
            MCTraits::invoke(proxy_, std::forward<Args>(args)...,
                             nullptr, nullptr, nullptr);
        else
            log_proxy_not_connected(MethodHandlerTraits<Tag>::dbus_method_name());
    }

    /*!
     * Asynchronous call of D-Bus method.
     *
     * The return value is a handle for canceling the method call. If its value
     * is \c UINT64_MAX, then the call has failed; otherwise, you may pass it
     * to #TDBus::Proxy::cancel_async_call_by_cookie(), or ignore it.
     *
     * The only way this function can fail is by calling it while having no
     * D-Bus proxy associated. Applications should handle this failure
     * regardless because the \p done function will not be called in this case,
     * thus probably leaving the application handing in some way.
     */
    template <typename Tag, typename... Args,
              typename MCTraits = MethodCallerTraits<Tag>>
    [[nodiscard]]
    uint64_t call(std::function<void(Proxy &, AsyncCall &)> &&done, Args &&... args)
    {
        static_assert(std::is_same<typename MCTraits::IfaceType, T>::value,
                      "Call is not meant to be used with this interface");

        if(proxy_ == nullptr)
        {
            log_proxy_not_connected(MethodHandlerTraits<Tag>::dbus_method_name());
            return UINT64_MAX;
        }

        auto c(std::make_unique<AsyncCall>(std::move(done)));
        MCTraits::invoke(proxy_, std::forward<Args>(args)...,
                         c->get_cancellable(), method_done,
                         c->mk_user_data_for_invoke(this));
        auto *const cptr = c.get();
        pending_calls_.emplace(cptr, std::move(c));
        return cptr->get_cookie();
    }

    /*!
     * Synchronous call of D-Bus method.
     *
     * This function may fail (return \c false) because no D-Bus proxy is
     * associated, or on D-Bus errors such as a timeout. In case of any
     * failure, the return values passed back via pointer parameters will not
     * have been initialized, and therefore they must \e not be accessed.
     */
    template <typename Tag, typename... Args,
              typename MCTraits = MethodCallerTraits<Tag>>
    [[nodiscard]]
    bool call_sync(Args &&... args) const
    {
        static_assert(std::is_same<typename MCTraits::IfaceType, T>::value,
                      "Call is not meant to be used with this interface");

        if(proxy_ == nullptr)
        {
            log_proxy_not_connected(MethodHandlerTraits<Tag>::dbus_method_name());
            return false;
        }

        GErrorWrapper error;
        MCTraits::invoke_sync(proxy_, std::forward<Args>(args)...,
                              nullptr, error.await());
        return !error.log_failure("Sync D-Bus call");
    }

    void cancel_async_call_by_cookie(uint64_t cookie_key)
    {
        if(cookie_key > UINT32_MAX)
            return;

        const auto cookie = uint32_t(cookie_key);
        const auto it = pending_calls_.find_if([cookie] (const auto &first, const auto &second)
                                               { return first->has_cookie(cookie); });
        if(it != pending_calls_.end())
            g_cancellable_cancel(it->get_cancellable());
    }

    /*!
     * DO NOT CALL FROM PRODUCTION CODE. This is for unit tests only.
     *
     * In production code, there is a pointer to a small object which tells our
     * asychronous ready callback which #TDBus::Proxy and which
     * #TDBus::Proxy::AsyncCall objects belong to the callback invocation. This
     * pointer is allocated via plain \c new and passed as \c user_data to the
     * method invocation function. The asychronous ready callback catches it
     * again and frees it via \c delete. In the meantime, the pointer is
     * managed somewhere inside of GLib/GIO, usually out of reach for anybody.
     * This is fine in regular operation because at some point, the callback
     * will be called and the object will be deleted.
     *
     * In unit tests, however, we may want to stop after testing some fraction
     * of code, possibly leaving unfinished method invacations around. This
     * causes memory leak which are detected by Valgrind. To avoid this, unit
     * tests need to call this function to tell they are ready and we need to
     * clean up. The unit tests must check the return value of this function so
     * that the expected number of pending method invocations can be verified.
     *
     * We could also try to avoid leaking memory by just passing a plain
     * #TDBus::Proxy::AsyncCall object pointer to the invoke function and
     * having this object point back to its proxy. Sometimes, however,
     * especially after cancellation or during program termination, the ready
     * function gets called with a dangling pointer, after the
     * #TDBus::Proxy::AsyncCall object has been deleted somewhere else. This is
     * because ownership is unclear with a plain pointer. Since GLib is just
     * plain C and thus doesn't support \c std::shared_ptr (which would be the
     * easiest and cleanest way to deal with this situation), and we can only
     * pass a plain pointer when calling a D-Bus method (and no, we will *NOT*
     * use their ref-counting as we want to minimize GLib utilization), we
     * create a small object which points to the #TDBus::Proxy::AsyncCall and
     * to its corresponding proxy, and store the pointer to that object in the
     * #TDBus::Proxy associated with the asynchronous call. Before the call
     * object is used, it is looked up in the proxy, and before it is deleted,
     * it gets removed from the proxy. This way, ownership is clear and
     * dangling pointers passed by GIO can be detected.
     *
     * To clean up from within unit tests, we need to walk through the pending
     * method invocations and clean them up. It cannot be done from unit tests
     * because while calling \c delete on the observed \c user_data would be
     * possible, clearing the pending calls in the proxy would not.
     *
     * To get at the user data pointer passed to the D-Bus method invocation
     * function, we simply store it directly in the #TDBus::Proxy::AsyncCall
     * object. The #TDBus::Proxy::AsyncCall doesn't own the object and there is
     * no production code which uses this pointer (except for the function
     * which initializes it, and the move operator), but we are using it here
     * to implement this function. Our function cleans up very bluntly by
     * deleting all known user data and then clearing the container which
     * stores the pending calls. This is OK because in unit tests, our mocks
     * take the role of GIO. There is no way to mess up with GIO's internal
     * state because it isn't used during unit tests.
     *
     * DO NOT CALL FROM PRODUCTION CODE. This is for unit tests only.
     */
    size_t kill_unfinished_calls()
    {
        for(auto &it : pending_calls_)
            delete it.first->take_user_data_for_invoke();

        const auto result(pending_calls_.size());
        pending_calls_.clear();
        return result;
    }

  private:
    static void connect_done(GObject *source_object, GAsyncResult *res,
                             gpointer user_data)
    {
        GErrorWrapper error;
        auto proxy = (*Traits::proxy_new_finish_fn())(res, error.await());
        const bool result = !error.log_failure("Create D-Bus proxy");
        static_cast<Proxy *>(user_data)->ready(proxy, result);
    }

    void ready(T *proxy, bool result)
    {
        proxy_ = proxy;
        is_busy_ = false;

        if(notify_ == nullptr)
            return;

        const auto notify(std::move(notify_));
        notify(*this, result);
    }

    static void method_done(GObject *source_object, GAsyncResult *res,
                            gpointer user_data)
    {
        auto *const data(static_cast<typename AsyncCall::UserDataForInvokeType *>(user_data));
        Proxy &proxy(*std::get<0>(*data));
        AsyncCall *acall(std::get<1>(*data));
        const auto &pending_call(proxy.pending_calls_.find(acall));

        if(pending_call != proxy.pending_calls_.end() &&
           pending_call->first->has_cookie(std::get<2>(*data)))
        {
            try
            {
                acall->put_result(proxy, res);
            }
            catch(const std::exception &e)
            {
                log_method_done_exception_bug(e.what());
            }

            if(acall->take_user_data_for_invoke() != data)
                log_bad_async_user_data_ptr();

            proxy.pending_calls_.erase(acall);
        }
        else
        {
            /* stray return of already deleted async call */
        }

        delete data;
    }
};

/*!
 * Return a singleton proxy object for given interface type \p T.
 */
template <typename T>
Proxy<T> &get_singleton();

/*!
 * Return a singleton interface object for given interface type \p T.
 */
template <typename T>
Iface<T> &get_exported_iface();

/*!
 * Observe presence of a specific name on a D-Bus connection.
 *
 * While it is perfectly possible to use watchers directly in client code,
 * using #TDBus::Bus::add_watcher() is simpler and often sufficient.
 */
class PeerWatcher
{
  private:
    const std::string name_;
    const std::function<void(GDBusConnection *, const char *)> name_appeared_;
    const std::function<void(GDBusConnection *, const char *)> name_vanished_;

    guint watcher_id_;

  public:
    PeerWatcher(const PeerWatcher &) = delete;
    PeerWatcher(PeerWatcher &&) = default;
    PeerWatcher &operator=(const PeerWatcher &) = delete;
    PeerWatcher &operator=(PeerWatcher &&) = delete;

    explicit PeerWatcher(const char *name,
                         std::function<void(GDBusConnection *, const char *)> &&name_appeared,
                         std::function<void(GDBusConnection *, const char *)> &&name_vanished):
        name_(name),
        name_appeared_(std::move(name_appeared)),
        name_vanished_(std::move(name_vanished)),
        watcher_id_(0)
    {}

    ~PeerWatcher() { stop(); }

    void start(GDBusConnection *connection);
    void stop();

  private:
    static void appeared(GDBusConnection *connection,
                         const gchar *name, const gchar *name_owner,
                         gpointer user_data);
    static void vanished(GDBusConnection *connection,
                         const gchar *name, gpointer user_data);
};

/*!
 * D-Bus connection.
 */
class Bus
{
  public:
    enum class Type
    {
        SESSION,
        SYSTEM,
    };

  private:
    const std::string object_name_;
    const Type bus_type_;

    std::function<void(GDBusConnection *)> on_connect_;
    std::function<void(GDBusConnection *)> on_name_acquired_;
    std::function<void(GDBusConnection *)> on_name_lost_;

    guint owner_id_;
    std::list<PeerWatcher> watchers_;
    std::list<const IfaceBase *> interfaces_;

  public:
    Bus(const Bus &) = delete;
    Bus(Bus &&) = default;
    Bus &operator=(const Bus &) = delete;
    Bus &operator=(Bus &&) = delete;

    explicit Bus(const char *object_name, Type t);
    ~Bus();

    void add_watcher(const char *name,
                     std::function<void(GDBusConnection *, const char *)> &&appeared,
                     std::function<void(GDBusConnection *, const char *)> &&vanished)
    {
        watchers_.emplace_back(name, std::move(appeared), std::move(vanished));
    }

    void add_auto_exported_interface(const IfaceBase &iface)
    {
        interfaces_.push_back(&iface);
    }

    bool connect(std::function<void(GDBusConnection *)> &&on_connect,
                 std::function<void(GDBusConnection *)> &&on_name_acquired,
                 std::function<void(GDBusConnection *)> &&on_name_lost);

  private:
    static void bus_acquired(GDBusConnection *connection,
                             const gchar *name, gpointer user_data);
    static void name_acquired(GDBusConnection *connection,
                              const gchar *name, gpointer user_data);
    static void name_lost(GDBusConnection *connection,
                          const gchar *name, gpointer user_data);
};

}

#endif /* !TADDYBUS_HH */
