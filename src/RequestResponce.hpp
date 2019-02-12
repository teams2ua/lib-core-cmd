#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system/error_code.hpp>
#include "api/HttpUrlConnection.hpp"

namespace ledger {
    namespace core {
        namespace api {
            class HttpRequest;
        }
    }
}

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

struct HttpUrlConnection : public ledger::core::api::HttpUrlConnection {
    int32_t statusCode;
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::vector<uint8_t> body;

    int32_t getStatusCode() override;
    std::string getStatusText() override;
    std::unordered_map<std::string, std::string> getHeaders() override;
    ledger::core::api::HttpReadBodyResult readBody() override;
};


class RequestResponse : public std::enable_shared_from_this<RequestResponse> {
public:
    RequestResponse(
        boost::asio::io_context& io_context,
        const std::string& url,
        const std::shared_ptr<ledger::core::api::HttpRequest>& request);
    void execute();
private:
    void handle_resolve(const boost::system::error_code& err, const boost::asio::ip::tcp::resolver::results_type& endpoints);
    void handle_connect(const boost::system::error_code& err);
    void handle_write_request(const boost::system::error_code& err);
    void handle_read_status_line(const boost::system::error_code& err);
    void handle_read_headers(const boost::system::error_code& err);
    void handle_read_content(const boost::system::error_code& err);
private:
	tcp::resolver resolver_;
	tcp::socket socket_;
	boost::beast::flat_buffer buffer_; // (Must persist between reads)
	http::request<http::empty_body> req_;
	http::response<http::string_body> res_;
	std::shared_ptr<ledger::core::api::HttpRequest> apiRequest_;
    std::shared_ptr<HttpUrlConnection> urlConnection;
};
