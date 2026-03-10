#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include "http/http_request.hpp"
#include <string>
#include <cstddef>

class HttpParser
{
public:
	enum State
	{
		READING_HEADERS,
		READING_BODY,
		COMPLETE,
		ERROR
	};

	enum ChunkPhase
	{
		READING_CHUNK_SIZE,
		READING_CHUNK_DATA,
		READING_CHUNK_TRAILERS
	};

	struct Result
	{
		State state;
		int statusCode;
		bool shouldClose;

		Result();
		Result(State s, int code, bool close);
	};

	HttpParser();
	~HttpParser();

	void setMaxHeaderBytes(std::size_t n);
	void setMaxBodyBytes(std::size_t n);
	Result validateBodySize();

	Result feed(const char *data, std::size_t len);
	const HttpRequest &getRequest() const;

	void reset();

private:
	State state_;
	HttpRequest request_;
	std::string buffer_;

	std::size_t maxHeaderBytes_;
	std::size_t headerEndPos_;
	std::size_t maxBodyBytes_;
	std::size_t contentLength_;
	bool hasContentLength_;
	bool shouldClose_;
	int errorStatus_;

	ChunkPhase chunkPhase_;
	std::size_t currentChunkSize_;
	bool isChunked_;

	Result tryParsingHeaders();
	Result tryParsingBody();
	Result tryParsingChunkedBody();
	Result setError(int statusCode, bool shouldClose);

	bool parseRequestLine(const std::string &line);
	bool parseHeaderLine(const std::string &line);
	static bool parseContentLength(const std::string &value, std::size_t &outLen);
	static bool parseChunkSizeLine(const std::string &line, std::size_t &outSize);

	static void splitTarget(const std::string &target, std::string &path, std::string &query);

	bool hasNonEmptyHeader(const std::string &name) const;
	bool hasHeaderToken(const std::string &header, const std::string &token) const;
	bool computeShouldClose(const HttpRequest &req) const;
};

#endif
