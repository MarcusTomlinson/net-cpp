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
#ifndef CORE_NET_HTTP_IMPL_CURL_CLIENT_H_
#define CORE_NET_HTTP_IMPL_CURL_CLIENT_H_

#include <core/net/http/streaming_client.h>

#include "curl.h"
#include "request.h"

namespace core
{
namespace net
{
namespace http
{
namespace impl
{
namespace curl
{
class Client : public core::net::http::StreamingClient
{
public:
    Client();

    // From core::net::http::Client

    std::string url_escape(const std::string& s) const;

    std::string base64_encode(const std::string& s) const override;

    std::string base64_decode(const std::string& s) const override;

    core::net::http::Client::Timings timings() override;

    void run() override;

    void stop() override;

    std::shared_ptr<http::Request> get(const Request::Configuration& configuration) override;
    std::shared_ptr<http::Request> head(const Request::Configuration& configuration) override;
    std::shared_ptr<http::Request> post(const Request::Configuration& configuration, const std::string&, const std::string&) override;
    std::shared_ptr<http::Request> post(const Request::Configuration& configuration, std::istream& payload, std::size_t size) override;
    std::shared_ptr<http::Request> put(const Request::Configuration& configuration, std::istream& payload, std::size_t size) override;
    std::shared_ptr<http::Request> del(const Request::Configuration& configuration) override;

    std::shared_ptr<http::StreamingRequest> streaming_get(const Request::Configuration& configuration) override;
    std::shared_ptr<http::StreamingRequest> streaming_head(const Request::Configuration& configuration) override;
    std::shared_ptr<http::StreamingRequest> streaming_put(const Request::Configuration& configuration, std::istream& payload, std::size_t size) override;
    std::shared_ptr<http::StreamingRequest> streaming_post(const Request::Configuration& configuration, const std::string& payload, const std::string& type) override;
    std::shared_ptr<http::StreamingRequest> streaming_post(const Request::Configuration& configuration, std::istream& payload, std::size_t size) override;
    std::shared_ptr<http::StreamingRequest> streaming_post_form(const Request::Configuration& configuration, const std::map<std::string, std::string>& values) override;
    std::shared_ptr<http::StreamingRequest> streaming_del(const Request::Configuration& configuration) override;

private:
    std::shared_ptr<curl::Request> get_impl(const Request::Configuration& configuration);
    std::shared_ptr<curl::Request> head_impl(const Request::Configuration& configuration);
    std::shared_ptr<curl::Request> post_impl(const Request::Configuration& configuration, const std::string&, const std::string&);
    std::shared_ptr<curl::Request> post_impl(const Request::Configuration& configuration, std::istream& payload, std::size_t size);
    std::shared_ptr<curl::Request> put_impl(const Request::Configuration& configuration, std::istream& payload, std::size_t size);
    std::shared_ptr<curl::Request> del_impl(const Request::Configuration& configuration);

    ::curl::multi::Handle multi;    
};
}
}
// Create an instance of a client implementation.
std::shared_ptr<Client> make_client();
}
}
}
#endif // CORE_NET_HTTP_IMPL_CURL_CLIENT_H_
