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
 *              Gary Wang  <gary.wang@canonical.com>
 */
#ifndef CORE_NET_HTTP_STREAMING_REQUEST_H_
#define CORE_NET_HTTP_STREAMING_REQUEST_H_

#include <core/net/http/request.h>

namespace core
{
namespace net
{
namespace http
{
/**
 * @brief The StreamingRequest class encapsulates a request for a web resource,
 * streaming data to the receiver as it receives in addition to accumulating all incoming data.
 */
class CORE_NET_DLL_PUBLIC StreamingRequest : public Request
{
public:

    /** DataHandler is invoked when a new chunk of data arrives from the server. */
    typedef std::function<void(const std::string&)> DataHandler;

    /**
     * @brief Synchronously executes the request.
     * @throw core::net::http::Error in case of http-related errors.
     * @throw core::net::Error in case of network-related errors.
     * @return The response to the request.
     */
    virtual Response execute(const ProgressHandler& ph, const DataHandler& dh) = 0;

    /**
     * @brief Asynchronously executes the request, reporting errors, progress and completion to the given handlers.
     * @param handler The handlers to called for events happening during execution of the request.
     * @param dh The data handler receiving chunks of data while executing the request.
     * @return The response to the request.
     */
    virtual void async_execute(const Handler& handler, const DataHandler& dh) = 0;

    /** 
     * @brief Pause the request with options for aborting the request.
     * The request will be aborted if transfer speed falls below \a limit in [bytes/second] for \a time seconds.
     * @throw core::net::http::Error in case of http-related errors.
     */
    virtual void pause() = 0;
    
    /** 
     * @brief Resume the request
     * @throw core::net::http::Error in case of http-related errors.
     */
    virtual void resume() = 0;
  
    /** 
     * @brief Sets options for aborting the request.
     * The request will be aborted if transfer speed belows \a limit bytes per second for \a time seconds
     * @param limit The transfer speed in seconds.
     * @param time waiting period(seconds) to abort the request.
     */
    virtual void abort_request_if(std::uint64_t limit, const std::chrono::seconds& time) = 0; 
};
}
}
}

#endif // CORE_NET_HTTP_STREAMING_REQUEST_H_
