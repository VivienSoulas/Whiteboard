#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <map>
#include <string>
#include <vector>
#include <cstddef>

class HttpResponse
{
	public:
		enum Version
		{
			HTTP_1_0,
			HTTP_1_1
		};

		typedef std::map<std::string, std::vector<std::string> > HeaderMap;

		HttpResponse();
		~HttpResponse();

		void setVersion(Version v);
		Version getVersion() const;

		void setStatus(int code);
		int getStatusCode() const;
		const std::string& getReasonPhrase() const;

		void setShouldClose(bool close);
		bool shouldClose() const;

		void setHeader(const std::string& name, const std::string& value);
		void addHeader(const std::string& name, const std::string& value);
		void removeHeader(const std::string& name);
		bool hasHeader(const std::string& name) const;
		std::string getHeader(const std::string& name) const;
		const HeaderMap& getHeaders() const;

		void setBody(const std::string& body);
		const std::string& getBody() const;
		std::size_t getBodySize() const;

		void setContentType(const std::string& value);

		std::string serialize() const;

		static std::string defaultReasonPhrase(int code);

	private:
		Version		version_;
		HeaderMap	headers_;
		std::string	body_;
		int			statusCode_;
		bool		shouldClose_;
		std::string	reasonPhrase_;

		static bool containsCRLF(const std::string& s);
	};

#endif
