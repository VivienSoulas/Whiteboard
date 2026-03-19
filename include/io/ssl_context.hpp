#ifndef SSL_CONTEXT_HPP
#define SSL_CONTEXT_HPP

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <map>
#include <memory>

class SSLContext
{
private:
	SSL_CTX *_ctx;
	std::string _cert_path;
	std::string _key_path;

public:
	SSLContext(const std::string &cert_path, const std::string &key_path);
	~SSLContext();

	SSL_CTX *getCtx() const;
	bool isValid() const;
	std::string getLastError() const;
};

class SSLContextManager
{
private:
	static bool _initialized;
	std::map<std::string, std::shared_ptr<SSLContext>> _contexts;

public:
	SSLContextManager();
	~SSLContextManager();

	static void initializeSSL();
	static void cleanupSSL();

	bool createContext(const std::string &id, const std::string &cert_path, const std::string &key_path);
	SSL_CTX *getContext(const std::string &id) const;
	bool hasContext(const std::string &id) const;
};

#endif // SSL_CONTEXT_HPP
