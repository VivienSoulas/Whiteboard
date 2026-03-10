#include "http/static_file_handler.hpp"
#include "path/path_utils.hpp"
#include "http/http_response_factory.hpp"
#include "http/http_status.hpp"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <limits.h>
#include "string/html_utils.hpp"
#include "path/dir_utils.hpp"
#include "logger.hpp"

namespace static_file_handler
{
	std::string serve(const ServerConfig &server, const LocationConfig &location, const std::string &path, bool head_only)
	{
		std::string norm_path = path;
		std::string root = location.root.empty() ? server.root : location.root;
		std::string effective_request_path = path;
		
		if (!location.root.empty())
		{
			if (effective_request_path.find(location.path) == 0 && location.path != "/")
			{
				effective_request_path.erase(0, location.path.length());
			}
		}

		// Normalize: treat /upload and /upload/ as directory
		if (!norm_path.empty() && norm_path[norm_path.size() - 1] != '/')
		{
			struct stat st_test;
			std::string test_file_path = path_utils::resolveFilePath(root, effective_request_path);
			if (!test_file_path.empty())
			{
				if (test_file_path[test_file_path.size() - 1] != '/')
					test_file_path += '/';
				if (stat(test_file_path.c_str(), &st_test) == 0 && S_ISDIR(st_test.st_mode))
				{
					effective_request_path += '/';
				}
			}
		}

		std::string file_path = path_utils::resolveFilePath(root, effective_request_path);
		if (file_path.empty()) {
			DEBUG_LOG("Failed to resolve file path for " << path << " (root: " << root << ")");
			return "";
		}

		// Resolve relative paths against current working directory
		file_path = path_utils::normalizeAndMakeAbsolute("", file_path);
		DEBUG_LOG("Absolute file path: " << file_path);

		struct stat st;
		if (stat(file_path.c_str(), &st) != 0) {
			DEBUG_LOG("File not found (stat failed): " << file_path);
			return "";
		}
		DEBUG_LOG("Stat success for: " << file_path << " (S_ISDIR: " << S_ISDIR(st.st_mode) << ")");

		if (S_ISDIR(st.st_mode))
		{
			// If directory listing is enabled, return a generated HTML listing
			if (location.directory_listing)
			{
				DEBUG_LOG("Generating directory listing for: " << file_path);
				std::vector<std::string> entries = dir_utils::getFilteredEntries(file_path, path);
				std::string html = html_utils::buildDirectoryListing(path, entries);
				HttpResponse res;
				res.setStatus(HttpStatus::OK);
				res.setContentType("text/html; charset=utf-8");
				res.setBody(html);
				res.setShouldClose(false);
				return res.serialize();
			}
			DEBUG_LOG("Directory listing NOT enabled for: " << file_path);

			// Otherwise, try to serve an index file (e.g., index.html)
			std::string index_path = file_path;
			// Ensure trailing slash for directory
			if (index_path[index_path.size() - 1] != '/')
				index_path += '/';
			// Use custom index if specified, otherwise default to "index.html"
			if (!location.index.empty())
				index_path += location.index;
			else
				index_path += "index.html";
			// If index file does not exist or is not a regular file, return empty (404)
			if (stat(index_path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
				DEBUG_LOG("Index file not found or invalid: " << index_path);
				return "";
			}
			DEBUG_LOG("Serving index file: " << index_path);
			file_path = index_path;
			st.st_size = 0;
			// Re-stat to get correct file info
			if (stat(file_path.c_str(), &st) != 0) {
				DEBUG_LOG("Failed to re-stat index file: " << file_path);
				return "";
			}
		}

		if (!S_ISREG(st.st_mode)) {
			DEBUG_LOG("Requested path is not a regular file: " << file_path);
			return "";
		}

		// Ensure path is under server root (no escape)
		std::string root_abs = path_utils::normalizeAndMakeAbsolute("", root);
		if (!path_utils::pathUnderRoot(file_path, root_abs)) {
			DEBUG_LOG("Path escape attempt blocked: " << file_path << " (not under root " << root_abs << ")");
			return "";
		}

		std::ifstream f(file_path.c_str(), std::ios::binary);
		if (!f) {
			DEBUG_LOG("Failed to open file for reading: " << file_path);
			return "";
		}
		std::string body((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		f.close();

		HttpResponse res;
		res.setStatus(HttpStatus::OK);
		res.setContentType(path_utils::getContentType(file_path));
		res.setBody(body);
		res.setShouldClose(false);
		std::string full = res.serialize();
		if (head_only)
		{
			size_t header_end = full.find("\r\n\r\n");
			if (header_end != std::string::npos)
				full.resize(header_end + 4);
		}
		DEBUG_LOG("Successfully serving file: " << file_path << " (" << body.size() << " bytes)");
		return full;
	}
}
