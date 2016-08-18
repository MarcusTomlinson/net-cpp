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

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/request.h>
#include <core/net/http/response.h>

#include "httpbin.h"
#include "table.h"

#include <gtest/gtest.h>

#include <json/json.h>

#include <cmath>

#include <future>

namespace http = core::net::http;
namespace json = Json;
namespace net = core::net;

namespace
{
struct HttpClientLoadTest : public ::testing::Test
{
    typedef std::function<std::shared_ptr<http::Request>(const std::shared_ptr<http::Client>&)> RequestFactory;
    typedef std::function<bool (const http::Response)> ResponseVerifier;

    void run(const RequestFactory& request_factory, const ResponseVerifier& response_verifier)
    {
        // We obtain a default client instance, dispatching to the default implementation.
        auto client = http::make_client();

        // Execute the client
        std::thread worker{[client]() { client->run(); }};

        // Url pointing to the resource we would like to access via http.
        auto url = std::string(httpbin::host) + httpbin::resources::get();

        std::size_t completed{1};
        std::size_t total{200};

        auto on_completed = [&completed, total, client]()
        {
            if (++completed == total)
            {
                auto timings = client->timings();

                testing::Table::Row<15, '|'> row;
                testing::Table::Row<15, '|'>::HorizontalSeparator<5> sep;

                std::cout << sep;
                std::cout << (row << "Indicator" << "Min [s]" << "Max [s]" << "Mean [s]" << "Std. Dev. [s]");
                std::cout << sep;
                std::cout << (row << "NameLookup" << timings.name_look_up.min.count() << timings.name_look_up.max.count() << timings.name_look_up.mean.count() << std::sqrt(timings.name_look_up.variance.count()));
                std::cout << (row << "Connect" << timings.connect.min.count() << timings.connect.max.count() << timings.connect.mean.count() << std::sqrt(timings.connect.variance.count()));
                std::cout << (row << "AppConnect" << timings.app_connect.min.count() << timings.app_connect.max.count() << timings.app_connect.mean.count() << std::sqrt(timings.app_connect.variance.count()));
                std::cout << (row << "PreTransfer" << timings.pre_transfer.min.count() << timings.pre_transfer.max.count() << timings.pre_transfer.mean.count() << std::sqrt(timings.pre_transfer.variance.count()));
                std::cout << (row << "StartTransfer" << timings.start_transfer.min.count() << timings.start_transfer.max.count() << timings.start_transfer.mean.count() << std::sqrt(timings.start_transfer.variance.count()));
                std::cout << (row << "Total" << timings.total.min.count() << timings.total.max.count() << timings.total.mean.count() << std::sqrt(timings.total.variance.count()));
                std::cout << sep;

                client->stop();
            }
        };

        for (unsigned int i = 0; i < total; i++)
        {
            // The client mostly acts as a factory for http requests.
            auto request = request_factory(client);

            //auto once = client->request_store().add(request);

            // We finally execute the query asynchronously.
            request->async_execute(
                        http::Request::Handler()
                        .on_response([on_completed, response_verifier](const core::net::http::Response& response) mutable
                        {
                            EXPECT_TRUE(response_verifier(response));
                            on_completed();
                            //once();
                        })
                        .on_error([on_completed](const core::net::Error&) mutable
                        {
                            on_completed();
                            //once();
                        }));
        }

        // We shut down our worker threads
        if (worker.joinable())
            worker.join();
    }
};

bool init()
{
    static httpbin::Instance instance;
    return true;
}

static const bool is_initialized __attribute__((used)) = init();
}

TEST_F(HttpClientLoadTest, async_head_request_for_existing_resource_succeeds)
{
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    auto request_factory = [url](const std::shared_ptr<http::Client>& client)
    {
        return client->head(
                    http::Request::Configuration::from_uri_as_string(url));
    };

    auto response_verifier = [](const http::Response& response) -> bool
    {
        // All endpoint data on httpbin.org is JSON encoded.
        json::Value root;
        json::Reader reader;

        // We expect the query to complete successfully
        EXPECT_EQ(core::net::http::Status::ok, response.status);

        return not ::testing::Test::HasFailure();
    };

    run(request_factory, response_verifier);
}

TEST_F(HttpClientLoadTest, async_get_request_for_existing_resource_succeeds)
{
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    auto request_factory = [url](const std::shared_ptr<http::Client>& client)
    {
        return client->get(
                    http::Request::Configuration::from_uri_as_string(url));
    };

    auto response_verifier = [url](const http::Response& response) -> bool
    {
        // All endpoint data on httpbin.org is JSON encoded.
        json::Value root;
        json::Reader reader;

        // We expect the query to complete successfully
        EXPECT_EQ(core::net::http::Status::ok, response.status);
        // Parsing the body of the response as JSON should succeed.
        EXPECT_TRUE(reader.parse(response.body, root));
        // The url field of the payload should equal the original url we requested.
        EXPECT_EQ(url, root["url"].asString());

        return not ::testing::Test::HasFailure();
    };

    run(request_factory, response_verifier);
}

TEST_F(HttpClientLoadTest, async_post_request_for_existing_resource_succeeds)
{
    auto url = std::string(httpbin::host) + httpbin::resources::post();
    auto payload = "{ 'test': 'test' }";

    auto request_factory = [url, payload](const std::shared_ptr<http::Client>& client)
    {
        return client->post(
                    http::Request::Configuration::from_uri_as_string(url),
                    payload,
                    core::net::http::ContentType::json);
    };

    auto response_verifier = [payload](const http::Response& response) -> bool
    {
        // All endpoint data on httpbin.org is JSON encoded.
        json::Value root;
        json::Reader reader;

        // We expect the query to complete successfully
        EXPECT_EQ(core::net::http::Status::ok, response.status);
        // Parsing the body of the response as JSON should succeed.
        EXPECT_TRUE(reader.parse(response.body, root));
        // The url field of the payload should equal the original url we requested.
        EXPECT_EQ(payload, root["data"].asString());

        return not ::testing::Test::HasFailure();
    };

    run(request_factory, response_verifier);
}
