/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <benjamin.viguier@epitech.eu> wrote this file. As long as you retain this 
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in
 * return Benjamin Viguier
 * --------------------------------------------------------------------------
 */

#include <sstream>
#include <fstream>
#include <vector>
#include <cstring>
#include <functional>
#include "./Parser.hpp"

namespace zia {

inline char Parser::__peek() const {
	char	c;

	while ((c = _stm.peek()) && c &&
		(c == '\n' || c == ' ' || c == '\t')) {
		const_cast<Counter&>(_count).offset++;
		if (c == '\n') {
			const_cast<Counter&>(_count).line++;
			const_cast<Counter&>(_count).lineOffset = const_cast<Counter&>(_count).offset;
		}
		_stm >> c;
	}
	const_cast<Counter&>(_count).offset++;		
	return c;
}

std::istream	&operator>>(std::istream &from, zany::Entity &me) {
	me = Parser::fromStream(from);
	return (from);
}

Parser::Parser(std::istream &stm):
	_stm(stm),
	_ff(_stm.flags()) {
	_stm.unsetf(std::ios::skipws);
}

Parser::~Parser(void) {
	_stm.flags(_ff);
}

zany::Entity Parser::fromString(std::string const &json) {
	std::istringstream	stm(json);

	return Parser::fromStream(stm);
}

zany::Entity Parser::fromFile(std::string const &file) {
	std::ifstream	stm(file);

	if (!stm.is_open())
		throw ParserException("'" + file + "': Unable to open file");
	return Parser::fromStream(stm);
}

zany::Entity Parser::fromStream(std::istream &stm) {
	Parser	parser(stm);

	parser.parse();
	return parser.get();
}

Parser::Pmember	Parser::_getUsefulMember(void) const {
	char	c;
	struct MemberIdent{
		char const	*ident;
		zany::Entity	(Parser::*member)(void);
	};
	static std::vector<struct MemberIdent> tab = {
		{"\"'", &Parser::_getStr},
		{"{", &Parser::_getObj},
		{"[", &Parser::_getArr},
		{"tfTF", &Parser::_getBool},
		{"0123456789-.", &Parser::_getNumber},
		{"nN", &Parser::_getNull}
	};

	while ((c = __peek()) && c && c == ',')
		_stm >> c;
	if ((c = __peek()) && c) {
		for (auto &a : tab)
			if (std::strchr(a.ident, c)) {
				return a.member;
			}
	}
	if (c)
		throw ParserException(_count, _stm);
	return nullptr;
}

void	Parser::parse(void) {
	Pmember	parseMember = _getUsefulMember();
	
	_obj = (this->*parseMember)();
}

zany::Entity Parser::_getNumber(void) {
	double	value;

	(void) _stm.gcount();
	_stm >> value;
	_count.offset += static_cast<std::size_t>(_stm.gcount());
	return zany::Entity(value);
}

zany::Entity Parser::_getObj(void) {
	char	c;
	zany::Entity	res(zany::Entity::Type::OBJ);

	auto extractKey = [this, &c] (std::string &key) -> bool {
		if (c == '"' || c == '\'') {
			key = __extractStr();
			return true;
		}
		return false;
	};

	_stm >> c;
	while ((c = __peek()) && c && c != '}') {
		std::string	key;

		if (extractKey(key) == false)
			throw ParserException(_count, _stm);

		c = __peek();
		_stm >> c;
		if (c != ':')
			throw ParserException(_count, _stm);
		res[key] = (this->*(_getUsefulMember()))();
		while ((c == __peek()) && c && c == ',')
			_stm >> c;
		_stm >> c;
		if (c == '}')
			break;
		
	}
	return res;
}

zany::Entity Parser::_getArr(void) {
	char	c;
	zany::Entity	res(zany::Entity::Type::ARR);

	_stm >> c;
	while ((c = __peek()) && c && c != ']') {
		auto fct = _getUsefulMember();
		
		res.push((this->*(fct))());
	}
	_stm >> c;		
	return (res);
}

zany::Entity Parser::_getStr(void) {
	return (zany::Entity(__extractStr()));
}

zany::Entity Parser::_getBool(void) {
	std::ostringstream	sValue;
	char			c;
	int			max = sizeof("false");

	while (_stm >> c && c && max) {
		sValue << c;

		_count.offset++;
		std::string toLower = sValue.str();
		for (std::size_t i = 0; i < toLower.size(); i++)
			toLower[i] = TOLOWER(toLower[i]);
		if (toLower == "true" ||
			toLower == "false")
			break;
		max--;
	}
	if (!max)
		throw ParserException(_count, _stm);
	return zany::Entity(sValue.str() == "true");
}

zany::Entity Parser::_getNull(void) {
	std::ostringstream	sValue;
	char			c;
	int			max = sizeof("null");

	while (_stm >> c && c && max) {
		sValue << c;
		_count.offset++;
		std::string toLower = sValue.str();
		for (std::size_t i = 0; i < toLower.size(); i++)
			toLower[i] = TOLOWER(toLower[i]);
		if (toLower == "null")
			break;
		max--;
	}
	if (!max)
		throw ParserException(_count, _stm);
	return zany::Entity(zany::Entity::Type::NUL);
}

std::string	Parser::__extractStr(void) {
	std::ostringstream	sValue;
	char			c;
	char			endKey;
	bool			escape = false;

	_stm >> endKey;			
	while (_stm >> c && (c != endKey || escape)) {
		_count.offset++;
		if (c == '\n') {
			_count.line++;
			_count.lineOffset = _count.offset;
		}
		if (escape)
			escape = !escape;
		else if (c == '\\') {
			escape = !escape;
			continue;
		}
		sValue << c;
	}
	return sValue.str();
}

zany::Entity	Parser::get(void) {
	return _obj;
}

Parser::ParserException::ParserException(std::string const &custom):
	_msg("Parser: Error: " + custom)
{}

Parser::ParserException::ParserException(Parser::Counter const &count, std::istream &stm):
	_pos({count.line, count.offset - count.lineOffset}) {
	std::ostringstream	_sstm;
	unsigned char		c =  stm.peek();

	_sstm << "Parser: Error: "
		<< "(Ln " << _pos.line << ": Col "
		<< _pos.col << "): "
		<< "Unexpected charater: '";
	if (ISPRINT(c)) {
		_sstm << c;
	} else if ((char) c == -1) {
		_sstm << "EOF";
	} else {
		_sstm << "0x" << std::hex << +c;
	}
	_sstm << '\'';
	_msg = _sstm.str();
}

const char	*Parser::ParserException::what() const throw() {
	return _msg.c_str();
}

}
