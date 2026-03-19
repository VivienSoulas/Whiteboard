#include "webserv.hpp"
#include "config/config_file.hpp"
#include "path/path_utils.hpp"
#include "http/multipart/multipart_parser.hpp"
#include "http/static_file_handler.hpp"
#include "http/upload_handler.hpp"
#include "http/cgi_handler.hpp"
#include "io/connection_manager.hpp"
#include "io/ssl_context.hpp"
#include "io/tls_socket.hpp"
#include <dirent.h>
#include <algorithm>
#include <unistd.h>
#include <limits.h>
#include <sstream>
#include "string/string_utils.hpp"
#include "main.hpp"
#include "logger.hpp"

// Static member initialization
SSLContextManager Webserv::_ssl_manager;

Webserv::Webserv(const std::string &config_path)
	: _config_file(config_path),
	  _router(_config_file.getConfig()),
	  _connection_manager(_router, std::bind(&Webserv::handleRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
{
	DEBUG_LOG("Webserv initialized with config file: " << config_path);
	SSLContextManager::initializeSSL();
	_config_file.getConfig().print();
	setupListeners();
}

Webserv::~Webserv() {}

void Webserv::accept()
{
	_connection_manager.pollConnections();
}

void Webserv::setupListeners()
{
	const Config &config = _config_file.getConfig();
	const std::vector<ServerConfig> &servers = config.getServerConfigs();
	typedef std::set<std::pair<std::string, std::string>> ListenSet;
	ListenSet seen;

	for (std::vector<ServerConfig>::const_iterator s = servers.begin(); s != servers.end(); ++s)
	{
		// Setup regular HTTP listeners
		for (size_t i = 0; i < s->listen.size(); ++i)
		{
			const std::string &addr = s->listen[i].first;
			const std::string &port = s->listen[i].second;
			std::pair<std::string, std::string> key(addr, port);
			if (seen.find(key) != seen.end())
				continue;
			seen.insert(key);
			_connection_manager.addListener(new Socket(port, addr), addr, port);
			DEBUG_LOG("Added HTTP listener on " << addr << ":" << port);
		}

		// Setup HTTPS listeners if SSL is enabled
		if (s->ssl_enabled && !s->ssl_certificate_path.empty() && !s->ssl_certificate_key_path.empty())
		{
			std::string ssl_port = s->ssl_port;
			if (ssl_port.empty())
				ssl_port = "443";

			std::string ssl_addr = "0.0.0.0";
			for (size_t i = 0; i < s->listen.size(); ++i)
			{
				ssl_addr = s->listen[i].first;
				break;
			}

			std::string context_id = ssl_addr + ":" + ssl_port;
			if (!_ssl_manager.createContext(context_id, s->ssl_certificate_path, s->ssl_certificate_key_path))
			{
				std::cerr << "Failed to create SSL context for " << context_id << std::endl;
				continue;
			}

			SSL_CTX *ssl_ctx = _ssl_manager.getContext(context_id);
			if (!ssl_ctx)
			{
				std::cerr << "Failed to retrieve SSL context for " << context_id << std::endl;
				continue;
			}

			try
			{
				TlsSocket *tls_socket = new TlsSocket(ssl_port, ssl_addr, ssl_ctx);
				_connection_manager.addTlsListener(tls_socket, ssl_addr, ssl_port);
				DEBUG_LOG("Added HTTPS listener on " << ssl_addr << ":" << ssl_port);
			}
			catch (const std::exception &e)
			{
				std::cerr << "Failed to create HTTPS listener: " << e.what() << std::endl;
			}
		}
	}
}

std::vector<std::string> Webserv::locationMethodsToAllow(const LocationConfig &loc)
{
	std::vector<std::string> out;
	for (std::set<HttpMethod>::const_iterator it = loc.methods.begin(); it != loc.methods.end(); ++it)
	{
		if (*it != UNKNOWN)
			out.push_back(httpMethodToString(*it));
	}
	return out;
}

std::string Webserv::handleRequest(const HttpRequest &req, const std::string &listen_addr,
								   const std::string &listen_port)
{
	std::string host;
	{
		HttpRequest::HeaderMap::const_iterator it = req.headers.find("host");
		if (it != req.headers.end() && !it->second.empty())
			host = it->second;
	}

	const ServerConfig *server = _router.selectServer(host, listen_addr, listen_port);
	if (!server)
	{
		HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::INTERNAL_SERVER_ERROR, true);
		return res.serialize();
	}
	DEBUG_LOG("Routing request: host [" << host << "] matched server [" << server->server_name << "]");

	if (req.method == UNKNOWN)
	{
		HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::METHOD_NOT_ALLOWED, true);
		return res.serialize();
	}
	HttpMethod method = req.method;
	if (req.version == HttpRequest::UNKNOWN_VERSION)
	{
		DEBUG_LOG("Final response: 505 Version Not Supported");
		HttpResponse res = HttpResponseFactory::buildVersionNotSupported(true);
		return res.serialize();
	}

	const LocationConfig *location = _router.matchLocation(*server, req.path);
	if (!location)
	{
		HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::NOT_FOUND, true);
		return res.serialize();
	}

	// Redirect: handle before method check so return-only locations (no methods directive) still respond to GET/HEAD
	if (location->redirect_code > 0 && !location->redirect_path.empty())
	{
		HttpMethod m = req.method;
		if (m == GET || m == HEAD)
		{
			HttpResponse res = HttpResponseFactory::buildRedirect(req, location->redirect_code, location->redirect_path, true);
			return res.serialize();
		}
		// Other methods on redirect-only location
		std::vector<std::string> allow_redirect;
		allow_redirect.push_back("GET");
		allow_redirect.push_back("HEAD");
		HttpResponse res = HttpResponseFactory::buildMethodNotAllowed(req, allow_redirect, true);
		DEBUG_LOG("Final response: 405 Method Not Allowed (redirecting to " << location->redirect_path << ")");
		return res.serialize();
	}

	if (location->methods.find(method) == location->methods.end())
	{
		DEBUG_LOG("Final response: 405 Method Not Allowed (" << httpMethodToString(method) << " not in location)");
		HttpResponse res = HttpResponseFactory::buildMethodNotAllowed(req, locationMethodsToAllow(*location), true);
		return res.serialize();
	}

	// CGI execution based on file extension
	if (!location->cgi_extension.empty() && string_utils::endsWith(req.path, location->cgi_extension))
	{
		DEBUG_LOG("Handing off request to CGI handler");
		return cgi_handler::handle(*server, *location, req);
	}

	// POST to upload_dir: parse body and save file(s)
	if (req.method == POST && !location->upload_dir.empty())
	{
		DEBUG_LOG("Handing off request to upload handler");
		return upload_handler::handle(*location, req);
	}

	// DELETE specific resource
	if (req.method == DELETE)
	{
		std::string file_path = path_utils::resolveFilePath(location->root.empty() ? server->root : location->root, 
                                                            location->root.empty() ? req.path : req.path.substr(location->path.length() == 1 ? 0 : location->path.length()));
		if (file_path.empty())
		{
			HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::NOT_FOUND, true);
			return res.serialize();
		}
		file_path = path_utils::normalizeAndMakeAbsolute("", file_path);
		if (access(file_path.c_str(), F_OK) == 0)
		{
			if (std::remove(file_path.c_str()) == 0)
			{
				DEBUG_LOG("Final response: 204 No Content (DELETE success)");
				HttpResponse res;
				res.setStatus(HttpStatus::NO_CONTENT);
				res.setShouldClose(true);
				return res.serialize();
			}
			else
			{
				DEBUG_LOG("Final response: 403 Forbidden (DELETE failed: " << strerror(errno) << ")");
				HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::FORBIDDEN, true);
				return res.serialize();
			}
		}
		else
		{
			HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::NOT_FOUND, true);
			return res.serialize();
		}
	}

	// Only serve static files for GET/HEAD
	if (req.method == GET || req.method == HEAD)
	{
		bool head_only = (req.method == HEAD);
		std::string static_response = static_file_handler::serve(*server, *location, req.path, head_only);
		if (!static_response.empty()) {
			DEBUG_LOG("Final response: served static file/listing");
			return static_response;
		}
		if (head_only)
		{
			HttpResponse res;
			res.setStatus(HttpStatus::OK);
			res.setBody("");
			res.setShouldClose(false);
			return res.serialize(); 
		}
	}

	if(req.method == POST)
	{
		HttpResponse res = HttpResponseFactory::buildOk(req, "", "text/html; charset=utf-8", true);
		return res.serialize();
	}
	
	DEBUG_LOG("Final response: 404 Not Found (no handler matched)");
	HttpResponse res = HttpResponseFactory::buildError(req, HttpStatus::NOT_FOUND, true); 
	return res.serialize();
}
