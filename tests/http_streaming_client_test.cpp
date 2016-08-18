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
#include <core/net/http/streaming_client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/request.h>
#include <core/net/http/response.h>

#include "httpbin.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <future>
#include <memory>

#include <fstream>
#include <iomanip>
#include <iostream>

namespace http = core::net::http;
namespace json = Json;
namespace net = core::net;

namespace
{
class MockDataHandler : public std::enable_shared_from_this<MockDataHandler>
{
public:
    // We are enabling shared from this, thus forcing creation of
    // managed instances here.
    static std::shared_ptr<MockDataHandler> create()
    {
        return std::shared_ptr<MockDataHandler>(new MockDataHandler);
    }

    MOCK_METHOD1(on_new_data, void(const std::string&));

    http::StreamingRequest::DataHandler to_data_handler()
    {
        auto thiz = shared_from_this();
        return [thiz](const std::string& s)
        {
            thiz->on_new_data(s);
        };
    }

private:
    MockDataHandler() = default;
};

class ProgressBar
{
public:
    ProgressBar(std::uint32_t width)
        : width{width}, out{std::cout}
    {
    }

    ~ProgressBar()
    {
        out << std::endl;
    }

    void update(double percentage)
    {
        struct CursorState
        {
            CursorState(std::ostream& out) : out{out} { out << "\33[?25l"; }
            ~CursorState()                            { out << "\33[?25h"; }
            std::ostream& out;
        } cs{out};

        out << "\r" << "[" << std::setw(width) << std::left << std::setfill(' ') << std::string(percentage * width, '=') << "] " << std::setw(5) << std::fixed << std::setprecision(2) << percentage * 100 << " %" << std::flush;
    }

private:
    std::uint32_t width;
    std::ostream& out;
};

auto silent_progress_reporter = [](const http::Request::Progress&)
{
    return http::Request::Progress::Next::continue_operation;
};

auto default_progress_reporter = silent_progress_reporter;

bool init()
{
    static httpbin::Instance instance;
    return true;
}

static const bool is_initialized __attribute__((used)) = init();
}

TEST(StreamingStreamingHttpClient, head_request_for_existing_resource_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->streaming_head(http::Request::Configuration::from_uri_as_string(url));

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
}

TEST(StreamingHttpClient, get_request_for_existing_resource_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->streaming_get(http::Request::Configuration::from_uri_as_string(url));

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(url, root["url"].asString());
}

TEST(StreamingHttpClient, get_request_with_custom_headers_for_existing_resource_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::headers();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.header.set("Test1", "42");
    configuration.header.set("Test2", "43");

    auto request = client->streaming_get(configuration);

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);

    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));

    auto headers = root["headers"];

    EXPECT_EQ("42", headers["Test1"].asString());
    EXPECT_EQ("43", headers["Test2"].asString());
}

TEST(StreamingHttpClient, empty_header_values_are_handled_correctly)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::headers();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.header.set("Empty", std::string{});

    auto request = client->streaming_get(configuration);

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);

    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));

    auto headers = root["headers"];
    EXPECT_EQ(std::string{}, headers["Empty"].asString());
}

TEST(StreamingHttpClient, get_request_for_existing_resource_guarded_by_basic_auth_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::basic_auth();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.authentication_handler.for_http = [](const std::string&)
    {
        return http::Request::Credentials{"user", "passwd"};
    };
    auto request = client->streaming_get(configuration);

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

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
TEST(StreamingHttpClient, DISABLED_get_request_for_existing_resource_guarded_by_digest_auth_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::digest_auth();

    // The client mostly acts as a factory for http requests.
    auto configuration = http::Request::Configuration::from_uri_as_string(url);
    configuration.authentication_handler.for_http = [](const std::string&)
    {
        return http::Request::Credentials{"user", "passwd"};
    };
    auto request = client->streaming_get(configuration);

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // We expect authentication to work.
    EXPECT_TRUE(root["authenticated"].asBool());
    // With the correct user id
    EXPECT_EQ("user", root["user"].asString());
}

TEST(StreamingHttpClient, async_get_request_for_existing_resource_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Execute the client
    std::thread worker{[client]() { client->run(); }};

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::get();

    // The client mostly acts as a factory for http requests.
    auto request = client->streaming_get(http::Request::Configuration::from_uri_as_string(url));

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

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
                    }),
                dh->to_data_handler());

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

