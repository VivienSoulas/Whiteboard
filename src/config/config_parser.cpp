#include "config/config_parser.hpp"
#include "http/http_method.hpp"
#include "logger.hpp"
#include <fstream>

ConfigParser::ConfigParser(const std::string& content) : StringParser(content)
{
}

ConfigParser::~ConfigParser()
{
	// Trivial destructor - base class handles cleanup
}

Config ConfigParser::parse()
{
	Config config;

	while (!this->isAtEnd())
	{
		while (!this->isAtEnd() && this->isSemicolon())
		{
			this->skipSemicolon();
		}

		if (this->isAtEnd())
		{
			break;
		}

		if (this->expectToken("server"))
		{
			ServerConfig server;
			this->parseServerBlock(server);
			config.addServerConfig(server);
		}
		else
		{
			std::string token = this->peekToken().value;
			throw std::runtime_error(this->formatError("Unexpected token: " + token + " (expected 'server')"));
		}
	}

	return config;
}

void ConfigParser::parseLocationDirectives(LocationConfig& location)
{
	while (!this->isAtEnd() && !this->peekToken().value.empty() && this->peekToken().value != "}")
	{
		if (this->expectToken("methods"))
		{
			std::vector<std::string> method_strings = this->parseStringList();
			std::vector<HttpMethod> method_enums;
			for (size_t i = 0; i < method_strings.size(); ++i)
			{
				method_enums.push_back(parseHttpMethod(method_strings[i]));
			}
			location.setMethods(method_enums);
			this->skipSemicolon();
		}
		else if (this->expectToken("index"))
		{
			location.index = this->parseString();
			this->skipSemicolon();
		}
		else if (this->expectToken("directory_listing"))
		{
			location.directory_listing = this->parseBool();
			this->skipSemicolon();
		}
		else if (this->expectToken("upload_dir"))
		{
			location.upload_dir = this->parseString();
			this->skipSemicolon();
		}
		else if (this->expectToken("cgi_extension"))
		{
			location.cgi_extension = this->parseString();
			this->skipSemicolon();
		}
		else if (this->expectToken("return"))
		{
			location.redirect_code = this->parseInt();
			location.redirect_path = this->parseString();
			this->skipSemicolon();
		}
		else if (this->expectToken("max_body_size"))
		{
			location.max_body_size = this->parseSize();
			this->skipSemicolon();
		}
		else if (this->expectToken("root"))
		{
			location.root = this->parseString();
			this->skipSemicolon();
		}
		else
		{
			std::string directive = this->peekToken().value;
			throw std::runtime_error(this->formatError("Unknown location directive: " + directive));
		}
	}
}

void ConfigParser::parseLocationBlock(ServerConfig& server)
{
	LocationConfig location;

	// "location" token already consumed by caller
	location.path = this->parseString();
	DEBUG_LOG("Parsing location block: " << location.path);
	this->expectBlockStart();
	
	this->parseLocationDirectives(location);
	
	this->expectBlockEnd();
	server.locations.push_back(location);
}

void ConfigParser::parseServerBlock(ServerConfig& server)
{
	DEBUG_LOG("Parsing server block");
	// "server" token already consumed by caller
	this->expectBlockStart();

	while (!this->isAtEnd() && !this->peekToken().value.empty() && this->peekToken().value != "}")
	{
		if (this->expectToken("listen"))
		{
			std::string listen_str = this->parseString();
			std::string interface = "0.0.0.0";
			std::string port;

			std::pair<std::string, std::string> host_port = string_parser::splitOnce(listen_str, ':');
			if (!host_port.second.empty())
			{
				interface = host_port.first;
				port = host_port.second;
			}
			else
			{
				port = host_port.first;
			}

			if (port.empty())
			{
				throw std::runtime_error(this->formatError("Invalid listen directive: " + listen_str));
			}

			server.listen.push_back(std::make_pair(interface, port));
			this->skipSemicolon();
		}
		else if (this->expectToken("server_name"))
		{
			server.server_name = this->parseString();
			this->skipSemicolon();
		}
		else if (this->expectToken("max_body_size"))
		{
			server.max_body_size = this->parseSize();
			this->skipSemicolon();
		}
		else if (this->expectToken("error_page"))
		{
			std::vector<int> error_codes;
			// Parse error codes until we find a path (starts with /)
			while (!this->isAtEnd() && !this->isBlockEnd() && !this->isSemicolon())
			{
				std::string token_value = this->peekToken().value;
				// Check if this looks like a path (starts with /)
				if (!token_value.empty() && token_value[0] == '/')
				{
					break;
				}
				int code = this->parseInt();
				if (code < 400 || code > 599)
				{
					throw std::runtime_error(this->formatError("Invalid error code: " + token_value + " (must be 400-599)"));
				}
				error_codes.push_back(code);
			}
			
			if (error_codes.empty())
			{
				throw std::runtime_error(this->formatError("Expected at least one error code"));
			}
			
			std::string error_path = this->parseString();
			for (size_t i = 0; i < error_codes.size(); ++i)
			{
				server.error_pages[error_codes[i]] = error_path;
			}
			this->skipSemicolon();
		}
		else if (this->expectToken("root"))
		{
			server.root = this->parseString();
			this->skipSemicolon();
		}
		else if (this->expectToken("location"))
		{
			this->parseLocationBlock(server);
		}
		else if (this->expectToken("ssl_certificate"))
		{
			server.ssl_certificate_path = this->parseString();
			server.ssl_enabled = true;
			this->skipSemicolon();
		}
		else if (this->expectToken("ssl_certificate_key"))
		{
			server.ssl_certificate_key_path = this->parseString();
			this->skipSemicolon();
		}
		else if (this->expectToken("ssl_port"))
		{
			server.ssl_port = this->parseString();
			this->skipSemicolon();
		}
		else
		{
			std::string directive = this->peekToken().value;
			throw std::runtime_error(this->formatError("Unknown server directive: " + directive));
		}
	}

	this->expectBlockEnd();

	if (server.listen.empty())
	{
		throw std::runtime_error(this->formatError("Server block must have at least one 'listen' directive"));
	}

	// Validate SSL configuration if enabled
	if (server.ssl_enabled)
	{
		if (server.ssl_certificate_path.empty())
		{
			throw std::runtime_error(this->formatError("ssl_certificate not specified but SSL is enabled"));
		}
		if (server.ssl_certificate_key_path.empty())
		{
			throw std::runtime_error(this->formatError("ssl_certificate_key not specified but SSL is enabled"));
		}

		// Check if certificate file exists
		std::ifstream cert_file(server.ssl_certificate_path.c_str());
		if (!cert_file.good())
		{
			throw std::runtime_error(this->formatError("ssl_certificate file not found: " + server.ssl_certificate_path));
		}

		// Check if key file exists
		std::ifstream key_file(server.ssl_certificate_key_path.c_str());
		if (!key_file.good())
		{
			throw std::runtime_error(this->formatError("ssl_certificate_key file not found: " + server.ssl_certificate_key_path));
		}
	}

	// Validate root directory
	if (!server.root.empty())
	{
		std::ifstream root_check(server.root.c_str());
		if (!root_check.good())
		{
			throw std::runtime_error(this->formatError("root directory not found: " + server.root));
		}
	}
}
