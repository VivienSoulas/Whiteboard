#ifndef MULTIPART_PARSER_HPP
#define MULTIPART_PARSER_HPP

#include <string>
#include <map>

namespace multipart_parser
{
	/** Extract boundary from Content-Type: multipart/form-data; boundary=... */
	std::string getMultipartBoundary(const std::map<std::string, std::string> &headers);

	/** Extract filename from Content-Disposition part headers (filename*= or filename=). */
	std::string getFilenameFromPartHeaders(const std::string &part_headers);

	/** Sanitize filename: basename only, no path or control chars. */
	std::string sanitizeUploadFilename(const std::string &filename);
}

#endif
