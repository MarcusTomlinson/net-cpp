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
#ifndef CORE_NET_HTTP_IMPL_CURL_EASY_H_
#define CORE_NET_HTTP_IMPL_CURL_EASY_H_

#include <core/net/http/client.h>
#include <core/net/http/header.h>
#include <core/net/http/method.h>
#include <core/net/http/status.h>

#include "shared.h"

#include <curl/curl.h>

#include <chrono>
#include <iosfwd>
#include <sstream>
#include <system_error>

namespace curl
{
typedef curl_slist StringList;

enum class Code
{
    ok = CURLE_OK,
    unsupported_protocol = CURLE_UNSUPPORTED_PROTOCOL,
    failed_init = CURLE_FAILED_INIT,
    url_malformat = CURLE_URL_MALFORMAT,
    not_built_in = CURLE_NOT_BUILT_IN,
    could_not_resolve_proxy = CURLE_COULDNT_RESOLVE_PROXY,
    could_not_resolve_host = CURLE_COULDNT_RESOLVE_HOST,
    could_not_connect = CURLE_COULDNT_CONNECT,
    remote_access_denied = CURLE_REMOTE_ACCESS_DENIED,
    quote_error = CURLE_QUOTE_ERROR,
    http_returned_error = CURLE_HTTP_RETURNED_ERROR,
    write_error = CURLE_WRITE_ERROR,
    upload_failed = CURLE_UPLOAD_FAILED,
    read_error = CURLE_READ_ERROR,
    out_of_memory = CURLE_OUT_OF_MEMORY,
    operation_timed_out = CURLE_OPERATION_TIMEDOUT,
    range_error = CURLE_RANGE_ERROR,
    http_post_error = CURLE_HTTP_POST_ERROR,
    ssl_connect_error = CURLE_SSL_CONNECT_ERROR,
    bad_download_resume = CURLE_BAD_DOWNLOAD_RESUME,
    function_not_found = CURLE_FUNCTION_NOT_FOUND,
    aborted_by_callback = CURLE_ABORTED_BY_CALLBACK,
    bad_function_argument = CURLE_BAD_FUNCTION_ARGUMENT,
    interface_failed = CURLE_INTERFACE_FAILED,
    too_many_redirects = CURLE_TOO_MANY_REDIRECTS,
    unknown_option = CURLE_UNKNOWN_OPTION,
    peer_failed_verification = CURLE_PEER_FAILED_VERIFICATION,
    ssl_engine_not_found = CURLE_SSL_ENGINE_NOTFOUND,
    ssl_engine_set_failed = CURLE_SSL_ENGINE_SETFAILED,
    send_error = CURLE_SEND_ERROR,
    receive_error = CURLE_RECV_ERROR,
    ssl_cert_problem = CURLE_SSL_CERTPROBLEM,
    ssl_cipher = CURLE_SSL_CIPHER,
    ssl_ca_cert = CURLE_SSL_CACERT,
    bad_content_encoding = CURLE_BAD_CONTENT_ENCODING,
    filesize_exceeded = CURLE_FILESIZE_EXCEEDED,
    send_fail_while_rewinding = CURLE_SEND_FAIL_REWIND,
    ssl_engine_init_failed = CURLE_SSL_ENGINE_INITFAILED,
    login_denied = CURLE_LOGIN_DENIED,
    remote_disk_full = CURLE_REMOTE_DISK_FULL,
    remote_file_exists = CURLE_REMOTE_FILE_EXISTS,
    conversion_failed = CURLE_CONV_FAILED,
    caller_must_register_conversion = CURLE_CONV_REQD,
    ssl_cacert_bad_file = CURLE_SSL_CACERT_BADFILE,
    remote_file_not_found = CURLE_REMOTE_FILE_NOT_FOUND,
    ssl_shutdown_failed = CURLE_SSL_SHUTDOWN_FAILED,
    again = CURLE_AGAIN,
    ssl_crl_bad_file = CURLE_SSL_CRL_BADFILE,
    ssl_issuer_error = CURLE_SSL_ISSUER_ERROR,
    chunk_failed = CURLE_CHUNK_FAILED,
    no_connection_available = CURLE_NO_CONNECTION_AVAILABLE
};

std::ostream& operator<<(std::ostream& out, Code code);

enum class Info
{
    response_code = CURLINFO_RESPONSE_CODE,
    namelookup_time = CURLINFO_NAMELOOKUP_TIME,
    connect_time = CURLINFO_CONNECT_TIME,
    appconnect_time = CURLINFO_APPCONNECT_TIME,
    pretransfer_time = CURLINFO_PRETRANSFER_TIME,
    starttransfer_time = CURLINFO_STARTTRANSFER_TIME,
    total_time = CURLINFO_TOTAL_TIME
};

enum class Option
{
    error_buffer = CURLOPT_ERRORBUFFER,
    cache_dns_timeout = CURLOPT_DNS_CACHE_TIMEOUT,
    header_function = CURLOPT_HEADERFUNCTION,
    header_data = CURLOPT_HEADERDATA,
    progress_function = CURLOPT_PROGRESSFUNCTION,
    progress_data = CURLOPT_PROGRESSDATA,
    no_progress = CURLOPT_NOPROGRESS,
    write_function = CURLOPT_WRITEFUNCTION,
    write_data = CURLOPT_WRITEDATA,
    read_function = CURLOPT_READFUNCTION,
    read_data = CURLOPT_READDATA,
    url = CURLOPT_URL,
    user_agent = CURLOPT_USERAGENT,
    http_header = CURLOPT_HTTPHEADER,
    http_auth = CURLOPT_HTTPAUTH,
    http_get = CURLOPT_HTTPGET,
    http_post = CURLOPT_POST,
    http_put = CURLOPT_PUT,
    copy_postfields = CURLOPT_COPYPOSTFIELDS,
    post_field_size = CURLOPT_POSTFIELDSIZE,
    upload = CURLOPT_UPLOAD,
    in_file_size = CURLOPT_INFILESIZE,
    sharing = CURLOPT_SHARE,
    username = CURLOPT_USERNAME,
    password = CURLOPT_PASSWORD,
    no_signal = CURLOPT_NOSIGNAL,
    verbose = CURLOPT_VERBOSE,
    timeout_ms = CURLOPT_TIMEOUT_MS,
    ssl_engine_default = CURLOPT_SSLENGINE_DEFAULT,
    ssl_verify_peer = CURLOPT_SSL_VERIFYPEER,
    ssl_verify_host = CURLOPT_SSL_VERIFYHOST,
    customrequest = CURLOPT_CUSTOMREQUEST,
    low_speed_limit = CURLOPT_LOW_SPEED_LIMIT,
    low_speed_time = CURLOPT_LOW_SPEED_TIME
};

namespace native
{
// Global setup of curl.
Code init();
// Cleanup all native curl resources.
void cleanup();
// URL escapes the given input string.
std::string escape(const std::string& in);
// Append a string to a string list
StringList* append_string_to_list(StringList* in, const char* string);
// Frees the overall string list
void free_string_list(StringList* in);
}

namespace easy
{
// Constant for disabling a feature on a curl easy instance.
constexpr static const long disable = 0;

// Constant for enabling a feature on a curl easy instance.
constexpr static const long enable = 1;

// Constant for enabling automatic SSL host verification.
constexpr static const long enable_ssl_host_verification = 2;

// Returns a human-readable description of the error code.
std::string print_error(Code code);

// Throws a std::runtime_error if the parameter to the function does match the
// constant templated value.
template<Code ref>
inline void throw_if(Code code, const std::function<std::string()>& descriptor = std::function<std::string()>())
{
    if (code == ref)
        throw std::runtime_error(print_error(code) + (descriptor ? ": " + descriptor() : ""));
}

// Throws a std::runtime_error if the parameter to the function does not match the
// constant templated value.
template<Code ref>
inline void throw_if_not(Code code, const std::function<std::string()>& descriptor = std::function<std::string()>())
{
    if (code != ref)
        throw std::runtime_error(print_error(code) + (descriptor ? ": " + descriptor() : ""));
}

// All curl native types and functions go here.
namespace native
{
// Global init/cleanup
namespace global
{
// Globally initializes curl.
Code init();
// Globally cleans up everything curl.
void cleanup();
}

// An opaque handle to a curl easy instance.
typedef CURL* Handle;

// Creates and initializes a new native easy instance.
Handle init();

// Releases and cleans up the resources of a native easy instance.
void cleanup(Handle handle);

// Executes the operation configured on the handle.
::curl::Code perform(Handle handle);

// Executes pause operation on the handle.
::curl::Code pause(Handle handle, int bitmask);

// URL escapes the given input string.
std::string escape(Handle handle, const std::string& in);

// URL unescapes the given input string.
std::string unescape(Handle handle, const std::string& in);

// Sets an option on a native curl multi instance.
template<typename T>
inline Code set(Handle handle, Option option, T value)
{
    return static_cast<Code>(curl_easy_setopt(handle, static_cast<CURLoption>(option), value));
}

// Reads information off a native curl multi instance.
template<typename T>
inline Code get(Handle handle, Info info, T value)
{
    return static_cast<Code>(curl_easy_getinfo(handle, static_cast<CURLINFO>(info), value));
}
}

class Handle
{
public:
    struct HandleHasBeenAbandoned : public std::runtime_error
    {
        HandleHasBeenAbandoned();
    };

