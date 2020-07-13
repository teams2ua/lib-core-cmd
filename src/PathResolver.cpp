#include "PathResolver.hpp"
#include <string>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace libcorecmd {
	
	std::string getAbsoluteBasedOn(const boost::filesystem::path& base, const std::string& path) {
		auto res = base;
		res.append(path);
		return boost::filesystem::absolute(res).generic_string();
	}

	PathResolver::PathResolver(boost::filesystem::path dataFolderPath) : _dataFolderPath(std::move(dataFolderPath)) {
		if (!boost::filesystem::exists(_dataFolderPath))
			throw std::runtime_error("Path not exist");
	};

	std::string PathResolver::resolveDatabasePath(const std::string& path) {
		auto res = _dataFolderPath;
		res.append("db.sqlite");
		res = boost::filesystem::relative(res);
		res.remove_trailing_separator();
		return res.generic_string();
	}

	std::string PathResolver::resolveLogFilePath(const std::string& path) {
		return getAbsoluteBasedOn(_dataFolderPath, path);
	}

	std::string PathResolver::resolvePreferencesPath(const std::string& path) {
		return getAbsoluteBasedOn(_dataFolderPath, path);
	}
}