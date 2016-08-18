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
#ifndef CORE_NET_HTTP_IMPL_CURL_MULTI_H_
#define CORE_NET_HTTP_IMPL_CURL_MULTI_H_

#include "easy.h"

namespace curl
{
namespace multi
{
// Return codes for native curl multi functions.
enum class Code
{
    // Indicates to the app that curl_multi_perform should be called.
    call_multi_perform = CURLM_CALL_MULTI_PERFORM,
    // All good.
    ok = CURLM_OK,
    // A bad native curl multi handle has been passed.
    bad_handle = CURLM_BAD_HANDLE,
    // A bad native curl easy handle has been passed.
    bad_easy_handle = CURLM_BAD_EASY_HANDLE,
    // Out of memory.
    out_of_memory = CURLM_OUT_OF_MEMORY,
    // An internal libcurl bug occured.
    internal_error = CURLM_INTERNAL_ERROR,
    // The underlying socket went havoc.
    bad_socket = CURLM_BAD_SOCKET,
    // An unknown option has been passed to curl::multi::native::set.
    unknown_option = CURLM_UNKNOWN_OPTION,
    // The native curl easy handle is already known to the curl multi instance.
    added_already = CURLM_ADDED_ALREADY
};

std::ostream& operator<<(std::ostream& out, Code code);

// Known options that can be set on a curl multi instance.
enum class Option
{
    // Controls pipelining behavior for multiple connections.
    // Expects a long value, pass 1 for enabling, 0 for disabling.
    pipelining = CURLMOPT_PIPELINING,
    // Callback function for associating a socket with an alien event loop.
    socket_function = CURLMOPT_SOCKETFUNCTION,
    // Cookie passed to invocation of the socket callback function.
    socket_data = CURLMOPT_SOCKETDATA,
    // Callback function for associating a timeout with an alien event loop.
    timer_function = CURLMOPT_TIMERFUNCTION,
    // Cookie passed to an invocation of the timeout callback function.
    timer_data = CURLMOPT_TIMERDATA
};

// Throws a std::runtime_error if the parameter to the function does match the
// constant templated value.
template<Code ref>
inline void throw_if(Code code)
{
    if (code == ref)
    {
        std::stringstream ss; ss << code;
        throw std::system_error(static_cast<int>(code), std::generic_category(), ss.str());
    }
}

// Throws a std::runtime_error if the parameter to the function does not match the
// constant templated value.
template<Code ref>
inline void throw_if_not(Code code)
{
    if (code != ref)
    {
        std::stringstream ss; ss << code;
        throw std::system_error(static_cast<int>(code), std::generic_category(), ss.str());
    }
}

// All curl native types and functions go here.
namespace native
{
// A message as popped when reading off infos from a native Handle.
typedef CURLMsg* Message;

// An opaque handle to a curl multi instance.
typedef CURLM* Handle;

// Underlying type of a socket.
typedef curl_socket_t Socket;

// Creates a new curl multi instance. Returns a nullptr in case of issues.
Handle init();

// Cleans up any resources associated with a curl multi instance.
Code cleanup(Handle handle);

// Sets an option on a native curl multi instance.
template<typename T>
inline Code set(Handle handle, Option option, T value)
{
    return static_cast<Code>(curl_multi_setopt(handle, static_cast<CURLMoption>(option), value));
}

// Assigns the cookie to the socket in the underlying implementation.
Code assign(Handle handle, Socket socket, void* cookie);

// Adds a native curl easy handle to a curl multi instance.
Code add_handle(Handle, curl::easy::native::Handle);

// Removes a native curl easy handle from a curl multi instance.
Code remove_handle(Handle, curl::easy::native::Handle);

// Pops a message off a curl multi instance.
std::pair<Message, int> read_info(Handle);

// Reports socket/timeout activity to a curl multi.instance.
std::pair<Code, int> socket_action(Handle handle, Socket socket, int events);
}

// Wrapper class for a native curl multi handle.
class Handle
{
public:
    // Creates a new instance and initializes a new curl multi instance.
    Handle();

    // Queries statistics about the timing information of the last transfers.
    core::net::http::Client::Timings timings();

    // Executes the underlying dispatcher executing the curl multi instance.
    // Can be called multiple times for thread-pool use-cases.
    void run();

    // Stops execution of the underlying dispatcher.
    // Only needs to be called once to be able to join all threads who are blocked in run().
    void stop();

    // Adds and schedules a new curl easy handle for execution.
    // Throws std::system_error in case of issues.
    void add(curl::easy::Handle easy);

    // Removes a previously added curl easy handle.
    // Throws std::system_error in case of issues.
    void remove(curl::easy::Handle easy);

    // Sets an option of the curl multi instance.
    // Throw std:: runtime_error in case of issues.
    template<typename T>
    inline void set_option(Option option, T value)
    {
        throw_if_not<Code::ok>(native::set(native(), option, value));
    }

    // Resolves a given native easy handle to its wrapper.
    // Throws std::runtime_error if the native handle cannot be resolved.
    curl::easy::Handle easy_handle_from_native(curl::easy::native::Handle native);

    // Returns the native curl multi instance handle.
    native::Handle native() const;

    // Dispatch dispatches task on the underlying reactor.
    void dispatch(const std::function<void()>& task);

private:
    struct Private;
    std::shared_ptr<Private> d;
};
}
}
#endif // CORE_NET_HTTP_IMPL_CURL_MULTI_H_
