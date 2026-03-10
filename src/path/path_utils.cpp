#include "path/path_utils.hpp"
#include <algorithm>
#include <unistd.h>
#include <limits.h>
#include "logger.hpp"

namespace path_utils
{
	std::string resolveFilePath(const std::string &root, const std::string &request_path)
	{
		std::string path_suffix = request_path.empty() ? "/" : request_path;
		std::string base = root;
		if (base.empty() || base[base.size() - 1] != '/')
			base += '/';
		if (path_suffix[0] == '/')
			path_suffix = path_suffix.substr(1);
		std::string combined = base + path_suffix;
		std::string result;
		result.reserve(combined.size());
		for (size_t i = 0; i < combined.size();)
		{
			if (combined[i] == '/' && !result.empty() && result[result.size() - 1] == '/')
			{
				i++;
				continue;
			}
			if (i + 2 < combined.size() && combined[i] == '.' && combined[i + 1] == '.' && combined[i + 2] == '/')
			{
				i += 3;
				size_t pos = result.rfind('/');
				if (pos != std::string::npos && pos > 0)
					result.resize(pos);
				else
					result.clear();
				continue;
			}
			if (i + 1 < combined.size() && combined[i] == '.' && combined[i + 1] == '/')
			{
				i += 2;
				continue;
			}
			if (combined[i] == '/' && i + 1 == combined.size())
				break;
			result += combined[i];
			i++;
		}
		std::string final_path = result.empty() ? base : result;
		DEBUG_LOG("Resolved path: " << request_path << " -> " << final_path << " (root: " << root << ")");
		return final_path;
	}

	bool pathUnderRoot(const std::string &canonical, const std::string &root)
	{
		std::string r = root;
		if (r.empty()) return true;
		if (r[r.size() - 1] != '/')
			r += '/';
		
		std::string c = canonical;
		// If canonical matches root (without trailing slash), it's safe
		if (c == root || (c + "/") == r)
			return true;
		
		if (c.size() < r.size())
			return false;
			
		bool is_safe = c.compare(0, r.size(), r) == 0;
		if (!is_safe)
			DEBUG_LOG("Path safety check failed: " << canonical << " is NOT under root " << r);
		return is_safe;
	}

	std::string getContentType(const std::string &path)
	{
		size_t dot = path.rfind('.');
		if (dot == std::string::npos)
			return "application/octet-stream";
		std::string ext = path.substr(dot + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		if (ext == "html" || ext == "htm")
			return "text/html; charset=utf-8";
		if (ext == "css")
			return "text/css";
		if (ext == "js")
			return "application/javascript";
		if (ext == "json")
			return "application/json";
		if (ext == "png")
			return "image/png";
		if (ext == "jpg" || ext == "jpeg")
			return "image/jpeg";
		if (ext == "gif")
			return "image/gif";
		if (ext == "ico")
			return "image/x-icon";
		if (ext == "svg")
			return "image/svg+xml";
		if (ext == "pdf")
			return "application/pdf";
		if (ext == "txt")
			return "text/plain; charset=utf-8";
		return "application/octet-stream";
	}

	std::string normalizeAndMakeAbsolute(const std::string &base, const std::string &input)
	{
		std::string path = input;
		while (path.size() >= 2 && path[0] == '.' && path[1] == '/')
			path = path.substr(2);
		
		if (path.empty())
			return base;
		
		if (path[0] != '/')
		{
			if (!base.empty())
			{
				std::string joined = base;
				if (joined[joined.size() - 1] != '/')
					joined += '/';
				path = joined + path;
			}
			else
			{
				char cwd[PATH_MAX];
				if (getcwd(cwd, sizeof(cwd)))
					path = std::string(cwd) + "/" + path;
				else
					path = "./" + path;
			}
		}
		return path;
	}
}
