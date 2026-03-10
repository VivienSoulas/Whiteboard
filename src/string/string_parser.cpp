#include "string/string_parser.hpp"
#include <cctype>

namespace string_parser
{
	std::string trim(const std::string &s)
	{
		std::string::size_type start = 0;
		while (start < s.size() && (s[start] == ' ' || s[start] == '\t'))
			++start;
		std::string::size_type end = s.size();
		while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
			--end;
		return s.substr(start, end - start);
	}

	std::string toLower(const std::string &s)
	{
		std::string out;
		out.reserve(s.size());
		for (std::string::size_type i = 0; i < s.size(); ++i)
		{
			unsigned char c = static_cast<unsigned char>(s[i]);
			out.push_back(static_cast<char>(std::tolower(c)));
		}
		return out;
	}

	std::pair<std::string, std::string> splitOnce(const std::string &s, char delim)
	{
		std::string::size_type pos = s.find(delim);
		if (pos == std::string::npos)
			return std::make_pair(s, std::string());
		return std::make_pair(s.substr(0, pos), s.substr(pos + 1));
	}

	std::vector<std::string> split(const std::string &s, char delim)
	{
		std::vector<std::string> result;
		std::string::size_type start = 0;
		for (;;)
		{
			std::string::size_type pos = s.find(delim, start);
			if (pos == std::string::npos)
			{
				result.push_back(s.substr(start));
				break;
			}
			result.push_back(s.substr(start, pos - start));
			start = pos + 1;
		}
		return result;
	}
}

Token::Token() : value(""), line(0)
{
}

Token::Token(const std::string &val, size_t ln) : value(val), line(ln)
{
}

StringParser::StringParser(const std::string &content)
	: _tokens(std::vector<Token>()), _position(0)
{
	std::vector<Token> tokens = this->tokenize(content);
	this->setTokens(tokens);
}

StringParser::~StringParser()
{
}

std::vector<Token> StringParser::tokenize(const std::string &content)
{
	std::vector<Token> tokens;
	std::string current;
	bool in_quotes = false;
	size_t line = 1;

	for (size_t i = 0; i < content.length(); ++i)
	{
		char c = content[i];

		if (c == '\n')
		{
			line++;
		}

		if (c == '"' || c == '\'')
		{
			in_quotes = !in_quotes;
			if (!in_quotes && !current.empty())
			{
				tokens.push_back(Token(current, line));
				current.clear();
			}
		}
		else if (in_quotes)
		{
			current += c;
		}
		else if (std::isspace(c))
		{
			if (!current.empty())
			{
				tokens.push_back(Token(current, line));
				current.clear();
			}
		}
		else if (c == ';' || c == '{' || c == '}')
		{
			if (!current.empty())
			{
				tokens.push_back(Token(current, line));
				current.clear();
			}
			tokens.push_back(Token(std::string(1, c), line));
		}
		else
		{
			current += c;
		}
	}

	if (!current.empty())
	{
		tokens.push_back(Token(current, line));
	}

	return tokens;
}

void StringParser::setTokens(const std::vector<Token> &tokens)
{
	this->_tokens = tokens;
	this->_position = 0;
}

bool StringParser::isAtEnd() const
{
	return this->_position >= this->_tokens.size();
}

Token StringParser::peekToken() const
{
	if (this->_position >= this->_tokens.size())
	{
		return Token("", 0);
	}
	return this->_tokens[this->_position];
}

Token StringParser::currentToken() const
{
	if (this->_position == 0 || this->_position > this->_tokens.size())
	{
		return Token("", 0);
	}
	return this->_tokens[this->_position - 1];
}

size_t StringParser::getCurrentLine() const
{
	if (this->_position > 0 && this->_position <= this->_tokens.size())
	{
		return this->_tokens[this->_position - 1].line;
	}
	if (!this->_tokens.empty() && this->_position < this->_tokens.size())
	{
		return this->_tokens[this->_position].line;
	}
	return 1;
}

bool StringParser::expectToken(const std::string &expected)
{
	if (this->isAtEnd())
	{
		return false;
	}

	if (this->_tokens[this->_position].value == expected)
	{
		this->_position++;
		return true;
	}
	return false;
}

void StringParser::requireToken(const std::string &expected)
{
	if (!this->expectToken(expected))
	{
		std::string msg = "Expected '" + expected + "'";
		this->throwError(msg);
	}
}

void StringParser::expectBlockStart()
{
	this->requireToken("{");
}

