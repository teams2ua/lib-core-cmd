#pragma once
#include <api/WalletPool.hpp>
#include <memory>
#include <string>
#include <vector>

namespace libcorecmd {
    void listAccounts(std::shared_ptr<ledger::core::api::WalletPool> walletPool);
}