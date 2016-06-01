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

#include <core/net/error.h>
#include <core/net/uri.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/request.h>
#include <core/net/http/response.h>

#include "httpbin.h"

#include <gtest/gtest.h>

#include <json/json.h>

#include <future>
#include <fstream>

namespace http = core::net::http;
namespace json = Json;
namespace net = core::net;

namespace
{
auto default_progress_reporter = [](const http::Request::Progress& progress)
{
    if (progress.download.current > 0. && progress.download.total > 0.)
        std::cout << "Download progress: " << progress.download.current / progress.download.total << std::endl;
    if (progress.upload.current > 0. && progress.upload.total > 0.)
        std::cout << "Upload progress: " << progress.upload.current / progress.upload.total << std::endl;

    return http::Request::Progress::Next::continue_operation;
};

bool init()
{
    static httpbin::Instance instance;
    return true;
}

static const bool is_initialized = init();
}

TEST(HttpClient, uri_to_string)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    EXPECT_EQ("http://baz.com", client->uri_to_string(net::make_uri("http://baz.com")));

    EXPECT_EQ("http://foo.com/foo%20bar/baz%20boz",
              client->uri_to_string(net::make_uri("http://foo.com",
              { "foo bar", "baz boz" })));

    EXPECT_EQ(
            "http://banana.fruit/my/endpoint?hello%20there=good%20bye&happy=sad",
            client->uri_to_string(net::make_uri("http://banana.fruit",
            { "my", "endpoint" },
            { { "hello there", "good bye" }, { "happy", "sad" } })));
}

TEST(HttpClient, head_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->head(http::Request::Configuration::from_uri_as_string(url));

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
}

TEST(HttpClient, DISABLED_a_request_can_timeout)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->head(http::Request::Configuration::from_uri_as_string(url));
    request->set_timeout(std::chrono::milliseconds{1});

    // We finally execute the query synchronously and story the response.
    EXPECT_THROW(auto response = request->execute(default_progress_reporter), core::net::Error);
}

TEST(HttpClient, DISABLED_get_request_against_app_store_succeeds)
{
    auto client = http::make_client();

    auto url = "https://search.apps.ubuntu.com/api/v1/search?q=%2Cframework%3Aubuntu-sdk-13.10%2Carchitecture%3Aamd64";

    auto request = client->get(http::Request::Configuration::from_uri_as_string(url));

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);

    response.header.enumerate([](const std::string& key, const std::set<std::string>& values)
    {
        for (const auto& value : values)
            std::cout << key << " -> " <<  value << std::endl;
    });
}

TEST(HttpClient, get_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->get(http::Request::Configuration::from_uri_as_string(url));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(url, root["url"].asString());
}

TEST(HttpClient, get_request_with_custom_headers_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::headers();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.header.set("Test1", "42");
    configuration.header.set("Test2", "43");

    auto request = client->get(configuration);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);

    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));

    auto headers = root["headers"];

    EXPECT_EQ("42", headers["Test1"].asString());
    EXPECT_EQ("43", headers["Test2"].asString());
}

TEST(HttpClient, empty_header_values_are_handled_correctly)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::headers();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.header.set("Empty", std::string{});

    auto request = client->get(configuration);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);

    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));

    auto headers = root["headers"];
    EXPECT_EQ(std::string{}, headers["Empty"].asString());
}

TEST(HttpClient, get_request_for_existing_resource_guarded_by_basic_auth_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::basic_auth();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.authentication_handler.for_http = [](const std::string&)
    {
        return http::Request::Credentials{"user", "passwd"};
    };
    auto request = client->get(configuration);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // We expect authentication to work.
    EXPECT_TRUE(root["authenticated"].asBool());
    // With the correct user id
    EXPECT_EQ("user", root["user"].asString());
}

// Digest auth is broken on httpbin.org. It even fails in the browser after the first successful access.
TEST(HttpClient, DISABLED_get_request_for_existing_resource_guarded_by_digest_auth_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::digest_auth();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.authentication_handler.for_http = [](const std::string&)
    {
        return http::Request::Credentials{"user", "passwd"};
    };
    auto request = client->get(configuration);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // We expect authentication to work.
    EXPECT_TRUE(root["authenticated"].asBool());
    // With the correct user id
    EXPECT_EQ("user", root["user"].asString());
}

TEST(HttpClient, async_get_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Execute the client
    std::thread worker{[client]() { client->run(); }};

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->get(http::Request::Configuration::from_uri_as_string(url));

    std::promise<core::net::http::Response> promise;
    auto future = promise.get_future();

    // We finally execute the query asynchronously.
    request->async_execute(
                http::Request::Handler()
                    .on_progress(default_progress_reporter)
                    .on_response([&](const core::net::http::Response& response)
                    {
                        promise.set_value(response);
                    })
                    .on_error([&](const core::net::Error& e)
                    {
                        promise.set_exception(std::make_exception_ptr(e));
                    }));

    auto response = future.get();

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(url, root["url"].asString());

    client->stop();

    // We shut down our worker thread
    if (worker.joinable())
        worker.join();
}

