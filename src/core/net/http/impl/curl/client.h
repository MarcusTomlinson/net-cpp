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
#ifndef CORE_NET_HTTP_IMPL_CURL_CLIENT_H_
#define CORE_NET_HTTP_IMPL_CURL_CLIENT_H_

#include <core/net/http/client.h>

#include "curl.h"

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
class Client : public core::net::http::Client
{
public:
    Client();

    // From core::net::http::Client

    std::string url_escape(const std::string& s) const;

    core::net::http::Client::Timings timings();

    void run();

    void stop();

    std::shared_ptr<Request> get(const Request::Configuration& configuration);

    std::shared_ptr<Request> head(const Request::Configuration& configuration);

    std::shared_ptr<Request> post(const Request::Configuration& configuration, const std::string&, const std::string&);

    std::shared_ptr<Request> put(const Request::Configuration& configuration, std::istream& payload, std::size_t size);

private:
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
