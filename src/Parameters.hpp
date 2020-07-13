#pragma once
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace libcorecmd {

    enum Command {
        HELP,
        ADD_ACCOUNT,
        LIST_ACCOUNTS,
        LIST_CURRENCIES,
        SYNC_ACCOUNT,
        GET_BALANCE,
        BUILD_TRANSACTION
    };

    struct ApplicationParameters {
        boost::filesystem::path dataFolder;
        std::string dbName;
        std::string dbPassword;
        std::string explorerEndpoint;
        Command command;
    };

    std::pair<ApplicationParameters, std::vector<std::string>> parseParameters(int argc, char** argv);
}