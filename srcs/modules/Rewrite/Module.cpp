/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include "Zany/Loader.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Orchestrator.hpp"

namespace zia {

class RewriteModule : public zany::Loader::AbstractModule {
public:
	virtual auto name() const -> const std::string & {
		static const std::string name("Rewrite");
		return name;
	}

	virtual void init() final;

private:
	inline void _onHandleRequest(zany::Pipeline::Instance &i);

	inline void _onDataReady(zany::Pipeline::Instance &i);

	inline void _onHandleResponse(zany::Pipeline::Instance &i);

	inline boost::process::environment _fillCgiEnv(zany::Pipeline::Instance &i);

private:
	zany::Entity	conf;
};


void RewriteModule::init() {
	this->conf = this->master->getConfig().clone();
}

inline void RewriteModule::_onHandleRequest(zany::Pipeline::Instance &i) {
	int pos;
	try {
		for (pos = 0; conf["server"][pos]["host"] != i.request.host && conf["server"][pos]["port"] != i.request.port; ++pos);
	} catch (...) {
		return;
	}

	auto &vec = conf["server"][pos]["if"].value<zany::Array>();
	for (auto it = vec.begin(); it < vec.end(); ++it) {
		for( auto &obj : it->value<zany::Object>()) {
			if (!(obj.first == "path" && obj.second.isString() && obj.second.value<zany::String>() == i.request.path))
				return;
		}
	}

	std::unordered_map<std::string, std::string> var;
	vec = conf["server"][pos]["var"].value<zany::Array>();
	for (auto it = vec.begin(); it < vec.end(); ++it) {
		var[it->to<std::string>()];
	}

}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::RewriteModule();
}