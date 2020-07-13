#pragma once
#include <api/ExecutionContext.hpp>
#include <api/WalletPool.hpp>
#include "AsioExecutionContext.hpp"
#include <vector>
#include <string>
#include <memory>

namespace libcorecmd {
    void syncAccount(std::shared_ptr<ledger::core::api::WalletPool> walletPool, std::shared_ptr<AsioExecutionContext>& context, const std::vector<std::string>& options);
}