#include "http/multipart/multipart_parser.hpp"
#include "string/string_parser.hpp"
#include <vector>

namespace multipart_parser
{
	std::string getMultipartBoundary(const std::map<std::string, std::string> &headers)
	{
		std::map<std::string, std::string>::const_iterator it = headers.find("content-type");
		if (it == headers.end())
			return "";

		std::vector<std::string> parts = string_parser::split(it->second, ';');
		for (size_t i = 0; i < parts.size(); ++i)
		{
			std::pair<std::string, std::string> kv = string_parser::splitOnce(string_parser::trim(parts[i]), '=');
			if (string_parser::toLower(kv.first) == "boundary")
			{
				std::string boundary = kv.second;
				if (boundary.size() >= 2 && boundary[0] == '"' && boundary[boundary.size() - 1] == '"')
					return boundary.substr(1, boundary.size() - 2);
				return boundary;
			}
		}
		return "";
	}

	std::string getFilenameFromPartHeaders(const std::string &part_headers)
	{
		std::vector<std::string> lines = string_parser::split(part_headers, '\n');
		std::string fallback_filename;

		for (size_t i = 0; i < lines.size(); ++i)
		{
			std::string line = string_parser::trim(lines[i]);
			while (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);

			std::pair<std::string, std::string> header_kv = string_parser::splitOnce(line, ':');
			if (string_parser::toLower(header_kv.first) == "content-disposition")
			{
				std::vector<std::string> params = string_parser::split(header_kv.second, ';');
				for (size_t j = 0; j < params.size(); ++j)
				{
					std::string param = string_parser::trim(params[j]);
					while (!param.empty() && param[param.size() - 1] == '\r')
						param.erase(param.size() - 1);

					std::pair<std::string, std::string> param_kv = string_parser::splitOnce(param, '=');
					std::string param_name = string_parser::toLower(param_kv.first);

					if (param_name == "filename*")
					{
						size_t idx = param_kv.second.find("''");
						if (idx != std::string::npos)
							return param_kv.second.substr(idx + 2);
					}
					else if (param_name == "filename")
					{
						std::string val = param_kv.second;
						if (val.size() >= 2 && val[0] == '"' && val[val.size() - 1] == '"')
							fallback_filename = val.substr(1, val.size() - 2);
						else
							fallback_filename = val;
					}
				}
			}
		}
		return fallback_filename;
	}

	std::string sanitizeUploadFilename(const std::string &filename)
	{
		std::string out;
		size_t pos = filename.find_last_of("/\\");
		std::string base = (pos == std::string::npos) ? filename : filename.substr(pos + 1);

		for (size_t i = 0; i < base.size(); ++i)
		{
			unsigned char c = static_cast<unsigned char>(base[i]);
			if (c >= 0x20 && c < 0x7F)
				out += base[i];
		}

		return out.empty() ? "uploaded" : out;
	}
}
