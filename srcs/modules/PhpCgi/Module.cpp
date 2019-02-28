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
};

void	PhpCgiModule::init() {
	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&PhpCgiModule::_onHandleRequest, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_DATA_READY>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&PhpCgiModule::_onDataReady, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_RESPONSE>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&PhpCgiModule::_onHandleResponse, this, std::placeholders::_1));
}

void	PhpCgiModule::_onHandleRequest(zany::Pipeline::Instance &i) {
	if (i.request.path.length() < 5 || i.request.path.substr(i.request.path.length() - 4, 4) != ".php")
		return;

	std::string cgiPath =
		#if defined(ZANY_ISWINDOWS)
			"tools/php-cgi.exe"
		#else
			"tools/php-cgi"
		#endif
			;
	i.writerID = getUniqueId();
	auto &is = (i.properties["php stream"] = zany::Property::make<boost ::process::ipstream>()).get<boost::process::ipstream>();
	i.properties["php child"] = zany::Property::make<boost::process::child>(cgiPath, i.request.path, boost::process::std_out > is);
}

void	PhpCgiModule::_onDataReady(zany::Pipeline::Instance &i) {
	if (i.writerID != getUniqueId() || i.request.method != zany::HttpRequest::RequestMethods::POST)
		return;
	i.request.headers.find("content_length");
}

void	PhpCgiModule::_onHandleResponse(zany::Pipeline::Instance &i) {
	if (i.writerID != getUniqueId())
		return;

	auto &stm = i.properties["php stream"].get<boost::process::ipstream>();

	i.connection->stream() << stm.rdbuf();
}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::PhpCgiModule();
}