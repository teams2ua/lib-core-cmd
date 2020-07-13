#include "ListAccounts.hpp"
#include <api/Account.hpp>
#include <api/AccountCallback.hpp>
#include <api/Currency.hpp>
#include <api/KeychainEngines.hpp>
#include <api/DynamicObject.hpp>
#include <api/Configuration.hpp>
#include <api/ExtendedKeyAccountCreationInfo.hpp>
#include <api/Wallet.hpp>
#include <api/WalletListCallback.hpp>
#include <wallet/currencies.hpp>
#include "Helper.hpp"

namespace libcorecmd {

	void listAccounts(std::shared_ptr<ledger::core::api::WalletPool> walletPool) {
		// we will create separate wallet for each account
		// get wallet
		
		auto walletsOption = GET_SYNC_LIST(Wallet, walletPool->getWallets(0, 1000, callback));
		if (!walletsOption) {
			std::cout << "0 accounts found" << std::endl;
			return;
		}
		auto wallets = walletsOption.value();
		if (wallets.empty()) {
			std::cout << "0 accounts found" << std::endl;
			return;
		}
		for (auto& wallet : wallets) {
			auto account = GET_SYNC(Account, wallet->getAccount(0, callback));
			std::cout << wallet->getName() << " " << account->getRestoreKey() << " " << wallet->getConfiguration()->getString(ledger::core::api::Configuration::KEYCHAIN_ENGINE).value() << std::endl;
		}
	}
}