    struct Timings
    {
        typedef std::chrono::duration<double> Seconds;

        // Time it took from the start until the name resolving was completed.
        Seconds name_look_up{Seconds::max()};
        // Time it took from the finished name lookup until the connect to the remote host (or proxy) was completed.
        Seconds connect{Seconds::max()};
        // Time it took from the connect until the SSL/SSH connect/handshake to the remote host was completed.
        Seconds app_connect{Seconds::max()};
        // Time it took from app_connect until the file transfer is just about to begin.
        Seconds pre_transfer{Seconds::max()};
        // Time it took from pre-transfer until the first byte is received by libcurl.
        Seconds start_transfer{Seconds::max()};
        // Time in total that the previous transfer took.
        Seconds total{Seconds::max()};
    };

    // Function type that gets called to indicate that an operation finished.
    typedef std::function<void(curl::Code)> OnFinished;
    // Function type that gets called to report progress of an operation.
    typedef std::function<int(void*, double, double, double, double)> OnProgress;
    // Function type that gets called whenever data should be read.
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnReadData;
    // Function type that gets called whenever body/payload data should be written.
    typedef std::function<std::size_t(char*, std::size_t, std::size_t)> OnWriteData;
    // Function type that gets called whenever header data should be written.
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnWriteHeader;

    // Creates a new handle and initializes the underlying curl easy instance.
    Handle();

