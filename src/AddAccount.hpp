#pragma once
#include <api/WalletPool.hpp>
#include <memory>
#include <string>
#include <vector>

namespace libcorecmd {
    void addAccount(std::shared_ptr<ledger::core::api::WalletPool> walletPool, const std::string& explorerUrl, const std::vector<std::string>& options);
}