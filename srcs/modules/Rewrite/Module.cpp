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
		inline void _beforeHandleRequest(zany::Pipeline::Instance &i);

	private:
		zany::Entity	conf;
	};


	void RewriteModule::init() {
		this->master->getContext().addTask([this] {
			this->conf = this->master->getConfig().clone();
		});
		garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::BEFORE_HANDLE_REQUEST>()
			.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&RewriteModule::_beforeHandleRequest, this, std::placeholders::_1));
	}

	inline void RewriteModule::_beforeHandleRequest(zany::Pipeline::Instance &i) {
		auto &config = i.serverConfig;

		if (config.isNull() || !config["rewrite"].isArray()) return;

		auto &rewrites = config["rewrite"].value<zany::Array>();
		for (auto &rewrite : rewrites) {

			if (false) {
				next:

				continue;
			}

			auto &vec = rewrite["if"].value<zany::Array>();
			for (auto it = vec.begin(); it < vec.end(); ++it) {
				for( auto &obj : it->value<zany::Object>()) {
					if (!(obj.first == "path" && i.properties.find("basepath") != i.properties.end() && i.properties["basepath"].get<std::string>() == obj.second.value<zany::String>())) {
						goto next;
					}
				}
			}

			std::unordered_map<std::string, std::string> var;
			vec = rewrite["var"].value<zany::Array>();
			for (auto it = vec.begin(); it < vec.end(); ++it) {
				var["$" + it->to<std::string>()];
			}

			vec = rewrite["set"].value<zany::Array>();
			for (auto it = vec.begin(); it < vec.end(); ++it) {
				for( auto &obj : it->value<zany::Object>()) {
					if (var.find("$" + obj.first) != var.end())
						var["$" + obj.first] = obj.second.value<zany::String>();
				}
			}

			std::string str = rewrite["rule"].value<zany::String>();
			std::size_t posV;
			for (auto &item : var) {
				if ((posV = str.find(item.first)) != std::string::npos)
					str.replace(posV, item.first.size(), item.second);
			}

			if (i.request.path.find(i.properties["basepath"].get<std::string>()) != std::string::npos)
				i.request.path.replace(	i.request.path.find(i.properties["basepath"].get<std::string>()), i.properties["basepath"].get<std::string>().size(), str);

			std::cout << i.response.status << std::endl;
		}
	}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::RewriteModule();
}