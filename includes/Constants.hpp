/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Constants.hpp
*/

#pragma once

#include <vector>
#include <cstdint>
#include "Zany/Platform.hpp"
#include "Zany/Entity.hpp"

struct constant {
	
#if defined(NDEBUG)
	static inline char const
		*defWorkDir = (zany::isWindows ? ".\\" : "/etc/zia/");
#else
	static inline char const
		*defWorkDir = (zany::isWindows ? ".\\tests" : "./tests");
#endif

	static inline char const
		*mainConfigPath = (zany::isWindows ? "config.zia" : "config.zia");
	
	static inline const std::uint16_t
		defThreadNbr = 8;

	static inline const zany::Entity
		defConfig = zany::makeObject{
			{ "listen", { 80, 443 } },
			{ "server", zany::makeArray{} }
		};

	static inline const auto *defSslProtocol = "ALL,-TLSv1";

};