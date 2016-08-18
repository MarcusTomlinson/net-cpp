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
#ifndef CORE_NET_HTTP_CLIENT_H_
#define CORE_NET_HTTP_CLIENT_H_

#include <core/net/visibility.h>

#include <core/net/http/method.h>
#include <core/net/http/request.h>

#include <chrono>
#include <iosfwd>
#include <memory>

namespace core
{
namespace net
{

struct Uri;

namespace http
{
class ContentType;
class Request;

class CORE_NET_DLL_PUBLIC Client
{
public:
    /** @brief Summarizes error conditions. */
    struct Errors
    {
        Errors() = delete;

        /** @brief HttpMethodNotSupported is thrown if the underlying impl.
         * does not support the requested HTTP method.
         */
        struct HttpMethodNotSupported : public http::Error
        {
            HttpMethodNotSupported(Method method, const core::Location&);
            Method method;
        };
    };

    /** @brief Summarizes timing information about completed requests. */
    struct Timings
    {
        typedef std::chrono::duration<double> Seconds;

        struct Statistics
        {
            /** Maximum duration that was encountered. */
            Seconds max{Seconds::max()};
            /** Minimum duration that was encountered. */
            Seconds min{Seconds::max()};
            /** Mean duration that was encountered. */
            Seconds mean{Seconds::max()};
            /** Variance in duration that was encountered. */
            Seconds variance{Seconds::max()};
        };

        /** Time it took from the start until the name resolving was completed. */
        Statistics name_look_up{};
        /** Time it took from the finished name lookup until the connect to the
         * remote host (or proxy) was completed.
         */
        Statistics connect{};
        /** Time it took from the connect until the SSL/SSH connect/handshake to
         * the remote host was completed.
         */
        Statistics app_connect{};
        /** Time it took from app_connect until the file transfer is just about to begin. */
        Statistics pre_transfer{};
        /** Time it took from pre-transfer until the first byte is received by libcurl. */
        Statistics start_transfer{};
        /** Time in total that the previous transfer took. */
        Statistics total{};
    };

    Client(const Client&) = delete;
    virtual ~Client() = default;

    Client& operator=(const Client&) = delete;
    bool operator==(const Client&) const = delete;

    virtual std::string uri_to_string (const core::net::Uri& uri) const;

    /** @brief Percent-encodes the given string. */
    virtual std::string url_escape(const std::string& s) const = 0;

    /** @brief Base64-encodes the given string. */
    virtual std::string base64_encode(const std::string& s) const = 0;

    /** @brief Base64-decodes the given string. */
    virtual std::string base64_decode(const std::string& s) const = 0;

    /** @brief Queries timing statistics over all requests that have been executed by this client. */
    virtual Timings timings() = 0;

    /** @brief Execute the client and any impl-specific thread-pool or runtime. */
    virtual void run() = 0;

    /** @brief Stop the client and any impl-specific thread-pool or runtime. */
    virtual void stop() = 0;

    /**
     * @brief get is a convenience method for issueing a GET request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> get(const Request::Configuration& configuration) = 0;

    /**
     * @brief head is a convenience method for issueing a HEAD request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> head(const Request::Configuration& configuration) = 0;

    /**
     * @brief put is a convenience method for issuing a PUT request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @param payload The data to be transmitted as part of the PUT request.
     * @param size Size of the payload data in bytes.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> put(const Request::Configuration& configuration, std::istream& payload, std::size_t size) = 0;

    /**
     * @brief post is a convenience method for issuing a POST request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @param payload The data to be transmitted as part of the POST request.
     * @param type The content-type of the data.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> post(const Request::Configuration& configuration, const std::string& payload, const std::string& type) = 0;

    /**
     * @brief post_form is a convenience method for issuing a POST request for the given URI, with url-encoded payload.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @param values Key-value pairs to be added to the payload in url-encoded format.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> post_form(const Request::Configuration& configuration, const std::map<std::string, std::string>& values);

    /**
     * @brief post is a convenience method for issuing a POST request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the pro vided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @param payload The data to be transmitted as part of the POST request.
     * @param size Size of the payload data in bytes.
     * @return An executable instance of class Request.
     */
    std::shared_ptr<Request> post(const Request::Configuration& configuration, std::istream& payload, std::size_t size);

    /** 
     * @brief del is a convenience method for issueing a DELETE request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @return An executable instance of class Request.
     */
    std::shared_ptr<Request> del(const Request::Configuration& configuration);

protected:
    Client() = default;
};

/** @brief Dispatches to the default implementation and returns a client instance. */
CORE_NET_DLL_PUBLIC std::shared_ptr<Client> make_client();
}
}
}

#endif // CORE_NET_HTTP_CLIENT_H_
