#include "AddAccount.hpp"
#include <api/Account.hpp>
#include <api/AccountCallback.hpp>
#include <api/Currency.hpp>
#include <api/KeychainEngines.hpp>
#include <api/DynamicObject.hpp>
#include <api/Configuration.hpp>
#include <api/ExtendedKeyAccountCreationInfo.hpp>
#include <api/Wallet.hpp>
#include <api/WalletCallback.hpp>
#include <api/WalletListCallback.hpp>
#include <wallet/currencies.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "Helper.hpp"

namespace po = boost::program_options;

namespace libcorecmd {
    struct AddAccountParameters {
        std::string xpub;
		std::string name;
        ledger::core::api::Currency currency;
        std::string keychainEngine;
		bool wasHelp = false;
    };

    AddAccountParameters parseParams(const std::vector<std::string>& options) {
		AddAccountParameters params;
		
		po::options_description generic_options("Generic options");
		generic_options.add_options()
			("help", "print this help")
			("xpub", po::value<std::string>(), "xpub of the account")
			("name", po::value<std::string>(), "account name")
			("currency", po::value<std::string>()->default_value("bitcoin"), "currency, use list-coins to see possible coins")
			("keychain-engine", po::value<std::string>()->default_value("BIP32_P2PKH"), "script-type/keychainEngine possible value [BIP32_P2PKH, BIP49_P2SH, BIP173_P2WPKH, BIP173_P2WSH]");

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
		params.name = vm["name"].as<std::string>();
		params.xpub = vm["xpub"].as<std::string>();
		bool found = false;
		const std::string currencyName = boost::to_upper_copy(vm["currency"].as<std::string>());
		for (auto& coin : ledger::core::currencies::ALL) {
			if (boost::to_upper_copy(coin.name) == currencyName) {
				found = true;
				params.currency = coin;
				break;
			}
		}
		if (!found) {
			throw std::runtime_error("Currency name is incorrect.");
		}
		const std::string keychainEngine = boost::to_upper_copy(vm["keychain-engine"].as<std::string>());
		if (keychainEngine == boost::to_upper_copy(ledger::core::api::KeychainEngines::BIP49_P2SH)) {
			params.keychainEngine = ledger::core::api::KeychainEngines::BIP49_P2SH;
		} else if (keychainEngine == boost::to_upper_copy(ledger::core::api::KeychainEngines::BIP32_P2PKH)) {
			params.keychainEngine = ledger::core::api::KeychainEngines::BIP32_P2PKH;
		} else if (keychainEngine == boost::to_upper_copy(ledger::core::api::KeychainEngines::BIP173_P2WSH)) {
			params.keychainEngine = ledger::core::api::KeychainEngines::BIP173_P2WSH;
		} else if (keychainEngine == boost::to_upper_copy(ledger::core::api::KeychainEngines::BIP173_P2WPKH)) {
			params.keychainEngine = ledger::core::api::KeychainEngines::BIP173_P2WPKH;
		}
		else {
			throw std::runtime_error("Unknown keychain-engine");
		}
		return params;
    }

    void addAccount(std::shared_ptr<ledger::core::api::WalletPool> walletPool, const std::string& explorerUrl, const std::vector<std::string>& options) {
        auto params = parseParams(options);
		if (params.wasHelp)
			return;
		auto walletConfig = ledger::core::api::DynamicObject::newInstance();
		walletConfig->putString(ledger::core::api::Configuration::KEYCHAIN_ENGINE, params.keychainEngine);
		walletConfig->putString(ledger::core::api::Configuration::BLOCKCHAIN_EXPLORER_API_ENDPOINT, explorerUrl);

		// we will create separate wallet for each account
		// get wallet
		auto walletsOption = GET_SYNC_LIST(Wallet, walletPool->getWallets(0, 1000, callback));
		if (walletsOption) {
			auto wallets = walletsOption.value();
			if (!wallets.empty()) {
				for (auto& wallet : wallets) {
					if (wallet->getName() == params.name) {
						std::cout << "Account `" << params.name << "` is already exists.";
						return;
					}
				}
			}
		}

		auto wallet = GET_SYNC(Wallet, walletPool->createWallet(params.name, params.currency, walletConfig, callback));
		// create account
		ledger::core::api::ExtendedKeyAccountCreationInfo info;
		info.index = 0;
		info.extendedKeys.push_back(params.xpub);
		auto account = GET_SYNC(Account, wallet->newAccountWithExtendedKeyInfo(info, callback));
    }
}