#include <memory>
#include "AsioExecutionContext.hpp"
#include "api/Runnable.hpp"
#include <iostream>

void AsioExecutionContext::execute(const std::shared_ptr<ledger::core::api::Runnable> & runnable) {
	_io_service.post([runnable]() { runnable->run(); });
};

void AsioExecutionContext::delay(const std::shared_ptr<ledger::core::api::Runnable> & runnable, int64_t millis) {

}

void AsioExecutionContext::start() {
	shouldStop = false;
	std::swap(executionThread, std::thread([&]() {
		while (true) {
			try {
				if ((_io_service.run() == 0) && shouldStop) // exit only when there are nothing more to do
					return;
				_io_service.reset();
			}
			catch (std::exception const& r) {
				std::cout << "Error: " << r.what() << std::endl;
				throw;
			}
			catch (...) {
				std::cout << "Something bad happened" << std::endl;
				throw;
			}
		}
	}));
}

void AsioExecutionContext::stop() {
	shouldStop = true;
	executionThread.join();
}