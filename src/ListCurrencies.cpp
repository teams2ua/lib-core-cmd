#include "ListCurrencies.hpp"
#include <iostream>
#include <wallet/currencies.hpp>

namespace libcorecmd {

	void listCurrencies() {
		for (auto& coin : ledger::core::currencies::ALL) {
			std::cout << coin.name << std::endl;
		}
	}
}