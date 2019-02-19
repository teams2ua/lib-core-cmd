#pragma once
#include "api/ExecutionContext.hpp"
#include <boost/asio.hpp>
#include <queue>
#include <mutex>

namespace ledger {
    namespace core {
        namespace api {
            class Runnable;
        };
    }
}

class AsioExecutionContext : public ledger::core::api::ExecutionContext {
public:
    void execute(const std::shared_ptr<ledger::core::api::Runnable> & runnable) override;

    void delay(const std::shared_ptr<ledger::core::api::Runnable> & runnable, int64_t millis) override;

	void start();
	void stop();

private:
	void run();
    bool runOne();
public:
    boost::asio::io_service _io_service;
private:
    std::queue<std::shared_ptr<ledger::core::api::Runnable>> q;
	std::mutex _lock;
	bool shouldStop{false};
	std::thread executionThread;
};