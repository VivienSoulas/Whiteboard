#ifndef DIR_UTILS_HPP
#define DIR_UTILS_HPP

#include <string>
#include <vector>

namespace dir_utils
{
	std::vector<std::string> getFilteredEntries(const std::string &dir_path, const std::string &base_path = "");
}

#endif // DIR_UTILS_HPP
