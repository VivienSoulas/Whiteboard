#ifndef HTML_UTILS_HPP
#define HTML_UTILS_HPP

#include <string>
#include <vector>

namespace html_utils
{
	std::string buildDirectoryListing(const std::string &path, const std::vector<std::string> &entries);
	std::string buildUploadSuccess(const std::vector<std::string> &filenames);
}

#endif // HTML_UTILS_HPP
