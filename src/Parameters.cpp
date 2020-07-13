#include "Parameters.hpp"
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace po = boost::program_options;

namespace libcorecmd {
    std::pair<ApplicationParameters, std::vector<std::string>> parseParameters(int argc, char** argv) {
        ApplicationParameters params;
		po::options_description generic_options("Generic options");
		generic_options.add_options()
			("data-folder", po::value<std::string>()->default_value("."), "path to folder where all data contains")
			("db-name", po::value<std::string>()->default_value("cmd-wallet"), "name of database")
			("db-password", po::value<std::string>()->default_value(""), "database password")
			("explorer-endpoint", po::value<std::string>()->default_value("https://explorers.api.live.ledger.com"), "explorer endpoint");

		po::options_description global("Global options");
		global.add_options()
			("config-file", po::value<std::string>()->default_value("lib-core-cmd.config"), "path to config file")
			("command", po::value<std::string>()->default_value("help"), "command to execute [help, add, list, list-coins, sync, balance, build]")
			("subargs", po::value<std::vector<std::string> >(), "Arguments for command");

		po::positional_options_description pos;
		pos.add("command", 1).
			add("subargs", -1);

		po::variables_map vm;

		po::parsed_options parsed = po::command_line_parser(argc, argv).
			options(generic_options).
			options(global).
			positional(pos).
			allow_unregistered().
			run();

		po::store(parsed, vm);
		if (vm.count("config-file")) {
			po::store(po::parse_config_file(vm["config-file"].as<std::string>().c_str(), generic_options), vm);
		}
		params.dataFolder = boost::filesystem::path(vm["data-folder"].as<std::string>());
		if (!boost::filesystem::exists(params.dataFolder)) {
			throw std::runtime_error("Folder " + vm["data-folder"].as<std::string>() + " does not exist, specify correct path in `data-folder` option");
		}
		params.dbName = vm["db-name"].as<std::string>();
		params.dbPassword = vm["db-password"].as<std::string>();
		params.explorerEndpoint = vm["explorer-endpoint"].as<std::string>();
		auto command = boost::to_upper_copy(vm["command"].as<std::string>());

		if (command == "HELP") {
			params.command = Command::HELP;
			global.print(std::cout);
		} else if (command == "ADD") {
			params.command = Command::ADD_ACCOUNT;
		} else if (command == "LIST") {
			params.command = Command::LIST_ACCOUNTS;
		} else if (command == "LIST-COINS") {
			params.command = Command::LIST_CURRENCIES;
		} else if (command == "SYNC") {
			params.command = Command::SYNC_ACCOUNT;
		} else if (command == "BALANCE") {
			params.command = Command::GET_BALANCE;
		} else if (command == "BUILD") {
			params.command = Command::BUILD_TRANSACTION;
		}
		else {
			throw std::runtime_error("Unknown command " + command);
		}
		std::vector<std::string> otherParameters = po::collect_unrecognized(parsed.options, po::include_positional);
		return std::make_pair(params, otherParameters);
    };
}