#include "http/upload_handler.hpp"
#include "http/multipart/multipart_parser.hpp"
#include "http/http_response_factory.hpp"
#include "http/http_status.hpp"
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <limits.h>
#include "string/html_utils.hpp"
#include "path/path_utils.hpp"
#include "string/string_utils.hpp"
#include "string/string_parser.hpp"
#include "logger.hpp"

namespace upload_handler
{
	std::string handle(const LocationConfig &location, const HttpRequest &req)
	{
		std::string boundary = multipart_parser::getMultipartBoundary(req.headers);
		if (boundary.empty())
		{
			DEBUG_LOG("Upload failed: Missing multipart boundary in headers");
		HttpResponse res = HttpResponseFactory::buildError(req, nullptr, HttpStatus::BAD_REQUEST, true);
			return res.serialize();
		}
		DEBUG_LOG("Starting multipart upload processing (boundary: " << boundary << ")");

		std::string upload_dir = location.upload_dir;
		std::string delim = "\r\n--" + boundary;
		std::string delim_start = "--" + boundary;
		const std::string &body = req.body;
		std::vector<std::string> saved_names;
		std::string::size_type pos = 0;
		bool first = true;

		while (pos < body.size())
		{
			std::string::size_type next;
			std::string::size_type part_start;
			if (first && string_utils::startsWith(body, delim_start))
			{
				next = 0;
				part_start = delim_start.size();
				first = false;
			}
			else
			{
				next = body.find(delim, pos);
				if (next == std::string::npos) {
					DEBUG_LOG("End of multipart body reached");
					break;
				}
				part_start = next + delim.size();
			}
			if (part_start + 2 <= body.size() && body.compare(part_start, 2, "--") == 0)
				break;
			if (part_start < body.size() && body[part_start] == '\r')
				++part_start;
			if (part_start < body.size() && body[part_start] == '\n')
				++part_start;
			std::string::size_type header_end = body.find("\r\n\r\n", part_start);
			if (header_end == std::string::npos) {
				DEBUG_LOG("Multipart part headers separator not found");
				break;
			}
			std::string part_headers = body.substr(part_start, header_end - part_start);
			std::string::size_type content_start = header_end + 4;
			std::string::size_type content_end = body.find(delim, content_start);
			if (content_end == std::string::npos)
				content_end = body.size();
			while (content_end > content_start && (body[content_end - 1] == '\n' || body[content_end - 1] == '\r'))
				--content_end;
			std::string filename = multipart_parser::getFilenameFromPartHeaders(part_headers);
			DEBUG_LOG("Processing part: " << (filename.empty() ? "(no filename)" : filename));
			if (!filename.empty())
			{
				if (filename.find('.') == std::string::npos)
				{
					std::vector<std::string> lines = string_parser::split(part_headers, '\n');
					for (size_t i = 0; i < lines.size(); ++i) {
						std::pair<std::string, std::string> kv = string_parser::splitOnce(lines[i], ':');
						if (string_parser::toLower(string_parser::trim(kv.first)) == "content-type") {
							std::string ct = string_parser::trim(string_parser::splitOnce(kv.second, ';').first);
							filename += string_utils::guessExtensionFromContentType(ct);
							break;
						}
					}
				}
				filename = multipart_parser::sanitizeUploadFilename(filename);
				if (filename.empty())
					filename = "uploaded";
				std::string path = path_utils::normalizeAndMakeAbsolute(upload_dir, filename);
				std::string temp_path = path + ".tmp";
				std::ofstream out(temp_path.c_str(), std::ios::binary);
				if (out)
				{
					out.write(body.data() + content_start, content_end - content_start);
					out.close();
					if (rename(temp_path.c_str(), path.c_str()) == 0)
					{
					DEBUG_LOG("Saved uploaded file to: " << path << " (" << (content_end - content_start) << " bytes)");
					saved_names.push_back(filename);
					}
					else
					{
						unlink(temp_path.c_str());
						DEBUG_LOG("Failed to rename temp file to final path");
					}
				}
				else
				{
					DEBUG_LOG("Failed to open temp file for writing: " << temp_path);
				}
			}
			pos = (next == 0) ? content_end : next + delim.size();
		}

		if (saved_names.empty())
		{
			DEBUG_LOG("No files were successfully saved from the upload");
		HttpResponse res = HttpResponseFactory::buildError(req, nullptr, HttpStatus::BAD_REQUEST, true);
			return res.serialize();
		}
		DEBUG_LOG("Upload successful: " << saved_names.size() << " files saved");

		std::string html = html_utils::buildUploadSuccess(saved_names);
		HttpResponse res = HttpResponseFactory::buildOk(req, html, "text/html; charset=utf-8", true);
		res.setStatus(HttpStatus::CREATED);
		return res.serialize();
	}
}
