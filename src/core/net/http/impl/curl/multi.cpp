/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include "multi.h"

#include "easy.h"

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>

namespace acc = boost::accumulators;

namespace easy = ::curl::easy;
namespace multi = ::curl::multi;

namespace
{
struct SynchronizedHandleStore
{
    std::mutex guard;
    std::map<CURL*, easy::Handle> handles;

    void add(easy::Handle easy)
    {
        std::lock_guard<std::mutex> lg(guard);
        handles.insert(std::make_pair(easy.native(), easy));
    }

    void remove(easy::Handle easy)
    {
        std::lock_guard<std::mutex> lg(guard);
        handles.erase(easy.native());
    }

    easy::Handle lookup_native(CURL* native)
    {
        std::lock_guard<std::mutex> lg(guard);
        auto it = handles.find(native);

        if (it == handles.end())
            throw std::runtime_error("SynchronizedHandleStore::lookup_native: No such handle.");

        return it->second;
    }
};

template<typename T>
struct Holder
{
    std::shared_ptr<T> value;
};
}

std::ostream& multi::operator<<(std::ostream& out, multi::Code code)
{
    switch (code)
    {
    case multi::Code::call_multi_perform: out << "curl::multi::Code::call_multi_perform"; break;
    case multi::Code::ok: out << "curl::multi::Code::ok"; break;
    case multi::Code::bad_handle: out << "curl::multi::Code::bad_handle"; break;
    case multi::Code::bad_easy_handle: out << "curl::multi::Code::easy_handle"; break;
    case multi::Code::out_of_memory: out << "curl::multi::Code::out_of_memory"; break;
    case multi::Code::internal_error: out << "curl::multi::Code::internal_error"; break;
    case multi::Code::bad_socket: out << "curl::multi::Code::bad_socket"; break;
    case multi::Code::unknown_option: out << "curl::multi::Code::unknown_option"; break;
    case multi::Code::added_already: out << "curl::multi::Code::added_already"; break;
    }

    return out;
}

multi::native::Handle multi::native::init()
{
    return curl_multi_init();
}

multi::Code multi::native::cleanup(multi::native::Handle handle)
{
    return static_cast<multi::Code>(curl_multi_cleanup(handle));
}

multi::Code multi::native::assign(multi::native::Handle handle, multi::native::Socket socket, void* cookie)
{
    return static_cast<multi::Code>(curl_multi_assign(handle, socket, cookie));
}

multi::Code multi::native::add_handle(multi::native::Handle m, easy::native::Handle e)
{
    return static_cast<multi::Code>(curl_multi_add_handle(m, e));
}

multi::Code multi::native::remove_handle(multi::native::Handle m, easy::native::Handle e)
{
    return static_cast<multi::Code>(curl_multi_remove_handle(m, e));
}

std::pair<multi::native::Message, int> multi::native::read_info(multi::native::Handle handle)
{
    int count{-1};
    auto msg = curl_multi_info_read(handle, &count);

    return std::make_pair(msg, count);
}

std::pair<multi::Code, int> multi::native::socket_action(multi::native::Handle handle, multi::native::Socket socket, int events)
{
    int count{-1};
    auto rc = static_cast<multi::Code>(
                curl_multi_socket_action(
                    handle,
                    socket,
                    events,
                    &count));

    return std::make_pair(rc, count);
}

struct multi::Handle::Private
{
    void process_multi_info();

    static int timer_callback(
            multi::native::Handle,
            long timeout_ms,
            void* cookie);

    struct Timeout
    {
        Timeout(boost::asio::io_service& dispatcher);

        void cancel();
        void async_wait_for(
                const std::shared_ptr<Handle::Private>& context,
                const std::chrono::milliseconds& ms);

        struct Private : public std::enable_shared_from_this<Private>
        {
            Private(boost::asio::io_service& dispatcher);

            void cancel();

            void async_wait_for(
                    const std::weak_ptr<multi::Handle::Private>& context,
                    const std::chrono::milliseconds& ms);

            void handle_timeout(const std::shared_ptr<multi::Handle::Private>& context);

