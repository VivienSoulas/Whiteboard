#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>

namespace string_utils
{
	std::string guessExtensionFromContentType(const std::string &content_type);
	bool endsWith(const std::string &str, const std::string &suffix);
	bool startsWith(const std::string &str, const std::string &prefix);
}

#endif // STRING_UTILS_HPP
