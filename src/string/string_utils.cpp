#include "string/string_utils.hpp"
#include <algorithm>

namespace string_utils
{
	std::string guessExtensionFromContentType(const std::string &content_type)
	{
		std::string ct = content_type;
		std::transform(ct.begin(), ct.end(), ct.begin(), ::tolower);
		if (ct.find("text/plain") != std::string::npos)
			return ".txt";
		if (ct.find("text/html") != std::string::npos)
			return ".html";
		// Add more as needed
		return "";
	}

	bool endsWith(const std::string &str, const std::string &suffix)
	{
		if (str.length() < suffix.length()) return false;
		return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
	}

	bool startsWith(const std::string &str, const std::string &prefix)
	{
		if (str.length() < prefix.length()) return false;
		return str.compare(0, prefix.length(), prefix) == 0;
	}
} // namespace string_utils