            boost::asio::deadline_timer timer;
        };
        std::shared_ptr<Private> d;
    };

    static int socket_callback(
            easy::native::Handle,
            multi::native::Socket s,
            int action,
            void* cookie,
            void* socket_cookie);

    struct Socket
    {
        Socket(boost::asio::io_service& dispatcher,
               multi::native::Socket native);
        ~Socket();

        void async_wait_for_readable(const std::weak_ptr<Handle::Private>& context);
        void async_wait_for_writeable(const std::weak_ptr<Handle::Private>& context);

        struct Private : public std::enable_shared_from_this<Private>
        {
            Private(boost::asio::io_service& dispatcher,
                    multi::native::Socket native);
            ~Private();

            void cancel();
            void async_wait_for_readable(const std::weak_ptr<Handle::Private>& context);
            void async_wait_for_writeable(std::weak_ptr<Handle::Private> context);

            bool cancel_requested;
            boost::asio::posix::stream_descriptor sd;
        };
        std::shared_ptr<Private> d;
    };

    typedef acc::accumulator_set<
        double,
        acc::stats<
            acc::tag::min,
            acc::tag::max,
            acc::tag::mean,
            acc::tag::variance
        >
    > Accumulator;

    static void fill_from_accumulator(core::net::http::Client::Timings::Statistics& stats,
                                      const multi::Handle::Private::Accumulator& accu)
    {
        typedef core::net::http::Client::Timings::Seconds Seconds;
        stats.max = Seconds{acc::max(accu)};
        stats.min = Seconds{acc::min(accu)};
        stats.mean = Seconds{acc::mean(accu)};
        stats.variance = Seconds{acc::variance(accu)};
    }

    Private();
    ~Private();

    void update_timings(const easy::Handle::Timings& timings);

    multi::native::Handle handle;
    boost::asio::io_service dispatcher;
    boost::asio::io_service::work keep_alive;
    std::mutex guard;
    SynchronizedHandleStore handle_store;
    Timeout timeout;

    struct
    {
        Accumulator for_name_look_up{};
        Accumulator for_connect{};
        Accumulator for_app_connect{};
        Accumulator for_pre_transfer{};
        Accumulator for_start_transfer{};
        Accumulator for_total{};
    } accumulator;

    struct Holder
    {
        std::weak_ptr<Private> value;
    } holder;
};

multi::Handle::Handle() : d(new Private())
{
    d->holder.value = d;

    set_option(Option::socket_function, Private::socket_callback);
    set_option(Option::socket_data, &d->holder);
    set_option(Option::timer_function, Private::timer_callback);
    set_option(Option::timer_data, &d->holder);

    set_option(Option::pipelining, easy::enable);
}

core::net::http::Client::Timings multi::Handle::timings()
{
    core::net::http::Client::Timings result;

    Private::fill_from_accumulator(result.name_look_up, d->accumulator.for_name_look_up);
    Private::fill_from_accumulator(result.connect, d->accumulator.for_connect);
    Private::fill_from_accumulator(result.app_connect, d->accumulator.for_app_connect);
    Private::fill_from_accumulator(result.pre_transfer, d->accumulator.for_pre_transfer);
    Private::fill_from_accumulator(result.start_transfer, d->accumulator.for_start_transfer);
    Private::fill_from_accumulator(result.total, d->accumulator.for_total);

    return result;
}

void multi::Handle::dispatch(const std::function<void ()> &task)
{
    d->dispatcher.post(task);
}

void multi::Handle::run()
{
    d->dispatcher.run();
}

void multi::Handle::stop()
{
    d->dispatcher.stop();
}

void multi::Handle::add(easy::Handle easy)
{
    std::lock_guard<std::mutex> lg(d->guard);

    d->handle_store.add(easy);
    multi::throw_if_not<multi::Code::ok>(
                multi::native::add_handle(
                    native(),
                    easy.native()));
}

void multi::Handle::remove(easy::Handle easy)
{
    d->handle_store.remove(easy);
    multi::throw_if_not<multi::Code::ok>(
                multi::native::remove_handle(
                    native(),
                    easy.native()));
}

