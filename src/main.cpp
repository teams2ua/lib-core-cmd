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
#include "Helper.hpp"
#include <functional>

int main(int argc, char** argv) {
	try
	{
		//std::string xpub = "xpub6CQRBg1k8KN2yZfWUywHt9dtDSEFfHhwhBrEjzuHj5YBV2p81NEviAhEhGzYpC5AzwuL6prM2wc1oyMQ8hmsCKqrWwHrjcboQvkBctC1JTq";

        // hash of 'ledger', used in tests, no segwit
        std::string xpub = "xpub661MyMwAqRbcExQa2CM5QpU2GJ2ZyPFmomjNx7QDW1MidgPsZZz7Ew64bMh6K1Uyy7Hu5g5e7ib7KQhEAjw3vZAeHpmvcMRztnJ6YHtwSBG";

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
		//walletConfig->putString(api::Configuration::KEYCHAIN_ENGINE, api::KeychainEngines::BIP49_P2SH); // segwit
        walletConfig->putString(api::Configuration::KEYCHAIN_ENGINE, api::KeychainEngines::BIP32_P2PKH);
        walletConfig->putString(api::Configuration::BLOCKCHAIN_EXPLORER_API_ENDPOINT, "http://127.0.0.1:8080/");

		// get wallet
		auto wallet = GET_SYNC(Wallet, pool->createWallet("wallet1", ledger::core::currencies::BITCOIN, walletConfig, callback));
		
		// create account
		api::ExtendedKeyAccountCreationInfo info;
		info.index = 0;
		info.extendedKeys.push_back(xpub);
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