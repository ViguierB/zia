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

	private:
		zany::Entity	conf;
	};


	void RewriteModule::init() {
		this->conf = this->master->getConfig().clone();
		garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
			.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&RewriteModule::_onHandleRequest, this, std::placeholders::_1));
	}

	inline void RewriteModule::_onHandleRequest(zany::Pipeline::Instance &i) {
		int pos;
		try {
			for (pos = 0; conf["server"][pos]["host"] != i.request.host && conf["server"][pos]["port"] != i.request.port; ++pos);
		} catch (...) {
			return;
		}

		auto &rewrites = conf["server"][pos]["rewrite"].value<zany::Array>();
		for (auto &rewrite : rewrites) {
			if (false) {
				next:
				continue;
			}

			auto &vec = rewrite["if"].value<zany::Array>();
			for (auto it = vec.begin(); it < vec.end(); ++it) {
				for( auto &obj : it->value<zany::Object>()) {
					if (!(obj.first == "path" && obj.second.isString() && obj.second.value<zany::String>() == i.request.path))
						goto next;
				}
			}

			std::unordered_map<std::string, std::string> var;
			vec = rewrite["var"].value<zany::Array>();
			for (auto it = vec.begin(); it < vec.end(); ++it) {
				var[it->to<std::string>()];
			}

			vec = rewrite["set"].value<zany::Array>();
			for (auto it = vec.begin(); it < vec.end(); ++it) {
				for( auto &obj : it->value<zany::Object>()) {
					if (var.find(obj.first) != var.end())
						var[obj.first] = obj.second.value<zany::String>();
				}
			}

			for (auto &item : var) {
				item.second.insert(0, 1, '$');
			}

			i.request.path = rewrite["rule"].value<zany::String>();
			std::size_t posV;
			for (auto &item : var) {
				if ((posV = i.request.path.find(item.first)) != std::string::npos)
					item.second.replace(posV, item.first.size(), item.second);
			}
		}
	}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::RewriteModule();
}