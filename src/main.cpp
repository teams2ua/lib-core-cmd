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
#include "api/BitcoinLikeAccount.hpp"
#include "api/BitcoinLikeTransactionBuilder.hpp"
#include "wallet/currencies.hpp"
#include "Helper.hpp"
#include <functional>
#include "Parameters.hpp"
#include "AddAccount.hpp"
#include "ListAccounts.hpp"
#include "SyncAccount.hpp"
#include "BuildTransaction.hpp"

int main(int argc, char** argv) {
	try
	{
		//throw MyExp("jopa");
		auto parsedParams = libcorecmd::parseParameters(argc, argv);
		auto applicationParams = parsedParams.first;
		if (applicationParams.command == libcorecmd::Command::HELP) {
			return 0;
		}

		auto executionContext = std::make_shared<AsioExecutionContext>();
		executionContext->start();

		auto httpClient = std::make_shared<AsioHttpClient>(executionContext);
		auto pathResolver = std::make_shared<libcorecmd::PathResolver>(applicationParams.dataFolder);
		auto threadDispathcher = std::make_shared<ThreadDispatcher>(executionContext);
		auto logPrinter = std::make_shared<LogPrinter>(executionContext);
		auto dbBackend = ledger::core::api::DatabaseBackend::getSqlite3Backend();
		
		auto pool = ledger::core::api::WalletPool::newInstance(
			applicationParams.dbName,
			applicationParams.dbPassword,
			httpClient,
			nullptr,
			pathResolver,
			logPrinter,
			threadDispathcher,
			nullptr,
			dbBackend,
			ledger::core::api::DynamicObject::newInstance(),
			nullptr,
			nullptr);

		switch (applicationParams.command)
		{
		case libcorecmd::Command::ADD_ACCOUNT:
			libcorecmd::addAccount(pool, applicationParams.explorerEndpoint, parsedParams.second);
			break;
		case libcorecmd::Command::LIST_ACCOUNTS:
			libcorecmd::listAccounts(pool);
			break;
		case libcorecmd::Command::LIST_CURRENCIES:
			
			break;
		case libcorecmd::Command::SYNC_ACCOUNT:
			libcorecmd::syncAccount(pool, executionContext, parsedParams.second);
			break;
		case libcorecmd::Command::GET_BALANCE:
			
			break;
		case libcorecmd::Command::BUILD_TRANSACTION:
			libcorecmd::buildTransaction(pool, parsedParams.second);
			break;
		default:
			// unknown values should be proceed by parser
			throw std::runtime_error("Unexpected code path.");
		}
		
		/*
		
		// get balance
		auto amount = GET_SYNC(Amount, account->getBalance(callback));
		std::cout << "Balance: " << amount->toLong() << std::endl;
		
		*/
		executionContext->stop();
	}
	catch (const std::exception & e)
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