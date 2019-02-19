/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <fstream>
#include "Zany/Loader.hpp"
#include "./Parser.hpp"

namespace zia {

class JsonParserModule : public zany::Loader::AbstractModule {
public:
	virtual auto	name() const -> const std::string&
		{ static const std::string name("JsonParserModule"); return name; }
	virtual void	init() {};
	virtual bool	isAParser() final { return true; }
	virtual auto	parse(std::string const &filename) -> zany::Entity final;
private:

};

zany::Entity	JsonParserModule::parse(std::string const &filename) {
	std::ifstream	sConfig(filename);

	if (!sConfig.is_open()) {
		return false;
	}
	try {
		return Parser::fromStream(sConfig);
	} catch (...) {
		return false;
	}
}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::JsonParserModule();
}