void StringParser::expectBlockEnd()
{
	this->requireToken("}");
}

void StringParser::expectSemicolon()
{
	this->requireToken(";");
}

void StringParser::skipSemicolon()
{
	if (this->isSemicolon())
	{
		this->_position++;
	}
}

std::string StringParser::parseString()
{
	if (this->isAtEnd() || this->isBlockEnd() || this->isSemicolon())
	{
		this->throwError("Expected string value");
	}

	std::string value = this->_tokens[this->_position].value;
	this->_position++;
	return value;
}

int StringParser::parseInt()
{
	if (this->isAtEnd() || this->isBlockEnd() || this->isSemicolon())
	{
		this->throwError("Expected integer value");
	}

	std::string value_str = this->_tokens[this->_position].value;
	this->_position++;

	std::istringstream iss(value_str);
	int value;
	iss >> value;
	if (iss.fail() || !iss.eof())
	{
		this->throwError("Invalid integer value: " + value_str);
	}

	return value;
}

bool StringParser::parseBool()
{
	if (this->isAtEnd() || this->isBlockEnd() || this->isSemicolon())
	{
		this->throwError("Expected boolean value");
	}

	std::string value_str = this->_tokens[this->_position].value;
	this->_position++;

	// Convert to lowercase for comparison
	std::string lower_value;
	for (size_t i = 0; i < value_str.length(); ++i)
	{
		lower_value += static_cast<char>(std::tolower(value_str[i]));
	}

	if (lower_value == "on" || lower_value == "true" || lower_value == "1")
	{
		return true;
	}
	else if (lower_value == "off" || lower_value == "false" || lower_value == "0")
	{
		return false;
	}
	else
	{
		this->throwError("Invalid boolean value: " + value_str + " (expected 'on'/'off' or 'true'/'false')");
		return false; // Never reached, but satisfies compiler
	}
}

size_t StringParser::parseSize()
{
	if (this->isAtEnd() || this->isBlockEnd() || this->isSemicolon())
	{
		this->throwError("Expected size value");
	}

	std::string size_str = this->_tokens[this->_position].value;
	this->_position++;

	if (size_str.empty())
	{
		this->throwError("Empty size string");
	}

	size_t multiplier = 1;
	std::string num_str = size_str;
	std::string unit;

	// Extract number and unit
	size_t i = 0;
	while (i < size_str.length() && std::isdigit(size_str[i]))
	{
		++i;
	}
	if (i < size_str.length())
	{
		num_str = size_str.substr(0, i);
		unit = size_str.substr(i);
	}

	// Convert to lowercase for comparison
	std::string lower_unit;
	for (size_t j = 0; j < unit.length(); ++j)
	{
		lower_unit += static_cast<char>(std::tolower(unit[j]));
	}

	if (lower_unit == "k" || lower_unit == "kb")
	{
		multiplier = 1024;
	}
	else if (lower_unit == "m" || lower_unit == "mb")
	{
		multiplier = 1024 * 1024;
	}
	else if (lower_unit == "g" || lower_unit == "gb")
	{
		multiplier = 1024 * 1024 * 1024;
	}
	else if (!lower_unit.empty())
	{
		this->throwError("Invalid size unit: " + unit);
	}

	std::istringstream iss(num_str);
	size_t value;
	iss >> value;
	if (iss.fail())
	{
		this->throwError("Invalid size value: " + num_str);
	}

	return value * multiplier;
}

std::vector<std::string> StringParser::parseStringList()
{
	std::vector<std::string> result;

	while (!this->isAtEnd() && !this->isBlockEnd() && !this->isSemicolon())
	{
		result.push_back(this->parseString());
	}

	return result;
}

std::vector<int> StringParser::parseIntList()
{
	std::vector<int> result;

	while (!this->isAtEnd() && !this->isBlockEnd() && !this->isSemicolon())
	{
		result.push_back(this->parseInt());
	}

	return result;
}

std::string StringParser::formatError(const std::string &message) const
{
	size_t line = this->getCurrentLine();
	std::ostringstream oss;
	oss << "line " << line << ": " << message;
	return oss.str();
}

void StringParser::throwError(const std::string &message) const
{
	throw std::runtime_error(this->formatError(message));
}

bool StringParser::isBlockEnd() const
{
	return !this->isAtEnd() && this->_tokens[this->_position].value == "}";
}

bool StringParser::isSemicolon() const
{
	return !this->isAtEnd() && this->_tokens[this->_position].value == ";";
}