TEST(HttpClient, async_get_request_for_existing_resource_guarded_by_basic_authentication_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Execute the client
    std::thread worker{[client]() { client->run(); }};

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::basic_auth();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);

    configuration.authentication_handler.for_http = [](const std::string&)
    {
        return http::Request::Credentials{"user", "passwd"};
    };

    auto request = client->get(configuration);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    std::promise<core::net::http::Response> promise;
    auto future = promise.get_future();

    // We finally execute the query asynchronously.
    request->async_execute(
                http::Request::Handler()
                    .on_progress(default_progress_reporter)
                    .on_response([&](const core::net::http::Response& response)
                    {
                        promise.set_value(response);
                        client->stop();
                    })
                    .on_error([&](const core::net::Error& e)
                    {
                        promise.set_exception(std::make_exception_ptr(e));
                        client->stop();
                    }));

    // And wait here for the response to arrive.
    auto response = future.get();

    // We shut down our worker thread
    if (worker.joinable())
        worker.join();

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // We expect authentication to work.
    EXPECT_TRUE(root["authenticated"].asBool());
    // With the correct user id
    EXPECT_EQ("user", root["user"].asString());
}

TEST(HttpClient, post_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::post();

    std::string payload = "{ 'test': 'test' }";

    // The client mostly acts as a factory for http requests.
    auto request = client->post(http::Request::Configuration::from_uri_as_string(url),
                                payload,
                                core::net::http::ContentType::json);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter);

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(payload, root["data"].asString());
}

TEST(HttpClient, post_form_request_for_existing_resource_succeeds)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::post();

    std::map<std::string, std::string> values
    {
        {"test", "test"}
    };

    // The client mostly acts as a factory for http requests.
    auto request = client->post_form(http::Request::Configuration::from_uri_as_string(url),
                                     values);

    // We finally execute the query synchronously and store the response.
    auto response = request->execute(default_progress_reporter);

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ("test", root["form"]["test"].asString());
}

TEST(HttpClient, post_request_for_file_with_large_chunk_succeeds)
{
    auto client = http::make_client();
    auto url = std::string(httpbin::host) + httpbin::resources::post();

    // create temp file with large chunk
    const std::size_t size = 1024*1024;
    std::ofstream ofs("tmp.dat", std::ios::binary | std::ios::out);
    ofs.seekp(size);
    ofs.write("", 1);
    ofs.close();

    std::ifstream payload("tmp.dat");
    auto request = client->post(http::Request::Configuration::from_uri_as_string(url),
                                payload,
                                size);

    json::Value root;
    json::Reader reader;

    auto response = request->execute(default_progress_reporter);

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(url, root["url"].asString());
}

TEST(HttpClient, put_request_for_existing_resource_succeeds)
{
    auto client = http::make_client();
    auto url = std::string(httpbin::host) + httpbin::resources::put();

    const std::string value{"{ 'test': 'test' }"};
    std::stringstream payload(value);

    auto request = client->put(http::Request::Configuration::from_uri_as_string(url),
                               payload,
                               value.size());

    json::Value root;
    json::Reader reader;

    auto response = request->execute(default_progress_reporter);

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(payload.str(), root["data"].asString());
}

TEST(HttpClient, put_request_for_file_with_large_chunk_succeeds)
{
    auto client = http::make_client();
    auto url = std::string(httpbin::host) + httpbin::resources::put();

    // create temp file with large chunk
    const std::size_t size = 1024*1024;
    std::ofstream ofs("tmp.dat", std::ios::binary | std::ios::out);
    ofs.seekp(size);
    ofs.write("", 1);
    ofs.close();

    std::ifstream payload("tmp.dat");
    auto request = client->put(http::Request::Configuration::from_uri_as_string(url),
                               payload,
                               size);

    json::Value root;
    json::Reader reader;

    auto response = request->execute(default_progress_reporter);

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(url, root["url"].asString());
}

TEST(HttpClient, del_request_for_existing_resource_succeeds)
{
    auto client = http::make_client();
    auto url = std::string(httpbin::host) + httpbin::resources::del();

    auto request = client->del(http::Request::Configuration::from_uri_as_string(url));

    json::Value root;
    json::Reader reader;

    auto response = request->execute(default_progress_reporter);

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(url, root["url"].asString());
}

namespace com
{
namespace mozilla
{
namespace services
{
namespace location
{
constexpr const char* host
{
    "https://location.services.mozilla.com"
};

namespace resources
{
namespace v1
{
const char* search() { return "/v1/search?key=net-cpp-testing"; }
const char* submit() { return "/v1/submit?key=net-cpp-testing"; }
}
}
}
}
}
}

