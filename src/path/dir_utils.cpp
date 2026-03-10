#include "path/dir_utils.hpp"
#include <dirent.h>

namespace dir_utils
{
	std::vector<std::string> getFilteredEntries(const std::string &dir_path, const std::string &base_path)
	{
		std::vector<std::string> entries;
		DIR *dir = opendir(dir_path.c_str());
		if (!dir)
			return entries;
		struct dirent *ent;
		while ((ent = readdir(dir)) != NULL)
		{
			std::string name = ent->d_name;
			if (name == "." || name == "..")
				continue;
			std::string sub = base_path;
			if (sub.empty() || sub[sub.size() - 1] != '/')
				sub += '/';
			sub += name;
			entries.push_back(sub);
		}
		closedir(dir);
		return entries;
	}
} // namespace dir_utils
