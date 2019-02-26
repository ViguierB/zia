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
	
	static inline char const
		*mainConfigPath = (zany::isWindows ? ".\\config.zia" : "/etc/zia/config.zia");
	
	static inline char const
		*parserPath = (zany::isWindows ? ".\\modules\\parser.dll" : "/etc/zia/modules/libparser.so");
	
	static inline const std::uint16_t
		defThreadNbr = 8;

	static inline const zany::Entity
		defConfig = zany::makeObject{
			{ "listen", { 80, 443 } },
			{ "server", zany::makeArray{} },
			{ "workdir", (zany::isWindows ? ".\\" : "/etc/zia/") }
		};

};