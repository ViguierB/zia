/*
** EPITECH PROJECT, 2018
** zia
** File description:
** ModulesUtils.hpp
*/

#pragma once

#include <ctime>
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <boost/optional.hpp>
#include "Zany/Entity.hpp"
#include "Zany/HttpBase.hpp"

namespace zia {

struct ModuleUtils {
private:
	static inline bool match(char const *s1, char const *s2) {
		if (*s2 == '*')
			return (*s1 ? match(s1 + 1, s2) || match(s1, s2 + 1) : match(s1, s2 + 1));
		if (*s1 == *s2)
				return (*s1 ? match(s1 + 1, s2 + 1) : true);
		return false;
	}
public:
	static inline auto getMethodFromString(std::string &method) {
		if (method == "GET") {
			return zany::HttpRequest::RequestMethods::GET;
		} else if (method == "POST") {
			return zany::HttpRequest::RequestMethods::POST;
		} else if (method == "HEAD") {
			return zany::HttpRequest::RequestMethods::HEAD;
		} else if (method == "OPTION") {
			return zany::HttpRequest::RequestMethods::OPTIONS;
		} else if (method == "PATCH") {
			return zany::HttpRequest::RequestMethods::PATCH;
		} else if (method == "CONNECT") {
			return zany::HttpRequest::RequestMethods::CONNECT;
		} else if (method == "PUT") {
			return zany::HttpRequest::RequestMethods::PUT;
		} else if (method == "TRACE") {
			return zany::HttpRequest::RequestMethods::TRACE;
		} else if (method == "DELETE") {
			return zany::HttpRequest::RequestMethods::DELETE;
		}
		return zany::HttpRequest::RequestMethods::ERROR;
	}

	static inline auto getTimeout(int seconds) {
#		if defined(ZANY_ISWINDOWS)
			return (DWORD) seconds * 1000;
#		else
			struct timeval tv { seconds, 0 };
			return tv;
#		endif
	}

	template<typename SourceT, typename TargetT>
	static inline void	copyByChunck(SourceT &&source, TargetT &&target) {
		std::size_t	contentlen;
		std::string line;
		try {
			char		*end;
			getline(source, line);
			
			contentlen = std::strtol(line.c_str(), &end, 16);
		} catch (...) { contentlen = 0; }

		target << line << "\n";

		if (contentlen == 0)
			return;
		
		contentlen += 2;
		thread_local char	buffer[1024];
		std::streamsize		sread;

		while (contentlen > 0) {
			sread = source.rdbuf()->sgetn(
				buffer,
				sizeof(buffer) <= contentlen
					? sizeof(buffer)
					: contentlen
			);
			target.write(buffer, sread);
			contentlen -= sread;
		}
		copyByChunck(source, target);
	}

	template<typename SourceT, typename TargetT>
	static inline void	copyByLength(SourceT &&source, TargetT &&target, std::size_t contentlen) {
		thread_local char	buffer[2048];
		std::streamsize		sread;

		while (contentlen > 0) {
			sread = source.rdbuf()->sgetn(
				buffer,
				sizeof(buffer) <= contentlen
					? sizeof(buffer)
					: contentlen
			);
			target.write(buffer, sread);
			contentlen -= sread;
		}
		target.flush();
	}

	template<typename ForwardIt>
	static inline ForwardIt getBestMatch(ForwardIt beg, ForwardIt end, std::string const &str) {
		std::vector<ForwardIt> matches;

		for (;beg != end; ++beg) {
			if (match(str.data(), beg->data()))
				matches.emplace_back(beg);
		}
		if (matches.size() == 0)
			return end;
		return *std::max_element(matches.begin(), matches.end(), [](auto &&s1, auto &&s2) {
			return s1->length() < s2->length();
		});
	}

	using ForwardIT = std::vector<zany::Entity>::iterator;
	static inline boost::optional<zany::Entity> getBestEntity(ForwardIT beg, ForwardIT end, std::string const &str) {
		std::vector<ForwardIT> matches;

		for (;beg != end; ++beg) {
			if (match(str.data(), (*beg)["host"].value<zany::String>().data()))
				matches.emplace_back(beg);
		}
		if (matches.size() == 0)
			return boost::optional<zany::Entity>{};

		zany::Entity mergedEntity = zany::makeObject{};

		std::stable_sort(matches.begin(), matches.end(), [] (ForwardIT e1, ForwardIT e2) {
			return (*e1)["host"].value<zany::String>().length() < (*e2)["host"].value<zany::String>().length();
		});
		for (auto const &e : matches) {
			mergedEntity.merge(*e, [] (std::string const &key, auto &first, auto &second) {
				first = second;
			});
		}
		return mergedEntity;
	}
};

}