#include "string/html_utils.hpp"
#include <sstream>

namespace html_utils
{

	std::string buildDirectoryListing(const std::string &path, const std::vector<std::string> &entries)
	{
		std::ostringstream html;
		html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Index of " << path << "</title></head><body><h1>Index of " << path << "</h1><ul>";
		for (size_t i = 0; i < entries.size(); ++i)
		{
			std::string escaped = escapeHtml(entries[i]);
			html << "<li><a href=\"" << escaped << "\">" << escaped << "</a></li>";
		}
		html << "</ul></body></html>";
		return html.str();
	}

	std::string buildUploadSuccess(const std::vector<std::string> &filenames)
	{
		std::ostringstream html;
		html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Upload</title></head><body><h1>Upload successful</h1><ul>";
		for (size_t i = 0; i < filenames.size(); ++i)
		{
			html << "<li>" << filenames[i] << "</li>";
		}
		html << "</ul></body></html>";
		return html.str();
	}

	std::string escapeHtml(const std::string &str)
	{
		std::string result;
		for (size_t i = 0; i < str.size(); ++i)
		{
			char c = str[i];
			if (c == '<') result += "&lt;";
			else if (c == '>') result += "&gt;";
			else if (c == '&') result += "&amp;";
			else if (c == '"') result += "&quot;";
			else if (c == '\'') result += "&#39;";
			else result += c;
		}
		return result;
	}

} // namespace html_utils