curl::easy::Handle multi::Handle::easy_handle_from_native(easy::native::Handle native)
{
    return d->handle_store.lookup_native(native);
}

multi::native::Handle multi::Handle::native() const
{
    return d->handle;
}

void multi::Handle::Private::process_multi_info()
{
    while (true)
    {
        multi::native::Message msg;
        int count;

        std::tie(msg, count) = multi::native::read_info(handle);

        if (!msg)
            break;

        if (msg->msg == CURLMSG_DONE)
        {
            auto native_easy = msg->easy_handle;
            auto rc = static_cast<curl::Code>(msg->data.result);
            try
            {
                auto easy = handle_store.lookup_native(native_easy);

                update_timings(easy.timings());

                easy.notify_finished(rc);
                handle_store.remove(easy);
                multi::native::remove_handle(handle, native_easy);
            } catch(...)
            {
                std::cout << "Something weird happened" << std::endl;
            }
        }
    }
}

multi::Handle::Private::Timeout::Timeout(boost::asio::io_service& dispatcher) : d(new Private(dispatcher))
{
}

void multi::Handle::Private::Timeout::cancel()
{
}

void multi::Handle::Private::Timeout::async_wait_for(const std::shared_ptr<Handle::Private>& context, const std::chrono::milliseconds& ms)
{
    d->async_wait_for(context, ms);
}

multi::Handle::Private::Timeout::Private::Private(boost::asio::io_service& dispatcher) : timer(dispatcher)
{
}

void multi::Handle::Private::Timeout::Private::cancel()
{
    timer.cancel();
}

void multi::Handle::Private::Timeout::Private::async_wait_for(const std::weak_ptr<Handle::Private>& context, const std::chrono::milliseconds& ms)
{
    if (ms.count() > 0)
    {
        std::weak_ptr<Private> self{shared_from_this()};
        timer.expires_from_now(boost::posix_time::milliseconds{ms.count()});
        timer.async_wait([self, context](const boost::system::error_code& ec)
        {
            if (ec)
                return;

            if (auto spc = context.lock())
            {
                if (auto sp = self.lock())
                {
                    std::lock_guard<std::mutex> lg(spc->guard);
                    sp->handle_timeout(spc);
                }
            }
        });
    } else if (ms.count() == 0)
    {
        auto sp = context.lock();
        if (not sp)
            return;

        handle_timeout(sp);
    }
}

void multi::Handle::Private::Timeout::Private::handle_timeout(const std::shared_ptr<Handle::Private>& context)
{
    auto result = multi::native::socket_action(context->handle, CURL_SOCKET_TIMEOUT, 0);
    multi::throw_if_not<multi::Code::ok>(result.first);

    context->process_multi_info();
}

int multi::Handle::Private::timer_callback(
        multi::native::Handle,
        long timeout_ms,
        void* cookie)
{
    if (timeout_ms < 0)
        return -1;

    auto holder = static_cast<Private::Holder*>(cookie);

    if (!holder)
        return 0;

    auto thiz = holder->value.lock();

    thiz->timeout.async_wait_for(thiz, std::chrono::milliseconds{timeout_ms});

    return 0;
}

multi::Handle::Private::Socket::Socket(boost::asio::io_service& dispatcher,
                                       multi::native::Socket native)
    : d(new Private(dispatcher, native))
{
}

multi::Handle::Private::Socket::~Socket()
{
    d->cancel();
}

void multi::Handle::Private::Socket::async_wait_for_readable(const std::weak_ptr<multi::Handle::Private>& context)
{
    d->async_wait_for_readable(context);
}

void multi::Handle::Private::Socket::async_wait_for_writeable(const std::weak_ptr<multi::Handle::Private>& context)
{
    d->async_wait_for_writeable(context);
}

multi::Handle::Private::Socket::Socket::Private::Private(boost::asio::io_service& dispatcher,
                                                         multi::native::Socket native)
    : cancel_requested(false),
      sd(dispatcher, static_cast<int>(native))
{
}

multi::Handle::Private::Socket::Socket::Private::~Private()
{
    sd.release();
}

