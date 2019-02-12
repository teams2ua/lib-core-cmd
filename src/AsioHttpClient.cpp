#include "AsioHttpClient.hpp"
#include "AsioExecutionContext.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "api/ExecutionContext.hpp"
#include "api/HttpUrlConnection.hpp"

using namespace ledger::core;

AsioHttpClient::AsioHttpClient(const std::shared_ptr<AsioExecutionContext> &context) {
    _context = context;
}

void AsioHttpClient::execute(const std::shared_ptr<api::HttpRequest> &request) {
    struct Query {
        
		boost::asio::ip::tcp::resolver resolver;
		boost::asio::ip::tcp::socket socket;

        std::shared_ptr<ledger::core::api::HttpRequest> apiRequest;

        Query(asio::io_service& io_service, const std::shared_ptr<api::HttpRequest> &request)
        : resolver(io_service), socket(io_service), apiRequest(request) {

        }
    };

    
}
