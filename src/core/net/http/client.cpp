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

#include "impl/curl/client.h"

#include <core/net/uri.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>

#include <sstream>

namespace net = core::net;
namespace http = net::http;

http::Client::Errors::HttpMethodNotSupported::HttpMethodNotSupported(
        http::Method method,
        const core::Location& loc)
    : http::Error("Http method not suppored", loc),
      method(method)
{

}

std::shared_ptr<http::Request> http::Client::post_form(
        const http::Request::Configuration& configuration,
        const std::map<std::string, std::string>& values)
{
    std::stringstream ss;
    bool first{true};

    for (const auto& pair : values)
    {
        ss << (first ? "" : "&") << url_escape(pair.first) << "=" << url_escape(pair.second);
        first = false;
    }

    return post(configuration, ss.str(), http::ContentType::x_www_form_urlencoded);
}

std::string http::Client::uri_to_string(const core::net::Uri& uri) const
{
    std::ostringstream s;

    // Start with the host of the URI
    s << uri.host;

    // Append each of the components of the path
    for (const std::string& part : uri.path)
    {
        s << "/" << url_escape(part);
    }

    // Append the parameters
    bool first = true;
    for (const std::pair<std::string, std::string>& query_parameter : uri.query_parameters)
    {
        if (first)
        {
            // The first parameter needs a ?
            s << "?";
            first = false;
        }
        else
        {
            // The rest are separated with a &
            s << "&";
        }

        // URL escape the parameters
        s << url_escape(query_parameter.first) << "=" << url_escape(query_parameter.second);
    }

    // We're done
    return s.str();
}

//TODO: Keep abi compatibility in vivid/xenial. 
//Should be virtual function for the following two methods and move them to impl/curl/client.cpp.
std::shared_ptr<http::Request> http::Client::post(
        const http::Request::Configuration& configuration, std::istream& payload, 
        std::size_t size)
{
    auto *curl_client = dynamic_cast<http::impl::curl::Client*>(this);
    if (curl_client)
    {
        return curl_client->post(configuration, payload, size);
    }
    throw std::runtime_error("bad cast for curl client");
}

std::shared_ptr<http::Request> http::Client::del(
        const http::Request::Configuration& configuration)
{
    auto *curl_client = dynamic_cast<http::impl::curl::Client*>(this);
    if (curl_client)
    {
        return curl_client->del(configuration);
    }
    throw std::runtime_error("bad cast for curl client");
}
