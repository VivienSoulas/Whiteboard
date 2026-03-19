#include "io/ssl_context.hpp"
#include <iostream>
#include <fstream>

// Initialize static member
bool SSLContextManager::_initialized = false;

SSLContext::SSLContext(const std::string &cert_path, const std::string &key_path)
	: _ctx(nullptr), _cert_path(cert_path), _key_path(key_path)
{
	_ctx = SSL_CTX_new(SSLv23_server_method());
	if (!_ctx)
	{
		std::cerr << "Error creating SSL context" << std::endl;
		return;
	}

	// Load certificate
	if (SSL_CTX_use_certificate_file(_ctx, _cert_path.c_str(), SSL_FILETYPE_PEM) <= 0)
	{
		std::cerr << "Error loading certificate: " << _cert_path << std::endl;
		ERR_print_errors_fp(stderr);
		SSL_CTX_free(_ctx);
		_ctx = nullptr;
		return;
	}

	// Load private key
	if (SSL_CTX_use_PrivateKey_file(_ctx, _key_path.c_str(), SSL_FILETYPE_PEM) <= 0)
	{
		std::cerr << "Error loading private key: " << _key_path << std::endl;
		ERR_print_errors_fp(stderr);
		SSL_CTX_free(_ctx);
		_ctx = nullptr;
		return;
	}

	// Verify that private key matches certificate
	if (!SSL_CTX_check_private_key(_ctx))
	{
		std::cerr << "Private key does not match certificate" << std::endl;
		SSL_CTX_free(_ctx);
		_ctx = nullptr;
		return;
	}

	// Set SSL options for non-blocking I/O and disable weak TLS versions
	SSL_CTX_set_options(_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
	SSL_CTX_set_mode(_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	// Enforce minimum TLS 1.2
	if (!SSL_CTX_set_min_proto_version(_ctx, TLS1_2_VERSION))
	{
		std::cerr << "Warning: Failed to set minimum TLS version to 1.2" << std::endl;
	}
}

SSLContext::~SSLContext()
{
	if (_ctx)
	{
		SSL_CTX_free(_ctx);
		_ctx = nullptr;
	}
}

SSL_CTX *SSLContext::getCtx() const
{
	return _ctx;
}

bool SSLContext::isValid() const
{
	return _ctx != nullptr;
}

std::string SSLContext::getLastError() const
{
	char error_buffer[256] = {0};
	ERR_error_string_n(ERR_get_error(), error_buffer, sizeof(error_buffer));
	return std::string(error_buffer);
}

// SSLContextManager implementation
SSLContextManager::SSLContextManager()
{
	initializeSSL();
}

SSLContextManager::~SSLContextManager()
{
	_contexts.clear();
	cleanupSSL();
}

void SSLContextManager::initializeSSL()
{
	if (_initialized)
		return;

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
	_initialized = true;
}

void SSLContextManager::cleanupSSL()
{
	if (!_initialized)
		return;

	EVP_cleanup();
	ERR_free_strings();
	_initialized = false;
}

bool SSLContextManager::createContext(const std::string &id, const std::string &cert_path,
									   const std::string &key_path)
{
	if (_contexts.find(id) != _contexts.end())
	{
		std::cerr << "Context with ID '" << id << "' already exists" << std::endl;
		return false;
	}

	auto ctx = std::make_shared<SSLContext>(cert_path, key_path);
	if (!ctx->isValid())
	{
		std::cerr << "Failed to create SSL context for ID '" << id << "'" << std::endl;
		return false;
	}

	_contexts[id] = ctx;
	return true;
}

SSL_CTX *SSLContextManager::getContext(const std::string &id) const
{
	auto it = _contexts.find(id);
	if (it == _contexts.end())
		return nullptr;
	return it->second->getCtx();
}

bool SSLContextManager::hasContext(const std::string &id) const
{
	return _contexts.find(id) != _contexts.end();
}
