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
#include <stdlib.h>
#include "Zany/Loader.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Orchestrator.hpp"
#include "../common/ModulesUtils.hpp"
#include "../common/NetStream.hpp"

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
	ReverseProxyModule();
	~ReverseProxyModule();

	virtual auto	name() const -> const std::string&
		{ static const std::string name("ReverseProxy"); return name; }
	virtual void	init() final;
private:
	inline void		_ready();
	inline void		_onHandleRequest(zany::Pipeline::Instance &i);
	inline void		_onDataReady(zany::Pipeline::Instance &i);
	inline void		_onHandleResponse(zany::Pipeline::Instance &i);

	std::unordered_map<std::string, VHostConfig>	_vhosts;
	using SslStreamBuf = boost::iostreams::stream_buffer<SslTcpBidirectionalIoStream<boost::asio::detail::socket_type>>;
};

void	ReverseProxyModule::init() {
	master->getContext().addTask(std::bind(&ReverseProxyModule::_ready, this));
	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::HIGH>(std::bind(&ReverseProxyModule::_onHandleRequest, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_DATA_READY>()
		.addTask<zany::Pipeline::Priority::LOW>(std::bind(&ReverseProxyModule::_onDataReady, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_RESPONSE>()
		.addTask<zany::Pipeline::Priority::LOW>(std::bind(&ReverseProxyModule::_onHandleResponse, this, std::placeholders::_1));
}

ReverseProxyModule::ReverseProxyModule() { 
	SSL_library_init();
	OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();
	ERR_load_crypto_strings();
}

ReverseProxyModule::~ReverseProxyModule() {
	FIPS_mode_set(0);
	ENGINE_cleanup();
	CONF_modules_unload(1);
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
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
	
	auto &ios = (i.properties["reverse-proxy_ioservice"] = zany::Property::make<boost::asio::io_service>()).get<boost::asio::io_service>();
	auto &socket = (i.properties["reverse-proxy_socket"] = zany::Property::make<boost::asio::ip::tcp::socket>(ios)).get<boost::asio::ip::tcp::socket>();
	
	boost::asio::ip::tcp::resolver 			resolver(ios);
	boost::asio::ip::tcp::resolver::query	query(vh.target.host, std::to_string(vh.target.port));
	auto 									endpoint_iterator = resolver.resolve(query);
	decltype(endpoint_iterator)				end;

	boost::system::error_code error = boost::asio::error::host_not_found;
	while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
	if (error) {
		i.writerID = 0;
		i.response.status = 500;
		return;
	} 

	auto &streambuf = (i.properties["reverse-proxy_socket_stream_buf"] = zany::Property::make<SslStreamBuf>(
		socket.native_handle(),
		((vh.target.scheme == "https") ? true : false)
	)).get<SslStreamBuf>();
	auto &stream = (i.properties["reverse-proxy_stream"] = zany::Property::make<std::iostream>(&streambuf)).get<std::iostream>();

	stream << i.request.methodString << " " << i.properties["basepath"].get<std::string>()  << " " << i.request.protocol << "/" << i.request.protocolVersion << "\r\n";
	
	for (auto it = i.request.headers.begin(); it != i.request.headers.end();) {
		if (it->first == "host") {
			stream << "host: " << vh.target.host << ":" << vh.target.port << "\r\n";
		} else if (it->first == "connection") {
		} else {
			stream << it->first << ": " << *it->second << "\r\n";
		}
		++it;
	}
	stream << "\r\n";
	stream.flush();

	i.properties["reverse-proxy-enabled"] = zany::Property::make<bool>(true);
}

void	ReverseProxyModule::_onDataReady(zany::Pipeline::Instance &i) {
	if (i.writerID != getUniqueId()) return;

	auto 		&stream = i.properties["reverse-proxy_stream"].get<std::iostream>();
	std::string	line;

	if (i.request.headers["content-length"].isNumber()) {
		ModuleUtils::copyByLength(i.connection->stream(), stream, i.request.headers["content-length"].getNumber());
	} else if (*i.request.headers["transfer-encoding"] == "chunked") {
		ModuleUtils::copyByChunck(i.connection->stream(), stream);
		stream << "\r\n";
	}

	stream >> line >> i.response.status;
	std::getline(stream, line);

	while (std::getline(stream, line) 
	&& line != "" && line != "\r") {
		line.erase(--line.end());
		std::istringstream	splitor(line);
		std::string			key;

		std::getline(splitor, key, ':');
		boost::to_lower(key);
		if (key != "connection") {
			auto &value = *i.response.headers[key];
			std::getline(splitor, value);
			boost::trim(value);
		}
	}
}

void	ReverseProxyModule::_onHandleResponse(zany::Pipeline::Instance &i) {
	if (i.writerID != getUniqueId()) return;
	auto 		&stream = i.properties["reverse-proxy_stream"].get<std::iostream>();
	
	if (i.response.headers["content-length"].isNumber()) {
		i.connection->stream() << "\r\n";
		ModuleUtils::copyByLength(stream, i.connection->stream(), i.response.headers["content-length"].getNumber());
	} else if (*i.response.headers["transfer-encoding"] == "chunked") {
		i.connection->stream() << "\r\n";
		ModuleUtils::copyByChunck(stream, i.connection->stream());
		i.connection->stream() << "\r\n";
	} else {
		i.connection->stream() << "\r\n";
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