// See https://mozilla-ichnaea.readthedocs.org/en/latest/api/search.html
// for API and endpoint documentation.
TEST(HttpClient, DISABLED_search_for_location_on_mozillas_location_service_succeeds)
{
    json::FastWriter writer;
    json::Value search;
    json::Value cell;
    cell["radio"] = "umts";
    cell["mcc"] = 123;
    cell["mnc"] = 123;
    cell["lac"] = 12345;
    cell["cid"] = 12345;
    cell["signal"] = -61;
    cell["asu"] = 26;
    json::Value wifi1, wifi2;
    wifi1["key"] = "01:23:45:67:89:ab";
    wifi1["channel"] = 11;
    wifi1["frequency"] = 2412;
    wifi1["signal"] = -50;
    wifi2["key"] = "01:23:45:67:cd:ef";

    search["radio"] = json::Value("gsm");
    search["cell"].append(cell);
    search["wifi"].append(wifi1);
    search["wifi"].append(wifi2);

    auto client = http::make_client();
    auto url =
            std::string(com::mozilla::services::location::host) +
            com::mozilla::services::location::resources::v1::search();
    auto request = client->post(http::Request::Configuration::from_uri_as_string(url),
                                writer.write(search),
                                http::ContentType::json);

    auto response = request->execute(default_progress_reporter);

    json::Reader reader;
    json::Value result;

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, result));

    // We cannot be sure that the server has got information for the given
    // cell and wifi ids. For that, we disable the test.
    EXPECT_EQ("ok", result["status"].asString());
    //EXPECT_DOUBLE_EQ(-22.7539192, result["lat"].asDouble());
    //EXPECT_DOUBLE_EQ(-43.4371081, result["lon"].asDouble());
}

// See https://mozilla-ichnaea.readthedocs.org/en/latest/api/submit.html
// for API and endpoint documentation.
TEST(HttpClient, DISABLED_submit_of_location_on_mozillas_location_service_succeeds)
{
    json::Value submit;
    json::Value cell;
    cell["radio"] = "umts";
    cell["mcc"] = 123;
    cell["mnc"] = 123;
    cell["lac"] = 12345;
    cell["cid"] = 12345;
    cell["signal"] = -60;
    json::Value wifi1, wifi2;
    wifi1["key"] = "01:23:45:67:89:ab";
    wifi1["channel"] = 11;
    wifi1["frequency"] = 2412;
    wifi1["signal"] = -50;
    wifi2["key"] = "01:23:45:67:cd:ef";

    json::Value item;
    item["lat"] = -22.7539192;
    item["lon"] = -43.4371081;
    item["time"] = "2012-03-15T11:12:13.456Z";
    item["accuracy"] = 10;
    item["altitude"] = 100;
    item["altitude_accuracy"] = 1;
    item["radio"] = "gsm";
    item["cell"].append(cell);
    item["wifi"].append(wifi1);
    item["wifi"].append(wifi2);

    submit["items"].append(item);

    json::FastWriter writer;
    auto client = http::make_client();
    auto url =
            std::string(com::mozilla::services::location::host) +
            com::mozilla::services::location::resources::v1::submit();
    auto request = client->post(http::Request::Configuration::from_uri_as_string(url),
                                writer.write(submit),
                                http::ContentType::json);
    auto response = request->execute(default_progress_reporter);

    EXPECT_EQ(http::Status::no_content,
              response.status);
}

typedef std::pair<std::string, std::string> StringPairTestParams;

class HttpClientBase64Test : public ::testing::TestWithParam<StringPairTestParams> {
};

TEST_P(HttpClientBase64Test, encoder)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Get our encoding parameters
    auto param = GetParam();

    // Try the base64 encode out
    EXPECT_EQ(param.second, client->base64_encode(param.first));
}

TEST_P(HttpClientBase64Test, decoder)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Get our encoding parameters
    auto param = GetParam();

    // Try the base64 decode out
    EXPECT_EQ(param.first, client->base64_decode(param.second));
}

INSTANTIATE_TEST_CASE_P(Base64Fixtures, HttpClientBase64Test,
        ::testing::Values(
                StringPairTestParams("", ""),
                StringPairTestParams("M", "TQ=="),
                StringPairTestParams("Ma", "TWE="),
                StringPairTestParams("Man", "TWFu"),
                StringPairTestParams("pleasure.", "cGxlYXN1cmUu"),
                StringPairTestParams("leasure.", "bGVhc3VyZS4="),
                StringPairTestParams("easure.", "ZWFzdXJlLg=="),
                StringPairTestParams("asure.", "YXN1cmUu"),
                StringPairTestParams("sure.", "c3VyZS4="),
                StringPairTestParams("bananas are tasty", "YmFuYW5hcyBhcmUgdGFzdHk=")
        ));

class HttpClientUrlEscapeTest : public ::testing::TestWithParam<StringPairTestParams> {
};

TEST_P(HttpClientUrlEscapeTest, url_escape)
{
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_client();

    // Get our encoding parameters
    auto param = GetParam();

    // Try the url_escape out
    EXPECT_EQ(param.second, client->url_escape(param.first));
}

INSTANTIATE_TEST_CASE_P(UrlEscapeFixtures, HttpClientUrlEscapeTest,
        ::testing::Values(
                StringPairTestParams("", ""),
                StringPairTestParams("Hello Günter", "Hello%20G%C3%BCnter"),
                StringPairTestParams("That costs £20", "That%20costs%20%C2%A320"),
                StringPairTestParams("Microsoft®", "Microsoft%C2%AE")
        ));
