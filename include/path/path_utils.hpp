#ifndef PATH_UTILS_HPP
#define PATH_UTILS_HPP

#include <string>

namespace path_utils
{
	/** Resolve request path against server root and location path. Normalizes ".." and "." */
	std::string resolveFilePath(const std::string &root, const std::string &request_path);

	/** True if canonical path is under root (no escape). */
	bool pathUnderRoot(const std::string &canonical, const std::string &root);

	/** MIME type from file path (extension). */
	std::string getContentType(const std::string &path);

	std::string normalizeAndMakeAbsolute(const std::string &base, const std::string &input);
}

#endif
