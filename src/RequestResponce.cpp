#include "RequestResponce.hpp"
#include <boost/asio.hpp>
#include "api/HttpReadBodyResult.hpp"
#include "api/HttpRequest.hpp"
#include <openssl/ssl.h>

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
	boost::asio::ssl::context& ssl_ctx,
	const std::shared_ptr<ledger::core::api::HttpRequest> &request)
	: resolver_(io_context)
	, socket_(io_context)
	, stream_(io_context, ssl_ctx)
	, _uri(request->getUrl())
	, apiRequest_(request)
{
	urlConnection = std::make_shared<HttpUrlConnection>();

	// Set SNI Hostname (many hosts need this to handshake successfully)
	if (!SSL_set_tlsext_host_name(stream_.native_handle(), _uri.authority().to_string().c_str()))
	{
		boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
		std::cerr << ec.message() << "\n";
		return;
	}

	// Set up an HTTP GET request message
	req_.version(11);
	req_.method(http::verb::get);
	std::string target = _uri.path().to_string();
	if (!_uri.query().empty())
		target += "?" + _uri.query().to_string();
	if (!_uri.fragment().empty())
		target += "#" + _uri.fragment().to_string();
	req_.target(target);
	req_.set(http::field::host, _uri.authority().to_string());
	req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	for (auto& x : request->getHeaders()) {
		req_.set(x.first, x.second);
	}
}

void RequestResponse::execute() {
	std::string authority = _uri.host().to_string();
	std::string port = _uri.port().to_string();
	if (port.empty()) {
		port = _uri.scheme().to_string();
	}
    resolver_.async_resolve(
        authority,
		port,
        [self = shared_from_this()](const system::error_code& err, const ip::tcp::resolver::results_type& endpoints) {
        self->handle_resolve(err, endpoints);
    });
}

void RequestResponse::handle_resolve(const system::error_code& err,
    const ip::tcp::resolver::results_type& endpoints)
{
    if (!err) {
        // Attempt a connection to each endpoint in the list until we
        // successfully establish a connection.
		if (_uri.scheme().to_string() == "https") {
			async_connect(
				stream_.next_layer(),
				endpoints,
				[self = shared_from_this()](const system::error_code& err, const tcp::endpoint& endpoint)
				{
					self->ssl_handle_connect(err);
				}
			);
		}
		else {
			async_connect(socket_, endpoints,
				[self = shared_from_this()](const system::error_code& err, const tcp::endpoint& endpoint)
			{
				self->handle_connect(err);
			});
		}
    }
    else {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::UNABLE_TO_CONNECT_TO_HOST, err.message()));
    }
}

void RequestResponse::ssl_handle_connect(const system::error_code& err)
{
	if (!err)
	{
		// The connection was successful. Send the request.
		stream_.async_handshake(boost::asio::ssl::stream_base::client,
			[self = shared_from_this()](const system::error_code& err) {self->ssl_handle_handshake(err); });
	}
	else
	{
		apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::UNABLE_TO_CONNECT_TO_HOST, err.message()));
	}
}

void RequestResponse::ssl_handle_handshake(const boost::system::error_code& err)
{
	if (err) {
		apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::UNABLE_TO_CONNECT_TO_HOST, err.message()));
		return;
	}

	// Send the HTTP request to the remote host
	http::async_write(
		stream_,
		req_,
		[self = shared_from_this()](const system::error_code& err,
			std::size_t bytes_transferred) {self->ssl_handle_write_request(err); });
}

void RequestResponse::ssl_handle_write_request(const system::error_code& err)
{
	if (!err) {
		// Read the response status line. The response_ streambuf will
		// automatically grow to accommodate the entire line. The growth may be
		// limited by passing a maximum size to the streambuf constructor.
		http::async_read(stream_, buffer_, res_,
			[self = shared_from_this()](const system::error_code& err,
				std::size_t bytes_transferred) {
			//std::cout << self->req_.begin() << std::endl;
			self->ssl_handle_read_response(err); 
		});

	}
	else {
		apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
	}
}


void RequestResponse::ssl_handle_read_response(const system::error_code& err)
{
	if (err) {
		apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
		return;
	}

	urlConnection->statusCode = res_.result_int();
	urlConnection->body = res_.body();
	apiRequest_->complete(urlConnection, std::experimental::optional<api::Error>());

	stream_.async_shutdown(
		[self = shared_from_this()](const system::error_code& err) {
			self->ssl_handle_shutdown(err);
	});
}

void RequestResponse::ssl_handle_shutdown(const system::error_code& err) {
	//if (err)
		//apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
}

void RequestResponse::handle_connect(const system::error_code& err)
{
    if (!err)
    {
        // The connection was successful. Send the request.
        http::async_write(socket_, req_,
            [self = shared_from_this()](const system::error_code& err,
                std::size_t bytes_transferred) {self->handle_write_request(err); });
    }
    else
    {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::UNABLE_TO_CONNECT_TO_HOST, err.message()));
    }
}

void RequestResponse::handle_write_request(const system::error_code& err)
{
    if (!err) {
        // Read the response status line. The response_ streambuf will
        // automatically grow to accommodate the entire line. The growth may be
        // limited by passing a maximum size to the streambuf constructor.
		http::async_read(socket_, buffer_, res_,
                    [self=shared_from_this()](const system::error_code& err,
                std::size_t bytes_transferred) {self->handle_read_response(err); });

    }
    else {
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
    }
}


void RequestResponse::handle_read_response(const system::error_code& err)
{
	if (err) {
		apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
		return;
	}

	urlConnection->statusCode = res_.result_int();
	urlConnection->body = res_.body();
	apiRequest_->complete(urlConnection, std::experimental::optional<api::Error>());

	// Gracefully close the socket
	system::error_code ec;
	socket_.shutdown(tcp::socket::shutdown_both, ec);
	if ( ec && ec != boost::system::errc::not_connected)
        apiRequest_->complete(urlConnection, api::Error(api::ErrorCode::NO_INTERNET_CONNECTIVITY, err.message()));
}