    // Releases the handle and all underlying state.
    // Subsequent accesses to this instance will throw a
    // HandleHasBeenAbandoned exception.
    void release();

    // Queries the timing information of the last execution from the native curl handle.
    Timings timings();

    // Queries information from the instance.
    template<typename T, typename U>
    inline void get_option(T option, U value)
    {
        throw_if_not<Code::ok>(native::get(native(), option, value));
    }

    // Sets an option on the instance.
    template<typename T>
    inline void set_option(Option option, T value)
    {
        switch (option)
        {
        case Option::ssl_engine_default:
            throw_if_not<Code::ok>(native::set(native(), option, value), []()
            {
                return "We failed to setup the default SSL engine for CURL.\n"
                       "This likely hints towards a broken CURL implementation.\n"
                       "Please make sure that all the build-dependencies of the software are installed.\n";
            });
            break;
        default:
            throw_if_not<Code::ok>(native::set(native(), option, value), [this]()
            {
                return error();
            });
            break;
        }
    }

    // Adjusts the url that the instance should download.
    Handle& url(const char* url);
    // Adjusts the user agent that is passed in the request.
    Handle& user_agent(const char* user_agent);
    // Adjusts the credentials of the request.
    Handle& http_credentials(const std::string& username, const std::string& pwd);
    // Sets the OnFinished handler.
    Handle& on_finished(const OnFinished& on_finished);
    // Sets the OnProgress handler.
    Handle& on_progress(const OnProgress& on_progress);
    // Sets the OnReadData handler.
    Handle& on_read_data(const OnReadData& on_read_data, std::size_t size);
    // Sets the OnWriteData handler.
    Handle& on_write_data(const OnWriteData& on_new_data);
    // Sets the OnWriteHeader handler.
    Handle& on_write_header(const OnWriteHeader& on_new_header);
    // Sets the http method used by this instance.
    Handle& method(core::net::http::Method method);
    // Sets the data to be posted by this instance.
    Handle& post_data(const std::string& data, const std::string&);
    // Sets custom request headers
    Handle& header(const core::net::http::Header& header);

    // Queries the current status of this instance.
    core::net::http::Status status();
    // Queries the native curl easy handle.
    native::Handle native() const;

    // Executes the operation associated with this handle.
    void perform();

    // Executes pause operation associated with this handle.
    void pause();

    // Executes resume operation associated with this handle.
    void resume();

    // URL escapes the given input string.
    std::string escape(const std::string& in);

    // URL unescapes the given input string.
    std::string unescape(const std::string& in);

    // Notifies this instance that the operation finished with 'code'.
    void notify_finished(curl::Code code);

private:
    static int progress_cb(void* data, double dltotal, double dlnow, double ultotal, double ulnow);
    static std::size_t read_data_cb(void* data, std::size_t size, std::size_t nmemb, void *cookie);
    static std::size_t write_data_cb(char* data, size_t size, size_t nmemb, void* cookie);
    static std::size_t write_header_cb(void* data, size_t size, size_t nmemb, void* cookie);

    // Returns the current error description.
    std::string error() const;

    struct Private;
    std::shared_ptr<Private> d;
};
}
}
#endif // CORE_NET_HTTP_IMPL_CURL_EASY_H_
