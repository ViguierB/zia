/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <fstream>
#include <thread>
#include <memory>
#include <iostream>
#include <openssl/sha.h>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/filesystem.hpp>
#include "Zany/Orchestrator.hpp"
#include "Zany/Loader.hpp"
#include "../common/ModulesUtils.hpp"
#include "../common/Json/Parser.hpp"

namespace zia {
namespace manager {

class ManagerModule : public zany::Loader::AbstractModule {
public:
	virtual auto	name() const -> const std::string&
		{ static const std::string name("Manager"); return name; }
	virtual void	init() final;
private:
	inline void		_onHandleRequest(zany::Pipeline::Instance &i);
	inline void		_onDataReady(zany::Pipeline::Instance &i);
	inline void		_onHandleResponse(zany::Pipeline::Instance &i);

	inline void		_counter(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result);
	inline void		_load(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result);
	inline void		_refresh(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result);
	inline void		_unload(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result);
	inline void		_list(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result);
	inline void		_execute(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result);
	
	static inline std::string	_sha256String(std::string &&);

	template<typename Stream>
	static inline Stream	&_sendJson(Stream &s, json::Entity &&response) {
		std::ostringstream sstm;

		sstm << response;

		auto str = sstm.str();

		s << "content-length: " << str.length() << "\r\n\r\n" << str;

		return s;
	}

	std::atomic<std::size_t>	_requestCounter = 0;
};

std::string ManagerModule::_sha256String(std::string &&in)
{
	std::array<unsigned char, SHA256_DIGEST_LENGTH>	res;
	std::stringstream								os;

	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, in.c_str(), in.length());
	SHA256_Final(res.data(), &sha256);

	for (auto e: res) {
		os << std::hex << std::setw(2) << std::setfill('0') << +e;
	}
	return os.str();
}

void	ManagerModule::init() {
	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&ManagerModule::_onHandleRequest, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_DATA_READY>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&ManagerModule::_onDataReady, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_RESPONSE>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&ManagerModule::_onHandleResponse, this, std::placeholders::_1));
}



void	ManagerModule::_onHandleRequest(zany::Pipeline::Instance &i) {
	_requestCounter++;

	if (i.serverConfig["manager"].isObject()
	&& i.serverConfig["manager"]["password"].isString()
	&& *i.request.headers["manager-api"] == "true") {
		i.writerID = this->getUniqueId();
// 		Access-Control-Allow-Origin: https://foo.bar.org
// Access-Control-Allow-Methods: POST, GET
		
	}
}

void	ManagerModule::_onDataReady(zany::Pipeline::Instance &i) {
	if (i.writerID == this->getUniqueId()) {
		if (i.request.method == zany::HttpRequest::RequestMethods::POST
		&& i.request.headers["content-length"].isNumber()) {
			std::ostringstream data;

			ModuleUtils::copyByLength(i.connection->stream(), data, i.request.headers["content-length"].getNumber());

			auto command = json::Parser::fromString(data.str());

			try {
				if (i.serverConfig["manager"]["password"].value<zany::String>() == command["password"].value<json::String>()) {
					i.properties["manager-command"] = zany::Property::make<json::Entity>(command);

					if (i.response.headers.find("content-length") != i.response.headers.end())
						i.response.headers.erase(i.response.headers.find("content-length"));

					json::Entity result;
					
					_execute(i, command, result);

					if (!result.isNull()) {
						std::ostringstream stm;
						stm << result;

						auto strres = (i.properties["manager-result"] = zany::Property::make<std::string>(stm.str())).get<std::string>();

						i.response.headers["content-length"] = std::to_string(strres.length());

					}
					return;
				} else {
					i.response.status = 401;
					return;
				}
			} catch (...) {}
		}
		i.response.status = 400;
	}
}

void	ManagerModule::_onHandleResponse(zany::Pipeline::Instance &i) {
	if (i.writerID == this->getUniqueId()) {
		if (i.properties.find("manager-result") == i.properties.end()) return;

		auto	&result = i.properties["manager-result"].get<std::string>();

		i.connection->stream() << "\r\n" << result;
	}
}

