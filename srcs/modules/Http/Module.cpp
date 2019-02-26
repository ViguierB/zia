/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <boost/algorithm/string.hpp>
#include <sstream>
#include "Zany/Loader.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Orchestrator.hpp"

namespace zia {

class HttpModule : public zany::Loader::AbstractModule {
public:
	virtual auto	name() const -> const std::string&
		{ static const std::string name("Http"); return name; }
	virtual void	init() final;
private:
	static inline void	_beforeHandleRequest(zany::Pipeline::Instance &i);
	static inline void	_onHandleRequest(zany::Pipeline::Instance &i);
	inline void			_afterHandleRequest(zany::Pipeline::Instance &i);
	static inline void	_onHandleResponse(zany::Pipeline::Instance &i);
	static inline auto	_getMethodeFromString(std::string &method);
};

void	HttpModule::init() {
	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::BEFORE_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::HIGH>(&HttpModule::_beforeHandleRequest);

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::HIGH>(&HttpModule::_onHandleRequest);

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::LOW>([this] (auto &i) { _afterHandleRequest(i); });

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_RESPONSE>()
		.addTask<zany::Pipeline::Priority::HIGH>(&HttpModule::_onHandleResponse);
}

auto HttpModule::_getMethodeFromString(std::string &method) {
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

void	HttpModule::_beforeHandleRequest(zany::Pipeline::Instance &i) {
	i.response.status = 200;
}

void	HttpModule::_onHandleRequest(zany::Pipeline::Instance &i) {
	std::string	line;

	std::getline(i.connection->stream(), line);
	{
		line.erase(--line.end());
		std::istringstream stm(line);
		
		std::string	str;
		std::getline(stm, str, ' ');

		i.request.method = _getMethodeFromString(str);
		std::getline(stm, str, ' ');
		i.request.path = str;

		std::getline(stm, str, ' ');

		std::istringstream pstm(str);
		std::string		proto;
		
		std::getline(pstm, proto, '/');
		i.request.protocol = proto;
		std::getline(pstm, proto, '/');
		i.request.protocolVersion = proto;

		i.response.protocol = i.request.protocol;
		i.response.protocolVersion = i.request.protocolVersion;
	}

	while (std::getline(i.connection->stream(), line) 
	&& line != "" && line != "\r") {
		line.erase(--line.end());
		std::istringstream	splitor(line);
		std::string			key;

		std::getline(splitor, key, ':');
		auto &value = *i.request.headers[key];
		std::getline(splitor, value);
		boost::trim(value);

		if (boost::to_lower_copy(key) == "host") {
			std::istringstream	splitor(value);
			std::string			port;
			
			std::getline(splitor, i.request.host, ':');
			std::getline(splitor, port);

			i.request.port = static_cast<int>(zany::HttpHeader(port).getNumber());
		}
	}
}

void	HttpModule::_afterHandleRequest(zany::Pipeline::Instance &i) {
	auto	found = false;

	for (auto &srv: master->getConfig()["server"].value<zany::Array>()) {
		if (srv["host"] == i.request.host && srv["port"] == i.request.port) {
			i.serverConfig = srv.clone();
			found = true;
			break;
		}
	}
	if (!found) {
		i.response.status = 404;
	}
}

void	HttpModule::_onHandleResponse(zany::Pipeline::Instance &i) {
	auto &resp = i.response;
	auto &stm = i.connection->stream();
	
	stm << resp.protocol << '/' << resp.protocolVersion
		<< ' ' << resp.status << ' ' << "OK";
	std::cout << resp.protocol << '/' << resp.protocolVersion
		<< ' ' << resp.status << ' ' << "OK" << "\n";
	
	for (auto &h: resp.headers) {
		stm << "\r\n" << h.first << ": " << *h.second;
	}
	stm << "\r\n";
}
	
}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::HttpModule();
}