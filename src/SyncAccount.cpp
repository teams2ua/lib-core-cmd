#include "SyncAccount.hpp"
#include <iostream>
#include <boost/program_options.hpp>
#include <api/Account.hpp>
#include <api/AccountCallback.hpp>
#include <api/Wallet.hpp>
#include <api/WalletCallback.hpp>
#include <api/ExecutionContext.hpp>
#include <api/EventBus.hpp>
#include "AsioExecutionContext.hpp"
#include "Helper.hpp"

namespace po = boost::program_options;

namespace libcorecmd {
    void syncAccount(std::shared_ptr<ledger::core::api::WalletPool> walletPool, std::shared_ptr<AsioExecutionContext>& context, const std::vector<std::string>& options) {
		po::options_description generic_options("Generic options");
		generic_options.add_options()
			("help", "print this help")
			("name", po::value<std::string>(), "account name");

		po::variables_map vm;

		po::parsed_options parsed = po::command_line_parser(options).
			options(generic_options).
			allow_unregistered().
			run();

		po::store(parsed, vm);
		if (vm.count("help")) {
			std::cout << generic_options << std::endl;
			return;
		}
		std::string accountName= vm["name"].as<std::string>();
		auto wallet = GET_SYNC(Wallet, walletPool->getWallet(accountName, callback));
		auto account = GET_SYNC(Account, wallet->getAccount(0, callback));
		auto eventReceiver = std::make_shared<EventReceiver>();
		auto bus = account->synchronize();
		bus->subscribe(context, eventReceiver);
		eventReceiver->wait();
    }
}