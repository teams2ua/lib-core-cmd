#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include "AsioHttpClient.hpp"
#include "AsioExecutionContext.hpp"
#include "PathResolver.hpp"
#include "LogPrinter.hpp"
#include "ThreadDispatcher.hpp"
#include "api/WalletPool.hpp"
#include "api/DynamicObject.hpp"
#include "api/DatabaseBackend.hpp"
#include "api/DatabaseEngine.hpp"
#include "api/Wallet.hpp"
#include "api/WalletCallback.hpp"
#include "api/Account.hpp"
#include "api/AccountCallback.hpp"
#include "api/AccountCreationInfo.hpp"
#include "api/ExtendedKeyAccountCreationInfo.hpp"
#include "api/EventBus.hpp"
#include "api/Event.hpp"
#include "api/EventReceiver.hpp"
#include "api/EventCode.hpp"
#include "api/Amount.hpp"
#include "api/AmountCallback.hpp"
#include "api/HttpRequest.hpp"
#include "api/Configuration.hpp"
#include "api/KeychainEngines.hpp"
#include "wallet/currencies.hpp"
#include <functional>

using namespace ledger::core;

#define GET_SYNC(type, operation) sync<api::type ## Callback, api::type>([&](std::shared_ptr<api::type ## Callback> & callback) { operation; });

class EventReceiver : public api::EventReceiver {
public:
	void onEvent(const std::shared_ptr<api::Event> & incomingEvent) override {
		std::lock_guard<std::mutex> guard(_lock);
		if (incomingEvent->getCode() == api::EventCode::SYNCHRONIZATION_FAILED) {
			_error = api::Error(api::ErrorCode::UNKNOWN, "Synchronization failed");
			_cond.notify_all();
			return;
		}
		if (incomingEvent->getCode() == api::EventCode::SYNCHRONIZATION_SUCCEED ||
			incomingEvent->getCode() == api::EventCode::SYNCHRONIZATION_SUCCEED_ON_PREVIOUSLY_EMPTY_ACCOUNT){
			done = true;
			_cond.notify_all();
		}
	}

	void wait() {
		std::unique_lock<std::mutex> lk(_lock);
		_cond.wait(lk, [&] { return _error || done; });
		if (done)
			return;
		throw std::runtime_error(_error.value().message);
	}
private:
	std::experimental::optional<api::Error> _error;
	bool done{false};
	std::mutex _lock;
	std::condition_variable _cond;
};

template<typename CallbackType, typename ParameterType>
class Callback : public CallbackType {
public:
	
	void onCallback(const ParameterType & value, const std::experimental::optional<api::Error> & error) override {
		std::lock_guard<std::mutex> guard(_lock);
		if (error) {
			_error = error;
		}
		else {
			_result = value;
		}
		_cond.notify_all();
	}

	ParameterType WaitForResult() {
		std::unique_lock<std::mutex> lk(_lock);
		_cond.wait(lk, [&] { return _error || _result; });
		if (_result)
			return _result;
		throw std::runtime_error(_error.value().message);
	}
private:
	std::experimental::optional<api::Error> _error;
	ParameterType _result;
	std::mutex _lock;
	std::condition_variable _cond;
};

template<typename CallbackType, typename ParameterType>
std::shared_ptr<ParameterType> sync(std::function<void(std::shared_ptr<CallbackType>)> operation) {
	auto cb = std::make_shared<Callback<CallbackType, std::shared_ptr<ParameterType>>>();
	operation(cb);
	return cb->WaitForResult();
}

int main(int argc, char** argv) {
	try
	{
		auto executionContext = std::make_shared<AsioExecutionContext>();
		executionContext->start();
		auto httpClient = std::make_shared<AsioHttpClient>(executionContext);

		auto pathResolver = std::make_shared<PathResolver>();
		auto threadDispathcher = std::make_shared<ThreadDispatcher>(executionContext);
		auto logPrinter = std::make_shared<LogPrinter>(executionContext);
		auto config = api::DynamicObject::newInstance();
		auto dbBackend = api::DatabaseBackend::getSqlite3Backend();
		std::experimental::optional<std::string> password;
		auto pool = api::WalletPool::newInstance(
			"cmd-wallet",
			password,
			httpClient,
			nullptr,
			pathResolver,
			logPrinter,
			threadDispathcher,
			nullptr,
			dbBackend,
			config);
		auto walletConfig = api::DynamicObject::newInstance();
		walletConfig->putString(api::Configuration::KEYCHAIN_ENGINE, api::KeychainEngines::BIP49_P2SH); // segwit

		// get wallet
		auto wallet = GET_SYNC(Wallet, pool->createWallet("wallet1", ledger::core::currencies::BITCOIN, walletConfig, callback));
		
		// create account
		api::ExtendedKeyAccountCreationInfo info;
		info.index = 0;
		info.extendedKeys.push_back("xpub6CQRBg1k8KN2yZfWUywHt9dtDSEFfHhwhBrEjzuHj5YBV2p81NEviAhEhGzYpC5AzwuL6prM2wc1oyMQ8hmsCKqrWwHrjcboQvkBctC1JTq");
		auto account = GET_SYNC(Account, wallet->newAccountWithExtendedKeyInfo(info, callback));
		
		// sync account
		auto eventReceiver = std::make_shared<EventReceiver>();
		auto bus = account->synchronize();
		bus->subscribe(executionContext, eventReceiver);
		eventReceiver->wait();

		// get balance
		auto amount = GET_SYNC(Amount, account->getBalance(callback));
		std::cout << "Balance: " << amount->toLong() << std::endl;
		/**/
		executionContext->stop();
	}
	catch (std::exception & e)
	{
		std::cout << "Unhandled exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (std::runtime_error & e)
	{
		std::cout << "Unhandled exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (...)
	{
		std::cout << "Unhandled exception " << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}