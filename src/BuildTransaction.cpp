#include "BuildTransaction.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <api/Wallet.hpp>
#include <api/WalletType.hpp>
#include <api/WalletCallback.hpp>
#include <api/Amount.hpp>
#include <api/Account.hpp>
#include <api/AccountCallback.hpp>
#include <api/BitcoinLikeAccount.hpp>
#include <api/BitcoinLikeInput.hpp>
#include <api/BitcoinLikeTransaction.hpp>
#include <api/BitcoinLikeTransactionCallback.hpp>
#include <api/BitcoinLikePickingStrategy.hpp>
#include <api/BitcoinLikeTransactionBuilder.hpp>
#include <api/Currency.hpp>
#include "Helper.hpp"

namespace po = boost::program_options;

namespace libcorecmd {

	struct BuildTransactionParameters {
		int64_t amount;
		int64_t feesPerByte;
		std::string accountName;
		std::string destinationAddress;
		bool wasHelp;
	};

	BuildTransactionParameters parseParams(const std::vector<std::string>& options) {
		BuildTransactionParameters params;
		params.wasHelp = false;

		po::options_description generic_options("Generic options");
		generic_options.add_options()
			("help", "print this help")
			("name", po::value<std::string>(), "account name")
			("amount", po::value<int64_t>(), "amount to send")
			("address", po::value<std::string>(), "destination address")
			("fees", po::value<int64_t>()->default_value(20), "fees per byte");

		po::variables_map vm;

		po::parsed_options parsed = po::command_line_parser(options).
			options(generic_options).
			allow_unregistered().
			run();

		po::store(parsed, vm);
		if (vm.count("help")) {
			std::cout << generic_options << std::endl;
			params.wasHelp = true;
			return params;
		}
		params.accountName = vm["name"].as<std::string>();
		params.destinationAddress = vm["address"].as<std::string>();
		params.amount = vm["amount"].as<int64_t>();
		params.feesPerByte = vm["fees"].as<int64_t>();
		return params;
	}

	void printTransactionDetails(std::shared_ptr<ledger::core::api::BitcoinLikeTransaction> transaction) {
		std::cout << "Transaction built:" << std::endl;
		std::cout << "  inputs: " << transaction->getInputs().size() << std::endl;
		for (auto& input : transaction->getInputs()) {
			std::cout << "   " << input->getValue()->toString() << std::endl;
		}
		std::cout << "  fees:   " << transaction->getFees()->toLong() << std::endl;
	}

    void buildTransaction(std::shared_ptr<ledger::core::api::WalletPool> walletPool, const std::vector<std::string>& options) {
		auto params = parseParams(options);
		if (params.wasHelp) {
			return;
		}
		auto wallet = GET_SYNC(Wallet, walletPool->getWallet(params.accountName, callback));
		if (wallet->getWalletType() != ledger::core::api::WalletType::BITCOIN) {
			std::cout << "Build transaction is only allowed for bitcoin like accounts" << std::endl;
			return;
		}
		auto account = GET_SYNC(Account, wallet->getAccount(0, callback));
		auto bitcoinAccount = account->asBitcoinLikeAccount();
		auto builder = bitcoinAccount->buildTransaction(false);
		auto amount = ledger::core::api::Amount::fromLong(wallet->getCurrency(), params.amount);
		builder->sendToAddress(amount, params.destinationAddress);
		auto feesPerByte = ledger::core::api::Amount::fromLong(wallet->getCurrency(), params.feesPerByte);
		builder->setFeesPerByte(feesPerByte);
		builder->pickInputs(ledger::core::api::BitcoinLikePickingStrategy::OPTIMIZE_SIZE, 0xFFFFFF);
		auto transaction = GET_SYNC(BitcoinLikeTransaction, builder->build(callback));
		printTransactionDetails(transaction);
    }
}