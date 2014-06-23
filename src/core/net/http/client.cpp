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

#include <core/net/uri.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#include <sstream>

namespace net = core::net;
namespace http = net::http;
namespace bai = boost::archive::iterators;

namespace
{
static const std::string BASE64_PADDING[] = { "", "==", "=" };
}

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

std::string http::Client::base64_encode(const std::string& s) const
{
    std::stringstream os;

    // convert binary values to base64 characters
    typedef bai::base64_from_binary
    // retrieve 6 bit integers from a sequence of 8 bit bytes
    <bai::transform_width<const char *, 6, 8> > base64_enc;

    std::copy(base64_enc(s.c_str()), base64_enc(s.c_str() + s.size()),
            std::ostream_iterator<char>(os));

    os << BASE64_PADDING[s.size() % 3];

    return os.str();
}

std::string http::Client::base64_decode(const std::string& s) const
{
    std::stringstream os;

    typedef bai::transform_width<bai::binary_from_base64<const char *>, 8, 6> base64_dec;

    unsigned int size = s.size();

    // Remove the padding characters
    // See: https://svn.boost.org/trac/boost/ticket/5629
    if (size && s[size - 1] == '=')
    {
        --size;
        if (size && s[size - 1] == '=')
        {
            --size;
        }
    }
    if (size == 0)
    {
        return std::string();
    }

    std::copy(base64_dec(s.data()), base64_dec(s.data() + size),
            std::ostream_iterator<char>(os));

    return os.str();
}
