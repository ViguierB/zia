/*
** EPITECH PROJECT, 2018
** zia
** File description:
** ModulesUtils.hpp
*/

#pragma once

#include <ctime>
#include "Zany/HttpBase.hpp"

namespace zia {

struct ModuleUtils {
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

		std::cout << contentlen << std::endl;

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
	}
};

}