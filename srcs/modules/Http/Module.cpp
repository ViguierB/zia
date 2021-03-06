/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include "Zany/Loader.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Orchestrator.hpp"
#include "../common/ModulesUtils.hpp"

namespace zia {

class HttpModule : public zany::Loader::AbstractModule {
public:
	virtual auto	name() const -> const std::string&
		{ static const std::string name("Http"); return name; }
	virtual void	init() final;
private:
	inline void			_beforeHandleRequest(zany::Pipeline::Instance &i);
	inline void			_onHandleRequest(zany::Pipeline::Instance &i);
	inline void			_beforeHandleResponse(zany::Pipeline::Instance &i);
	inline void			_onHandleResponse(zany::Pipeline::Instance &i);
	static inline auto	_getMethodeFromString(std::string &method);
	
	static inline auto	_getReasonPhrase(int code) -> const std::string&;
	inline bool			_isAllowedExt(boost::filesystem::path const &p, zany::Pipeline::Instance const &i) const;
	static inline void	_parsePath(zany::Pipeline::Instance &i, std::string const &path);
	static inline bool	_urlDecode(const std::string& in, std::string& out);
	inline void			_setHeaders(zany::Pipeline::Instance &i);
};

void	HttpModule::init() {
	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::BEFORE_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::HIGH>(std::bind(&HttpModule::_beforeHandleRequest, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::HIGH>(std::bind(&HttpModule::_onHandleRequest, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::BEFORE_HANDLE_RESPONSE>()
		.addTask<zany::Pipeline::Priority::HIGH>(std::bind(&HttpModule::_beforeHandleResponse, this, std::placeholders::_1));

	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_RESPONSE>()
		.addTask<zany::Pipeline::Priority::HIGH>(std::bind(&HttpModule::_onHandleResponse, this, std::placeholders::_1));
}

const std::string	&HttpModule::_getReasonPhrase(int code) {
	static const std::unordered_map<int, std::string> map {
		{ 100, "Continue" },
		{ 101, "Switching Protocols" },
		{ 200, "OK" },
		{ 201, "Created" },
		{ 202, "Accepted" },
		{ 203, "Non-Authoritative Information" },
		{ 204, "No Content" },
		{ 205, "Reset Content" },
		{ 206, "Partial Content" },
		{ 300, "Multiple Choices" },
		{ 301, "Moved Permanently" },
		{ 302, "Found" },
		{ 303, "See Other" },
		{ 304, "Not Modified" },
		{ 305, "Use Proxy" },
		{ 307, "Temporary Redirect" },
		{ 400, "Bad Request" },
		{ 401, "Unauthorized" },
		{ 402, "Payment Required" },
		{ 403, "Forbidden" },
		{ 404, "Not Found" },
		{ 405, "Method Not Allowed" },
		{ 406, "Not Acceptable" },
		{ 407, "Proxy Authentication Required" },
		{ 408, "Request Time-out" },
		{ 409, "Conflict" },
		{ 410, "Gone" },
		{ 411, "Length Required" },
		{ 412, "Precondition Failed" },
		{ 413, "Request Entity Too Large" },
		{ 414, "Request-URI Too Large" },
		{ 415, "Unsupported Media Type" },
		{ 416, "Requested range not satisfiable" },
		{ 417, "Expectation Failed" },
		{ 500, "Internal Server Error" },
		{ 501, "Not Implemented" },
		{ 502, "Bad Gateway" },
		{ 503, "Service Unavailable" },
		{ 504, "Gateway Time-out" },
		{ 505, "HTTP Version not supported" }
	};

	return map.find(code).operator*().second;
}

void	HttpModule::_parsePath(zany::Pipeline::Instance &i, std::string const &path) {
	std::size_t		endPathPos = 0;
	char			c;

	while (endPathPos < path.size() && (c = path.at(endPathPos)) && c != '?') ++endPathPos;

	i.request.path = std::string(path.begin(), path.begin() + endPathPos);
	if (c == '?') {
		i.request.fullQuery = std::string(path.begin() + endPathPos + 1, path.end());
		std::istringstream	sstm(i.request.fullQuery);
		std::string			param;

		while (std::getline(sstm, param, '&')) {
			std::string		key;

			std::istringstream	pstm(param);
			std::getline(pstm, key, '=');
			std::getline(pstm, i.request.query[key]);
		}
	}
}

bool	HttpModule::_urlDecode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

void	HttpModule::_setHeaders(zany::Pipeline::Instance &i) {
	try {
		if (i.serverConfig.isObject() && i.serverConfig["headers"].isObject() && i.serverConfig["request"].isObject()) {
			auto hds = i.serverConfig["headers"]["request"].value<zany::Object>();

			for (auto &el: hds) {
				if (!el.second.isString()) continue;
				i.request.headers[el.first] = el.second.value<zany::String>();
			}
		}
	} catch(...) {}
}

void	HttpModule::_beforeHandleRequest(zany::Pipeline::Instance &i) {
	i.writerID = this->getUniqueId();
	i.response.headers["server"] = "Zia (PCinc)";
	std::string	line;

	i.connection->stream() >> std::ws;
	std::getline(i.connection->stream(), line);
	if (line.empty()) { return; }
	{
		line.erase(--line.end());
		std::istringstream stm(line);
		std::string	str;
		
		stm >> str;
		i.request.method = ModuleUtils::getMethodFromString(str);
		i.request.methodString = str;
		
		std::string url;
		stm >> str;
		i.properties["basepath"] = zany::Property::make<std::string>(str);
		_urlDecode(str, url);
		HttpModule::_parsePath(i, url);

		stm >> str;
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
		boost::to_lower(key);
		auto &value = *i.request.headers[key];
		std::getline(splitor, value);
		boost::trim(value);

		if (boost::to_lower_copy(key) == "host") {
			std::istringstream	splitor(value);
			std::string			port;
			
			std::getline(splitor, i.request.host, ':');
			std::getline(splitor, port);

			if (port.empty() == false) {
				i.request.port = static_cast<int>(zany::HttpHeader(port).getNumber());
			} else {
				i.request.port = i.connection->info.localPort;
			}
		}
	}

	auto	found = false;

	if (i.serverConfig.isNull()) {
		for (auto &srv: master->getConfig()["server"].value<zany::Array>()) {
			if (srv["host"] == i.request.host && srv["port"] == i.request.port) {
				i.serverConfig = srv.clone();
				found = true;
				break;
			}
		}
	} else {
		found = true;
	}
	if (!found) {
		i.response.status = 404;
	} else {
		i.request.path = boost::filesystem::path(i.request.path).lexically_normal().string();
		if (i.request.path.substr(0, 3) == "../"
		    || i.request.path.substr(0, 3) == "/.."
		    || i.request.path.substr(0, 2) == "..") {
			i.response.status = 403; /* try to access to sub-directory */
		} else {
			i.request.path = boost::filesystem::path(
				(i.serverConfig["path"].isString()
				 ? i.serverConfig["path"].value<zany::String>()
				 : "")
				+ "/" + i.request.path
			).lexically_normal().string();
		}
		if (boost::filesystem::is_directory(i.request.path)) {
			for (auto &entry: boost::make_iterator_range(boost::filesystem::directory_iterator(i.request.path))) {
				boost::filesystem::path	p(entry);
				if (boost::filesystem::is_regular_file(p)
				&& boost::to_lower_copy(p.stem().string()) == "index") {
					i.request.path = p.lexically_normal().string();
				}
			}
		}
		_setHeaders(i);
	}
}

void	HttpModule::_onHandleRequest(zany::Pipeline::Instance &i) {

}

void	HttpModule::_beforeHandleResponse(zany::Pipeline::Instance &i) {
	if (i.response.status != 200 || (i.response.status == 200 && i.writerID != this->getUniqueId())) {
	} else if (!boost::filesystem::is_regular_file(i.request.path)) {
		i.response.status = 404;
	} else if (i.request.method == zany::HttpRequest::RequestMethods::GET && i.writerID == this->getUniqueId()) {
		auto &fs = (i.properties["filestream"] = zany::Property::make<boost::filesystem::ifstream>(
			i.request.path, std::ios_base::in | std::ios_base::binary
		)).get<boost::filesystem::ifstream>();

		if (fs.bad()) {
			i.response.status = 500;
		}
	}

	auto &resp = i.response;
	auto &stm = i.connection->stream();
	
	stm << resp.protocol << '/' << resp.protocolVersion
		<< ' ' << resp.status << ' ' << _getReasonPhrase(resp.status);
	
	try {
		if (i.serverConfig.isObject() && i.serverConfig["headers"].isObject() && i.serverConfig["response"].isObject()) {
			auto hds = i.serverConfig["headers"]["response"].value<zany::Object>();

			for (auto &el: hds) {
				if (!el.second.isString()) continue;
				i.response.headers[el.first] = el.second.value<zany::String>();
			}
		}
	} catch(...) {}

	for (auto &h: resp.headers) {
		stm << "\r\n" << h.first << ": " << *h.second;
	}
	stm << "\r\n";
}

void	HttpModule::_onHandleResponse(zany::Pipeline::Instance &i) {
	if (i.writerID == this->getUniqueId()
	&& i.response.status == 200
	&& i.request.method == zany::HttpRequest::RequestMethods::GET) {
		auto 	&fs = i.properties["filestream"].get<boost::filesystem::ifstream>();

		i.connection->stream() << "\r\n";
		if (*i.response.headers["transfer-encoding"] == "chunked") {
			ModuleUtils::copyByChunck(fs, i.connection->stream());
			i.connection->stream() << "\r\n";
		} else {
			i.connection->stream() << fs.rdbuf();
		}

		fs.close();
	} else if (i.writerID == 0 || i.writerID == this->getUniqueId()) {
		i.connection->stream()
			<< "\r\n"
			<< "<html><body><h2>"
			<< i.response.status << " - "
			<< _getReasonPhrase(i.response.status)
				<< "</h2></body></html>\r\n";
	}
}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::HttpModule();
}