TEST(StreamingHttpClient, async_get_request_for_existing_resource_guarded_by_basic_authentication_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

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

    auto request = client->streaming_get(configuration);

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

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
                    }),
                dh->to_data_handler());

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

TEST(StreamingHttpClient, post_request_for_existing_resource_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::post();

    std::string payload = "{ 'test': 'test' }";

    // The client mostly acts as a factory for http requests.
    auto request = client->streaming_post(http::Request::Configuration::from_uri_as_string(url),
                                payload,
                                core::net::http::ContentType::json);

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    // We finally execute the query synchronously and story the response.
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    // We expect the query to complete successfully
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    // Parsing the body of the response as JSON should succeed.
    EXPECT_TRUE(reader.parse(response.body, root));
    // The url field of the payload should equal the original url we requested.
    EXPECT_EQ(payload, root["data"].asString());
}

TEST(StreamingHttpClient, post_form_request_for_existing_resource_succeeds)
{
    using namespace ::testing;

    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Url pointing to the resource we would like to access via http.
    auto url = std::string(httpbin::host) + httpbin::resources::post();

    std::map<std::string, std::string> values
    {
        {"test", "test"}
    };

    // The client mostly acts as a factory for http requests.
    auto request = client->streaming_post_form(http::Request::Configuration::from_uri_as_string(url),
                                               values);

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    // We finally execute the query synchronously and store the response.
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    // All endpoint data on httpbin.org is JSON encoded.
    json::Value root;
    json::Reader reader;

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ("test", root["form"]["test"].asString());
}

TEST(StreamingHttpClient, post_request_for_file_with_large_chunk_succeeds)
{
    using namespace ::testing;

    auto client = http::make_streaming_client();
    auto url = std::string(httpbin::host) + httpbin::resources::post();
  
    // create temp file with large chunk
    const std::size_t size = 1024*1024;
    std::ofstream ofs("tmp.dat", std::ios::binary | std::ios::out);
    ofs.seekp(size);
    ofs.write("", 1);
    ofs.close();
  
    std::ifstream payload("tmp.dat");
    auto request = client->streaming_post(http::Request::Configuration::from_uri_as_string(url),
                                payload,
                                size);
  
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));
  
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());
  
    json::Value root;
    json::Reader reader;

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(url, root["url"].asString());
}

TEST(StreamingHttpClient, post_request_for_file_with_large_chunk_with_read_callback)
{
    using namespace ::testing;

    auto client = http::make_streaming_client();
    auto url = std::string(httpbin::host) + httpbin::resources::post();
  
    // create temp file with large chunk
    const std::size_t size = 1024*1024;
    std::ofstream ofs("tmp.dat", std::ios::binary | std::ios::out);
    ofs.seekp(size);
    ofs.write("", 1);
    ofs.close();

    std::ifstream payload("tmp.dat");
    auto request = client->streaming_post(http::Request::Configuration::from_uri_as_string(url),
                                [&payload](void *dest, size_t buf_size) -> size_t {
                                    return payload.readsome(static_cast<char *>(dest), buf_size);
                                },
                                size);
  
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));
  
    auto response = request->execute(default_progress_reporter, dh->to_data_handler());
  
    json::Value root;
    json::Reader reader;

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(url, root["url"].asString());
}

TEST(StreamingHttpClient, put_request_for_existing_resource_succeeds)
{
    using namespace ::testing;

    auto client = http::make_streaming_client();
    auto url = std::string(httpbin::host) + httpbin::resources::put();

    const std::string value{"{ 'test': 'test' }"};
    std::stringstream payload(value);

    auto request = client->streaming_put(http::Request::Configuration::from_uri_as_string(url),
                               payload,
                               value.size());

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    json::Value root;
    json::Reader reader;

    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(payload.str(), root["data"].asString());
}