void	ManagerModule::_unload(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result) {
	auto &moduleName = command["module-name"].value<json::String>();

	zany::Loader::AbstractModule *toRemove = nullptr;

	for (auto &module : const_cast<zany::Loader &>(this->master->getLoader())) {
		if (module.name() == moduleName) {
			toRemove = &module;
			break;
		}
	}
	auto connection = i.connection;
	if (!toRemove) {
		this->master->getContext().addTask([connection, moduleName] {
			_sendJson(connection->stream(), json::makeObject {
				{ "status", "fail" },
				{ "command", "unload" },
				{ "module-name", moduleName },
				{ "message", "Module " + moduleName + " not found." }
			});
		});
	} else {
		this->master->unloadModule(*toRemove,
			[connection, moduleName] {
				_sendJson(connection->stream(), json::makeObject {
					{ "status", "success" },
					{ "command", "unload" },
					{ "module-name", moduleName }
				});
			},
			[connection, moduleName] (zany::Loader::Exception e) {
				_sendJson(connection->stream(), json::makeObject {
					{ "status", "fail" },
					{ "command", "unload" },
					{ "module-name", moduleName },
					{ "message", e.what() }
				});
			});
	}
}

void	ManagerModule::_load(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result) {
	boost::filesystem::path path(command["module-path"].value<json::String>());
	
	bool found = false;
	auto connection = i.connection;

	if ((boost::filesystem::is_regular_file(path) || boost::filesystem::is_symlink(path))
		&& path.extension() ==
#if defined(ZANY_ISWINDOWS)
			".dll"
#else
			".so"
#endif
		) {
		auto mp =
#if defined(ZANY_ISWINDOWS)
			path.lexically_normal();
#else
			boost::filesystem::path(
				(boost::filesystem::is_symlink(path)
					? boost::filesystem::read_symlink(path)
					: path
				).string()
			).lexically_normal()
#endif
		;
		this->master->loadModule(path.generic_string(),
		[connection, path] (auto &module) {
			_sendJson(connection->stream(), json::makeObject {
				{ "status", "success" },
				{ "command", "load" },
				{ "module-path", path.string() },
				{ "module-name", module.name() }
			});
		}, [connection, path] (auto error) {
			_sendJson(connection->stream(), json::makeObject {
				{ "status", "fail" },
				{ "command", "load" },
				{ "module-path", path.string() },
				{ "message", error.what() }
			});
		});
		found = true;
	}
	if (!found) {
		this->master->getContext().addTask([connection, path] {
			_sendJson(connection->stream(), json::makeObject {
				{ "status", "fail" },
				{ "command", "load" },
				{ "module-path", path.string() },
				{ "message", "Module not found." }
			});
		});
	}
}

void	ManagerModule::_list(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result) {
	result = json::Entity::ARR;
	for (auto &m: const_cast<zany::Loader&>(this->master->getLoader())) {
		json::Entity mdl( json::makeObject {
			{ "name", m.name() },
			{ "badges", json::Entity::ARR }
		} );
		if (m.isACoreModule()) {
			mdl["badges"].push("core");
		}
		if (m.isAParser()) {
			mdl["badges"].push("parser");
		}
		if (m.getUniqueId() == getUniqueId()) {
			mdl["badges"].push("me :)");
		}

		result.push(mdl);
	}
}

void	ManagerModule::_counter(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result) {
	result = json::Entity(json::makeObject {
		{ "count", (long) _requestCounter.load() }
	});
}

void	ManagerModule::_refresh(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result) {
	auto connection = i.connection;
#	if defined(ZANY_ISUNIX)
		if ((void*)(this->master->*(&zany::Orchestrator::reload)) == (void*)(&zany::Orchestrator::reload)) {
			this->master->getContext().addTask([connection] {
				_sendJson(connection->stream(), json::makeObject {
					{ "status", "fail" },
					{ "command", "refresh" },
					{ "message", "Refresh not implemented on this server" }
				});
			});
			return;
		}
#	else
		// disabled feature on windows, because of MSVC...
#	endif
	this->master->getContext().addTask([connection] {
		_sendJson(connection->stream(), json::makeObject {
			{ "status", "success" },
			{ "command", "refresh" }
		});
	});
	this->master->reload();
}

void	ManagerModule::_execute(zany::Pipeline::Instance &i, json::Entity &command, json::Entity &result) {
	try {
		if (command["command"] == "list") {
			_list(i, command, result);
		} else if (command["command"] == "unload") {
			_unload(i, command, result);
		} else if (command["command"] == "load") {
			_load(i, command, result);
		} else if (command["command"] == "refresh") {
			_refresh(i, command, result);
		} else if (command["command"] == "counter") {
			_counter(i, command, result);
		}
	} catch (...) {}
}

}
}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::manager::ManagerModule();
}