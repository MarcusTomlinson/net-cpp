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
#ifndef CORE_NET_HTTP_REQUEST_H_
#define CORE_NET_HTTP_REQUEST_H_

#include <core/net/visibility.h>

#include <core/net/http/error.h>
#include <core/net/http/header.h>

#include <chrono>
#include <memory>

namespace core
{
namespace net
{
namespace http
{
class Response;

/**
 * @brief The Request class encapsulates a request for a web resource.
 */
class CORE_NET_DLL_PUBLIC Request
{
public:
    /**
     * @brief The State enum describes the different states a request can be in.
     */
    enum class State
    {
        ready, ///< The request is idle and needs execution.
        active, ///< The request is active and is actively being executed.
        done ///< Execution of the request has finished.
    };

    /**
     * @brief The Errors struct collects the Request-specific exceptions and error modes.
     */
    struct Errors
    {
        Errors() = delete;

        /**
         * @brief AlreadyActive is thrown when *execute is called on an active request.
         */
        struct AlreadyActive : public core::net::http::Error
        {
            /**
             * @brief AlreadyActive creates a new instance with a location hint.
             * @param loc The location that the call originates from.
             */
            AlreadyActive(const core::Location& loc);
        };
    };

    /**
     * @brief The Progress struct encapsulates progress information for web-resource requests.
     */
    struct Progress
    {
        /**
         * @brief The Next enum summarizes the available return-types for the progress callback.
         */
        enum class Next
        {
            continue_operation, ///< Continue the request.
            abort_operation ///< Abort the request.
        };

        struct
        {
            double total{-1.}; ///< Total number of bytes to be downloaded.
            double current{-1.}; ///< Current number of bytes already downloaded.
        } download{};

        struct
        {
            double total{-1.}; ///< Total number of bytes to be uploaded.
            double current{-1.}; ///< Current number of bytes already uploaded.
        } upload{};
    };

    /**
     * @brief ErrorHandler is invoked in case of errors arising while executing the request.
     */
    typedef std::function<void(const core::net::Error&)> ErrorHandler;

    /**
     * @brief ProgressHandler is invoked for progress updates while executing the request.
     */
    typedef std::function<Progress::Next(const Progress&)> ProgressHandler;

    /**
     * @brief ResponseHandler is invoked when a request completes.
     */
    typedef std::function<void(const Response&)> ResponseHandler;

    /**
     * @brief Encapsulates callbacks that can happen during request execution.
     */
    class Handler
    {
    public:
        Handler() = default;

        /** @brief Returns the currently set progress handler. */
        const ProgressHandler& on_progress() const;
        /** @brief Adjusts the currently set progress handler. */
        Handler& on_progress(const ProgressHandler& handler);

        /** @brief Returns the currently set response handler. */
        const ResponseHandler& on_response() const;
        /** @brief Adjusts the currently set response handler. */
        Handler& on_response(const ResponseHandler& handler);

        /** @brief Returns the currently set error handler. */
        const ErrorHandler& on_error() const;
        /** @brief Adjusts the currently set error handler. */
        Handler& on_error(const ErrorHandler& handler);

    private:
        /** @cond */
        ProgressHandler progress_handler{};
        ResponseHandler response_handler{};
        ErrorHandler error_handler{};
        /** @endcond */
    };

    /**
     * @brief The Credentials struct encapsulates username and password for basic & digest authentication.
     */
    struct Credentials
    {
        std::string username;
        std::string password;
    };

    /** Function signature for querying credentials for a given URL. */
    typedef std::function<Credentials(const std::string&)> AuthenicationHandler;

    /**
     * @brief The Configuration struct encapsulates all options for creating requests.
     */
    struct Configuration
    {
        /**
         * @brief from_uri_as_string creates a new instance of Configuration for a url.
         * @param uri The url of the web resource to issue a request for.
         * @return A new Configuration instance.
         */
        inline static Configuration from_uri_as_string(const std::string& uri)
        {
            Configuration result;
            result.uri = uri;

            return result;
        }

        /** Uri of the web resource to issue a request for. */
        std::string uri;

        /** Custom header fields that are added to the request. */
        Header header;

        /** Invoked to report progress. */
        ProgressHandler on_progress;

        /** Invoked to report a successfully finished request. */
        ResponseHandler on_response;

        /** Invoked to report a request that finished with an error. */
        ErrorHandler on_error;

        /** SSL-specific options. Please be very careful when adjusting these. */
        struct
        {
            /** Yes, we want to verify our peer by default. */
            bool verify_peer
            {
                true
            };

            /** Yes, we want to be strict and verify the host by default, too. */
            bool verify_host
            {
                true
            };
        } ssl;

        /** Encapsulates proxy and http authentication handlers. */
        struct
        {
            /** Invoked for querying user credentials to do basic/digest auth. */
            AuthenicationHandler for_http;
            /** Invoked for querying user credentials to authenticate proxy accesses. */
            AuthenicationHandler for_proxy;
        } authentication_handler;

        /** Encapsulates thresholds for minimum transfer speed in [kB/s] for duration seconds. */
        struct
        {
            std::uint64_t limit{1};
            std::chrono::seconds duration{std::chrono::seconds{30}};
        } speed;
    };

    Request(const Request&) = delete;
    virtual ~Request() = default;

    Request& operator=(const Request&) = delete;
    bool operator==(const Request&) const = delete;

    /**
     * @brief state queries the current state of the operation.
     * @return A value from the State enumeration.
     */
    virtual State state() = 0;

    /**
     * @brief Adjusts the timeout of a State::ready request.
     * @param timeout The timeout in milliseconds.
     */
    virtual void set_timeout(const std::chrono::milliseconds& timeout) = 0;

    /**
     * @brief Synchronously executes the request.
     * @throw core::net::http::Error in case of http-related errors.
     * @throw core::net::Error in case of network-related errors.
     * @return The response to the request.
     */
    virtual Response execute(const ProgressHandler& ph) = 0;

    /**
     * @brief Asynchronously executes the request, reporting errors, progress and completion to the given handlers.
     * @param handler The handlers to called for events happening during execution of the request.
     * @return The response to the request.
     */
    virtual void async_execute(const Handler& handler) = 0;

    /**
     * @brief Pause the request with options for aborting the request.
     * The request will be aborted if transfer speed falls below \a limit in [bytes/second] for \a time seconds.
     * @throw core::net::http::Error in case of http-related errors.
     */
    virtual void pause() = 0;

    /**
     * @brief Resume the request
     * @throw core::net::http::Error in case of http-related errors.
     */
    virtual void resume() = 0;

    /**
     * @brief Returns the input string in URL-escaped format.
     * @param s The string to be URL escaped.
     */
    virtual std::string url_escape(const std::string& s) = 0;

    /**
     * @brief Returns the input string in URL-unescaped format.
     * @param s The string to be URL unescaped.
     */
    virtual std::string url_unescape(const std::string& s) = 0;

protected:
    /** @cond */
    Request() = default;
    /** @endcond */
};
}
}
}

#endif // CORE_NET_HTTP_REQUEST_H_
