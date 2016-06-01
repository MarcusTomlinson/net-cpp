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
#ifndef CORE_NET_HTTP_STREAMING_CLIENT_H_
#define CORE_NET_HTTP_STREAMING_CLIENT_H_

#include <core/net/http/client.h>

#include <core/net/http/streaming_request.h>

namespace core
{
namespace net
{
namespace http
{
class StreamingClient : public Client
{
public:

    virtual ~StreamingClient() = default;

    /**
    * @brief streaming_get is a convenience method for issueing a GET request for the given URI.
    * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
    * @param configuration The configuration to issue a get request for.
    * @return An executable instance of class Request.
    */
    virtual std::shared_ptr<StreamingRequest> streaming_get(const Request::Configuration& configuration) = 0;

    /**
    * @brief streaming_head is a convenience method for issueing a HEAD request for the given URI.
    * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
    * @param configuration The configuration to issue a get request for.
    * @return An executable instance of class Request.
    */
    virtual std::shared_ptr<StreamingRequest> streaming_head(const Request::Configuration& configuration) = 0;

    /**
    * @brief streaming_put is a convenience method for issuing a PUT request for the given URI.
    * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
    * @param configuration The configuration to issue a get request for.
    * @param payload The data to be transmitted as part of the PUT request.
    * @param size Size of the payload data in bytes.
    * @return An executable instance of class Request.
    */
    virtual std::shared_ptr<StreamingRequest> streaming_put(const Request::Configuration& configuration, std::istream& payload, std::size_t size) = 0;

    /**
    * @brief streaming_post is a convenience method for issuing a POST request for the given URI.
    * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
    * @param configuration The configuration to issue a get request for.
    * @param payload The data to be transmitted as part of the POST request.
    * @param type The content-type of the data.
    * @return An executable instance of class Request.
    */
    virtual std::shared_ptr<StreamingRequest> streaming_post(const Request::Configuration& configuration, const std::string& payload, const std::string& type) = 0;
    
    /**
    * @brief streaming_post is a convenience method for issuing a POST request for the given URI.
    * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the pro vided HTTP method.
    * @param configuration The configuration to issue a get request for.
    * @param payload The data to be transmitted as part of the POST request.
    * @param size Size of the payload data in bytes.
    * @return An executable instance of class Request.
    */
    virtual std::shared_ptr<StreamingRequest> streaming_post(const Request::Configuration& configuration, std::istream& payload, std::size_t size) = 0;
    
    /**
    * @brief streaming_post_form is a convenience method for issuing a POST request for the given URI, with url-encoded payload.
    * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
    * @param configuration The configuration to issue a get request for.
    * @param values Key-value pairs to be added to the payload in url-encoded format.
    * @return An executable instance of class Request.
    */
    virtual std::shared_ptr<StreamingRequest> streaming_post_form(const Request::Configuration& configuration, const std::map<std::string, std::string>& values) = 0;


    /**
    * @brief streaming_del is a convenience method for issuing a DELETE request for the given URI.
    * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
    * @param configuration The configuration to issue a del request for.
    * @return An executable instance of class Request.
    */
    virtual std::shared_ptr<StreamingRequest> streaming_del(const Request::Configuration& configuration) = 0;
};

/** @brief Dispatches to the default implementation and returns a streaming client instance. */
CORE_NET_DLL_PUBLIC std::shared_ptr<StreamingClient> make_streaming_client();
}
}
}
#endif // CORE_NET_HTTP_STREAMING_CLIENT_H_
