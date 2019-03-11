/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <benjamin.viguier@epitech.eu> wrote this file. As long as you retain this 
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in
 * return Benjamin Viguier
 * --------------------------------------------------------------------------
 */

#pragma once

#include <istream>
#include <exception>
#include "Entity.hpp"

#if defined _MSC_VER
 #define TOLOWER ::tolower
 #define ISPRINT ::isprint
#else
 #define TOLOWER std::tolower
 #define ISPRINT std::isprint
#endif

namespace json
{
	class Parser
	{
	public:
		explicit Parser(std::istream &stm);
		~Parser(void);
		void		parse(void);
		Entity	get(void);

		static Entity fromString(std::string const &json);
		static Entity fromFile(std::string const &file);
		static Entity fromStream(std::istream &stm);
	private:
		using Pmember = Entity	(Parser::*)(void);

		Pmember	_getUsefulMember(void) const;
		Entity	_getNumber(void);
		Entity	_getObj(void);
		Entity	_getArr(void);
		Entity	_getStr(void);
		Entity	_getBool(void);
		Entity	_getNull(void);

		std::string	__extractStr(void);
		inline char	__peek() const;
	private:
		struct Counter {
			std::size_t	line = 1;
			std::size_t	lineOffset = 0;
			std::size_t	offset = 0;
		}	_count;

		Entity	_obj;
		std::istream	&_stm;
		std::ios_base::fmtflags
				_ff;
	public:
		struct ParserException : public std::exception
		{
		public:
			struct Pos {
				std::size_t	line = 0;
				std::size_t	col = 0;
			};
		public:
			ParserException(Parser::Counter const &count, std::istream &stm);
			ParserException(std::string const &custom);

			const char	*what() const throw();
			Pos		getErrorPos() const;
		private:
			std::string	_msg;
			Pos		_pos;
		};
	};
	std::istream	&operator>>(std::istream &from, Entity &me);
}