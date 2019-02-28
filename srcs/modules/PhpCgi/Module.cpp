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

class PhpCgiModule : public zany::Loader::AbstractModule {
public:
	virtual auto	name() const -> const std::string&
		{ static const std::string name("PhpCgi"); return name; }
	virtual void	init() final;
private:
	inline void		_onHandleRequest(zany::Pipeline::Instance &i);
	inline void		_onDataReady(zany::Pipeline::Instance &i);
	inline void		_onHandleResponse(zany::Pipeline::Instance &i);

	inline boost::process::environment		_fillCgiEnv(zany::Pipeline::Instance &i);
};

void	PhpCgiModule::init() {
	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&PhpCgiModule::_onHandleRequest, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_DATA_READY>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&PhpCgiModule::_onDataReady, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_RESPONSE>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&PhpCgiModule::_onHandleResponse, this, std::placeholders::_1));
}

boost::process::environment		PhpCgiModule::_fillCgiEnv(zany::Pipeline::Instance &inst) {
	constexpr std::pair<char const *, char const *> map[] = {
		{ "CONTENT_LENGTH", "content-length" },
		{ "CONTENT_TYPE", "content-type" }
	};
	constexpr auto size = sizeof(map) / sizeof(*map);

	auto env = boost::this_process::environment();

	env["SERVER_SOFTWARE"] = "Zia Server -- PCinc";
	env["SERVER_PROTOCOL"] = inst.request.protocol + "/" + inst.request.protocolVersion;
	env["SERVER_PORT"] = std::to_string(inst.request.port);
	env["SERVER_NAME"] = inst.request.host;
	env["REQUEST_METHOD"] = inst.request.methodString;
	env["QUERY_STRING"] = inst.request.fullQuery;
	env["REDIRECT_STATUS"] = "CGI";
	env["SCRIPT_FILENAME"] = boost::filesystem::current_path().string() + "/" + inst.request.path;

	for (std::size_t i = 0; i < size; ++i) {
		auto it = inst.request.headers.find(map[i].second);
		if (it == inst.request.headers.end()) continue;
		env[map[i].first] = *it->second;
	}
	return env;
}

void	PhpCgiModule::_onHandleRequest(zany::Pipeline::Instance &i) {
	if (i.request.path.length() < 5 || i.request.path.substr(i.request.path.length() - 4, 4) != ".php")
		return;

	std::string cgiPath =
#	if defined(ZANY_ISWINDOWS)
		"tools/php-cgi.exe"
#	else
		"tools/php-cgi"
#	endif
	;
	i.writerID = getUniqueId();
	auto &ins = (i.properties["php_cin"] = zany::Property::make<boost::process::opstream>()).get<boost::process::opstream>();
	auto &outs = (i.properties["php_cout"] = zany::Property::make<boost::process::ipstream>()).get<boost::process::ipstream>();
	auto &errs = (i.properties["php_cerr"] = zany::Property::make<boost::process::ipstream>()).get<boost::process::ipstream>();
	i.properties["php_cgi"] = zany::Property::make<boost::process::child>(
		cgiPath,
		i.request.path,
		boost::process::std_in < ins,
		boost::process::std_out > outs,
		boost::process::std_err > errs,
		_fillCgiEnv(i)
	);

}

void	PhpCgiModule::_onDataReady(zany::Pipeline::Instance &i) {
	if (i.writerID != getUniqueId() || i.request.method != zany::HttpRequest::RequestMethods::POST)
		return;
	auto it = i.request.headers.find("content-length");
	if (it == i.request.headers.end() && it->second.isNumber()) {
		i.response.status = 400;
		i.writerID = 0;
		return;
	}

	thread_local char	buffer[1024];
	std::size_t			contentlen = static_cast<std::size_t>(it->second.getNumber());
	std::streamsize		sread;
	auto				&phpostream = i.properties["php_cin"].get<boost::process::opstream>();


	while (contentlen > 0) {
		sread = i.connection->stream().readsome(
			buffer,
			sizeof(buffer) <= contentlen
				? sizeof(buffer)
				: contentlen
		);
		
		phpostream.write(buffer, sread);
		contentlen -= sread;
	}
	phpostream.flush();
}

void	PhpCgiModule::_onHandleResponse(zany::Pipeline::Instance &i) {
	if (i.writerID != getUniqueId())
		return;

	auto &stm = i.properties["php_cout"].get<boost::process::ipstream>();

	i.connection->stream() << stm.rdbuf();
}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::PhpCgiModule();
}