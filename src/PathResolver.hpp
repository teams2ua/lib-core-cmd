#pragma once

#include "api/PathResolver.hpp"
#include <string>
#include <boost/filesystem.hpp>

namespace libcorecmd {
	class PathResolver : public ledger::core::api::PathResolver {
	public:
		PathResolver(boost::filesystem::path dataFolderPath);

		std::string resolveDatabasePath(const std::string& path) override;

		std::string resolveLogFilePath(const std::string& path) override;

		std::string resolvePreferencesPath(const std::string& path) override;
	private:
		boost::filesystem::path _dataFolderPath;
	};
}