TEST(StreamingHttpClient, put_request_for_file_with_large_chunk_succeeds)
{
    using namespace ::testing;

    auto client = http::make_streaming_client();
    auto url = std::string(httpbin::host) + httpbin::resources::put();
  
    // create temp file with large chunk
    const std::size_t size = 1024*1024;
    std::ofstream ofs("tmp.dat", std::ios::binary | std::ios::out);
    ofs.seekp(size);
    ofs.write("", 1); 
    ofs.close();
  
    std::ifstream payload("tmp.dat");
    auto request = client->streaming_put(http::Request::Configuration::from_uri_as_string(url),
                               payload,
                               size);
  
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    auto response = request->execute(default_progress_reporter, dh->to_data_handler());
  
    json::Value root;
    json::Reader reader;
  
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(url, root["url"].asString());
}

TEST(StreamingHttpClient, put_request_for_file_with_large_chunk_with_read_callback)
{
    using namespace ::testing;

    auto client = http::make_streaming_client();
    auto url = std::string(httpbin::host) + httpbin::resources::put();
  
    // create temp file with large chunk
    const std::size_t size = 1024*1024;
    std::ofstream ofs("tmp.dat", std::ios::binary | std::ios::out);
    ofs.seekp(size);
    ofs.write("", 1); 
    ofs.close();
  
    std::ifstream payload("tmp.dat");
    auto request = client->streaming_put(http::Request::Configuration::from_uri_as_string(url),
                                [&payload](void *dest, size_t buf_size) -> size_t {
                                    return payload.readsome(static_cast<char *>(dest), buf_size);
                                },
                               size);
  
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    auto response = request->execute(default_progress_reporter, dh->to_data_handler());
  
    json::Value root;
    json::Reader reader;
  
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(url, root["url"].asString());
}

TEST(StreamingHttpClient, del_request_for_existing_resource_succeeds)
{
    using namespace ::testing;

    auto client = http::make_streaming_client();
    auto url = std::string(httpbin::host) + httpbin::resources::del();
  
    auto request = client->streaming_del(http::Request::Configuration::from_uri_as_string(url));
  
    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    auto response = request->execute(default_progress_reporter, dh->to_data_handler());

    json::Value root;
    json::Reader reader;
  
    EXPECT_EQ(core::net::http::Status::ok, response.status);
    EXPECT_TRUE(reader.parse(response.body, root));
    EXPECT_EQ(url, root["url"].asString());
}

TEST(StreamingHttpClient, request_can_be_paused_and_resumed)
{
    using namespace ::testing;
    // We obtain a default client instance, dispatching to the default implementation.
    auto client = http::make_streaming_client();

    // Execute the client
    std::thread worker{[client]() { client->run(); }};

    auto url = "http://archive.ubuntu.com/ubuntu/dists/xenial/main/installer-amd64/current/images/netboot/ubuntu-installer/amd64/linux";

    // The client mostly acts as a factory for http requests.
    auto request = client->streaming_get(http::Request::Configuration::from_uri_as_string(url));

    // Our mocked data handler.
    auto dh = MockDataHandler::create(); EXPECT_CALL(*dh, on_new_data(_)).Times(AtLeast(1));

    std::promise<core::net::http::Response> promise;
    auto future = promise.get_future();

    ProgressBar pb{80};

    //set abort option for the request.
    request->abort_request_if(1, std::chrono::seconds{30});
    // We finally execute the query asynchronously.
    request->async_execute(http::Request::Handler()
        .on_progress([&pb](const http::Request::Progress& progress)
        {
            if (progress.download.current > 0. && progress.download.total > 0.)
                pb.update(progress.download.current / static_cast<double>(progress.download.total));
            return http::Request::Progress::Next::continue_operation;
        })
        .on_response([&](const core::net::http::Response& response)
        {
            promise.set_value(response);
        })
        .on_error([&](const core::net::Error& e)
        {
            promise.set_exception(std::make_exception_ptr(e));
        }),
        dh->to_data_handler());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    request->pause();
    request->resume();

    try
    {
        // This might very well throw.
        EXPECT_EQ(core::net::http::Status::ok, future.get().status);
    } catch (const std::exception& e) { FAIL() << e.what(); }

    client->stop();

    // We shut down our worker thread
    if (worker.joinable())
        worker.join();    
}

