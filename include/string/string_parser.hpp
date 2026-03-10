#ifndef STRING_PARSER_HPP
#define STRING_PARSER_HPP

#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <stdexcept>

namespace string_parser
{
	std::string trim(const std::string &s);
	std::string toLower(const std::string &s);
	/** Splits at first occurrence of \a delim; if not found, returns (s, ""). */
	std::pair<std::string, std::string> splitOnce(const std::string &s, char delim);
	/** Splits into segments by \a delim (empty segments between consecutive delims are included). */
	std::vector<std::string> split(const std::string &s, char delim);
}

struct Token
{
	std::string value;
	size_t line;

	Token();
	Token(const std::string &val, size_t ln);
};

class StringParser
{
public:
	StringParser(const std::string &content);
	virtual ~StringParser();

	std::vector<Token> tokenize(const std::string &content);
	void setTokens(const std::vector<Token> &tokens);

	std::string formatError(const std::string &message) const;

protected:
	bool isAtEnd() const;
	Token peekToken() const;
	Token currentToken() const;
	size_t getCurrentLine() const;

	bool expectToken(const std::string &expected);
	void requireToken(const std::string &expected);
	void expectBlockStart();
	void expectBlockEnd();
	void expectSemicolon();
	void skipSemicolon(); // Optional semicolon
	bool isBlockEnd() const;
	bool isSemicolon() const;

	std::string parseString();
	int parseInt();
	bool parseBool();
	size_t parseSize();
	std::vector<std::string> parseStringList();
	std::vector<int> parseIntList();

private:
	std::vector<Token> _tokens;
	size_t _position;

	void throwError(const std::string &message) const;
};

#endif
