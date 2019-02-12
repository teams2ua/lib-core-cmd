#include "RequestResponce.hpp"
#include <boost/asio.hpp>
#include "api/HttpReadBodyResult.hpp"
#include "api/HttpRequest.hpp"

using namespace boost;
using namespace boost::asio;
using namespace std;
using namespace ledger::core;

int32_t HttpUrlConnection::getStatusCode() {
    return statusCode;
}

std::string HttpUrlConnection::getStatusText() {
    return statusText;
}

std::unordered_map<std::string, std::string> HttpUrlConnection::getHeaders() {
    return headers;
}

ledger::core::api::HttpReadBodyResult HttpUrlConnection::readBody() {
    auto b = body;
    body = std::vector<uint8_t>();
    return api::HttpReadBodyResult(std::experimental::optional<ledger::core::api::Error>(), b);
}

RequestResponse::RequestResponse(
    io_context& io_context,
    const std::string& url,
    const std::shared_ptr<ledger::core::api::HttpRequest> &request)
    : resolver_(io_context)
    , socket_(io_context)
    , _uri(url)
    , apiRequest_(request)
{
    urlConnection = std::make_shared<HttpUrlConnection>();
    std::ostream request_stream(&request_);
    request_stream << "GET " << url << " HTTP/1.0\r\n";
    request_stream << "Host: " << _uri.authority().to_string() << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";
}

void RequestResponse::execute() {
        
    resolver_.async_resolve(
        _uri.authority().to_string(),
        "http",
        [this](const error_code& err, const ip::tcp::resolver::results_type& endpoints) {
        handle_resolve(err, endpoints);
    });
}

void RequestResponse::handle_resolve(const error_code& err,
    const ip::tcp::resolver::results_type& endpoints)
{
    if (!err)
    {
        // Attempt a connection to each endpoint in the list until we
        // successfully establish a connection.

        async_connect(socket_, endpoints,
            [this](const error_code& err, const tcp::endpoint& endpoint) 
        {
            handle_connect(err); 
        });
    }
    else
    {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::UNABLE_TO_CONNECT_TO_HOST, err.message()));
    }
}

void RequestResponse::handle_connect(const asio::error_code& err)
{
    if (!err)
    {
        // The connection was successful. Send the request.
        asio::async_write(socket_, request_,
            [this](const asio::error_code& err,
                std::size_t bytes_transferred) {handle_write_request(err); });
    }
    else
    {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::UNABLE_TO_CONNECT_TO_HOST, err.message()));
    }
}

void RequestResponse::handle_write_request(const asio::error_code& err)
{
    if (!err)
    {
        // Read the response status line. The response_ streambuf will
        // automatically grow to accommodate the entire line. The growth may be
        // limited by passing a maximum size to the streambuf constructor.

        asio::async_read_until(socket_, response_, "\r\n",
            [this](const asio::error_code& err,
                std::size_t bytes_transferred) {handle_read_status_line(err); });

    }
    else
    {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
    }
}

void RequestResponse::handle_read_status_line(const asio::error_code& err)
{
    if (!err)
    {
        // Check that response is OK.
        std::istream response_stream(&response_);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::HTTP_ERROR, "Server send not a HTTP response"));
            return;
        }
        urlConnection->statusCode = status_code;
        urlConnection->statusText = status_message;
        if (status_code != 200)
        {
            apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::HTTP_ERROR, status_message));
            return;
        }

        // Read the response headers, which are terminated by a blank line.

        asio::async_read_until(socket_, response_, "\r\n\r\n",
            [this](const asio::error_code& err,
                std::size_t bytes_transferred) { handle_read_headers(err); });
    }
    else
    {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
    }
}

void RequestResponse::handle_read_headers(const asio::error_code& err)
{
    if (!err)
    {
        // Process the response headers.
        std::istream response_stream(&response_);
        std::string header;
        while (std::getline(response_stream, header) && header != "\r")
        {
            //TODO: fix this, make a split at :
            urlConnection->headers[header] = header;
        }

        // Start reading remaining data until EOF.
            
        asio::async_read(socket_, response_,
        asio::transfer_at_least(1),
            [this](const asio::error_code& err,
                std::size_t bytes_transferred) {handle_read_content(err); });

    }
    else
    {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
    }
}

void RequestResponse::handle_read_content(const asio::error_code& err)
{
    if (!err)
    {
        // Continue reading remaining data until EOF.
        asio::async_read(
            socket_, 
            response_,
            asio::transfer_at_least(1),
            [this](const asio::error_code& err,
                std::size_t bytes_transferred) {handle_read_content(err); }
        );

    }
    else if (err == asio::error::eof)
    {
        urlConnection->body = std::vector<uint8_t>{ asio::buffers_begin(response_.data()), asio::buffers_end(response_.data()) };
        apiRequest_->complete(urlConnection, std::experimental::optional<api::Error>());
    }
    else
    {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
    }
}
