#include "path/dir_utils.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <cstring>
#include <limits.h>

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
			
			// Validate symlinks don't escape document root
			std::string full_path = dir_path + "/" + name;
			struct stat st;
			if (lstat(full_path.c_str(), &st) == 0)
			{
				if (S_ISLNK(st.st_mode))  // Is a symlink
				{
					char resolved_path[PATH_MAX];
					if (realpath(full_path.c_str(), resolved_path) != NULL)
					{
						// Check if resolved path starts with dir_path
						char resolved_dir[PATH_MAX];
						if (realpath(dir_path.c_str(), resolved_dir) != NULL)
						{
							std::string resolved_str = std::string(resolved_path);
							std::string resolved_dir_str = std::string(resolved_dir);
							// Ensure the symlink target is within the directory
							if (resolved_str.find(resolved_dir_str) != 0)
							{
								// Symlink escapes root, skip it
								continue;
							}
						}
					}
					else
					{
						// realpath failed, skip this symlink
						continue;
					}
				}
			}
			
			entries.push_back(sub);
		}
		closedir(dir);
		return entries;
	}
} // namespace dir_utils