void multi::Handle::Private::Socket::Socket::Private::cancel()
{
    cancel_requested = true;
    sd.release();
}

void multi::Handle::Private::Socket::Socket::Private::async_wait_for_readable(const std::weak_ptr<multi::Handle::Private>& context)
{
    std::weak_ptr<Private> self{shared_from_this()};
    sd.async_read_some(boost::asio::null_buffers{}, [self, context](const boost::system::error_code& ec, std::size_t)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (auto sp = self.lock())
        {
            if (sp->cancel_requested)
                return;

            if (auto spc = context.lock())
            {
                std::lock_guard<std::mutex> lg(spc->guard);

                int bitmask{0};

                if (ec)
                    bitmask = CURL_CSELECT_ERR;
                else
                    bitmask = CURL_CSELECT_IN;

                auto result = multi::native::socket_action(spc->handle, sp->sd.native_handle(), bitmask);
                multi::throw_if_not<multi::Code::ok>(result.first);

                spc->process_multi_info();

                if (result.second <= 0)
                    spc->timeout.cancel();

                // Restart
                sp->async_wait_for_readable(context);
            }
        }
    });
}

void multi::Handle::Private::Socket::Socket::Private::async_wait_for_writeable(
        std::weak_ptr<multi::Handle::Private> context)
{
    std::weak_ptr<Private> self(shared_from_this());
    sd.async_write_some(boost::asio::null_buffers{}, [self, context](const boost::system::error_code& ec, std::size_t)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (auto sp = self.lock())
        {
            if (sp->cancel_requested)
                return;

            if (auto spc = context.lock())
            {
                std::lock_guard<std::mutex> lg(spc->guard);

                int bitmask{0};

                if (ec)
                    bitmask = CURL_CSELECT_ERR;
                else
                    bitmask = CURL_CSELECT_OUT;

                auto result = multi::native::socket_action(spc->handle, sp->sd.native_handle(), bitmask);
                multi::throw_if_not<multi::Code::ok>(result.first);

                spc->process_multi_info();

                if (result.second <= 0)
                    spc->timeout.cancel();
            }
            // Restart
            sp->async_wait_for_writeable(context);
        }
    });
}

int multi::Handle::Private::socket_callback(
        easy::native::Handle,
        multi::native::Socket s,
        int action,
        void* cookie,
        void* socket_cookie)
{
    static const int doc_tells_we_must_return_0{0};

    auto holder = static_cast<Private::Holder*>(cookie);

    if (!holder)
        return doc_tells_we_must_return_0;

    auto thiz = holder->value.lock();
    auto socket = static_cast<Socket*>(socket_cookie);

    if (!socket)
    {
        socket = new Socket{thiz->dispatcher, s};
        multi::throw_if_not<multi::Code::ok>(multi::native::assign(thiz->handle, s, socket));
    }

    switch (action)
    {
    case CURL_POLL_NONE:
    {
        break;
    }
    case CURL_POLL_IN:
        socket->async_wait_for_readable(thiz);
        break;
    case CURL_POLL_OUT:
        socket->async_wait_for_writeable(thiz);
        break;
    case CURL_POLL_INOUT:
        socket->async_wait_for_readable(thiz);
        socket->async_wait_for_writeable(thiz);
        break;
    case CURL_POLL_REMOVE:
    {
        multi::native::assign(thiz->handle, s, nullptr);
        delete socket;
        break;
    }
    }

    return doc_tells_we_must_return_0;
}

multi::Handle::Private::Private()
    : handle(multi::native::init()),
      keep_alive(dispatcher),
      timeout(dispatcher)
{
}

multi::Handle::Private::~Private()
{
    multi::native::cleanup(handle);
}

void multi::Handle::Private::update_timings(const easy::Handle::Timings& timings)
{
    accumulator.for_name_look_up(timings.name_look_up.count());
    accumulator.for_connect(timings.connect.count());
    accumulator.for_app_connect(timings.app_connect.count());
    accumulator.for_pre_transfer(timings.pre_transfer.count());
    accumulator.for_start_transfer(timings.start_transfer.count());
    accumulator.for_total(timings.total.count());
}
