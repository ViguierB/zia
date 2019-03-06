/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include "Zany/Loader.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Orchestrator.hpp"
#include "../common/ModulesUtils.hpp"

namespace zia {

struct VHostConfig {
	std::string		host;
	std::uint16_t	port;

	struct {
		std::string		host;
		std::uint16_t	port;
		std::string		scheme;
	}				target;
};


class ReverseProxyModule : public zany::Loader::AbstractModule {
public:
	virtual auto	name() const -> const std::string&
		{ static const std::string name("ReverseProxy"); return name; }
	virtual void	init() final;
private:
	inline void		_ready();
	inline void		_onHandleRequest(zany::Pipeline::Instance &i);
	inline void		_onDataReady(zany::Pipeline::Instance &i);
	inline void		_onHandleResponse(zany::Pipeline::Instance &i);

	std::unordered_map<std::string, VHostConfig>	_vhosts;
};

void	ReverseProxyModule::init() {
	master->getContext().addTask(std::bind(&ReverseProxyModule::_ready, this));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::LOW>(std::bind(&ReverseProxyModule::_onHandleRequest, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_DATA_READY>()
		.addTask<zany::Pipeline::Priority::LOW>(std::bind(&ReverseProxyModule::_onDataReady, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_RESPONSE>()
		.addTask<zany::Pipeline::Priority::LOW>(std::bind(&ReverseProxyModule::_onHandleResponse, this, std::placeholders::_1));
}

void	ReverseProxyModule::_ready() {
	auto cfg = master->getConfig();

	try {
		auto &vHosts = cfg["server"].value<zany::Array>();

		for (auto &vhost: vHosts) {
			try {
				auto it = vhost.value<zany::Object>().find("reverse-proxy");
				if (it == vhost.value<zany::Object>().end())
					continue;

				auto &rpcfg = it->second;
				_vhosts.emplace(
					std::pair<std::string, VHostConfig>(
						vhost["host"].value<zany::String>(),
						VHostConfig {
							vhost["host"].value<zany::String>(),
							(std::uint16_t)vhost["port"].to<int>(),
							{
								rpcfg["host"].value<zany::String>(),
								(std::uint16_t)rpcfg["port"].to<int>(),
								rpcfg["scheme"].value<zany::String>()
							}
						}
					)
				);
			} catch (std::exception const &e) {
				std::cerr << e.what() << std::endl;
			}
		}
	} catch (...) {}
}

void	ReverseProxyModule::_onHandleRequest(zany::Pipeline::Instance &i) {
	auto vhit = _vhosts.find(i.request.host);

	if (vhit == _vhosts.end()) { return; } 
	if (vhit->second.port != i.request.port) { return; }

	auto &vh = vhit->second;

	i.writerID = getUniqueId();
	i.response.status = 200;
	
	auto &stream = (i.properties["reverse-proxy_socket"] = zany::Property::make<boost::asio::ip::tcp::iostream>()).get<boost::asio::ip::tcp::iostream>();
	stream.connect(vh.target.host, std::to_string(vh.target.port));
	if (!stream) {
		i.writerID = 0;
		i.response.status = 500;
	} 

	stream << i.request.methodString << " " << boost::filesystem::path(i.request.path).lexically_normal().string() << " " << i.request.protocol << "/" << i.request.protocolVersion << "\r\n";
	for (auto it = i.request.headers.begin(); it != i.request.headers.end(); ++it) {
		if (it->first == "host") {
			stream << "host=" << vh.target.host << ":" << vh.target.port << "\r\n";
		} else {
			stream << it->first << "=" << *it->second << "\r\n";
		}
	}
	stream << "\r\n";
	stream.flush();
}

void	ReverseProxyModule::_onDataReady(zany::Pipeline::Instance &i) {
	if (i.writerID != getUniqueId()) return;
	std::string	line;
	auto 		&stream = i.properties["reverse-proxy_socket"].get<boost::asio::ip::tcp::iostream>();

	stream >> line >> i.response.status;
	std::getline(stream, line);
	
	while (std::getline(stream, line) 
	&& line != "" && line != "\r") {
		line.erase(--line.end());
		std::istringstream	splitor(line);
		std::string			key;

		std::getline(splitor, key, ':');
		boost::to_lower(key);
		auto &value = *i.response.headers[key];
		std::getline(splitor, value);
		boost::trim(value);
	}
}

template<typename SourceT, typename TargetT>
inline void	passChunck(SourceT &&source, TargetT &&target) {
	std::size_t	contentlen;
	try {
		source >> std::hex >> contentlen >> std::dec;
	} catch (...) { contentlen = 0; }

	std::cout << contentlen << std::endl;

	target << std::hex << contentlen << std::dec << "\r\n";

	if (contentlen == 0)
		return;

	source >> std::ws;
	
	thread_local char	buffer[1024];
	std::streamsize		sread;

	while (contentlen > 0) {
		sread = source.rdbuf()->sgetn(
			buffer,
			sizeof(buffer) <= contentlen
				? sizeof(buffer)
				: contentlen
		);
		target.write(buffer, sread);
		contentlen -= sread;
		std::cout << contentlen << std::endl;
	}
	passChunck(source, target);
}

void	ReverseProxyModule::_onHandleResponse(zany::Pipeline::Instance &i) {
	if (i.writerID != getUniqueId()) return;
	auto 		&stream = i.properties["reverse-proxy_socket"].get<boost::asio::ip::tcp::iostream>();
	
	if (*i.response.headers["transfer-encoding"] == "chunked") {
		i.connection->stream() << "\r\n";
		passChunck(stream, i.connection->stream());
	} else {
		i.connection->stream() << stream.rdbuf();
	}
	i.connection->stream().flush();
	i.connection->stream().sync();
}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::ReverseProxyModule();
}