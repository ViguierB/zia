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

namespace zia {

class MimeModule : public zany::Loader::AbstractModule {
public:
	virtual auto	name() const -> const std::string&
		{ static const std::string name("Mime"); return name; }
	virtual void	init() final;
private:
	inline void		_onHandleRequest(zany::Pipeline::Instance &i);
	inline void		_onHandleResponse(zany::Pipeline::Instance &i);

	inline std::string	_isAllowedExt(boost::filesystem::path const &path, zany::Pipeline::Instance const &i) const;
};

void	MimeModule::init() {
	garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_REQUEST>()
		.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&MimeModule::_onHandleRequest, this, std::placeholders::_1));

	// garbage << this->master->getPipeline().getHookSet<zany::Pipeline::Hooks::ON_HANDLE_RESPONSE>()
	// 	.addTask<zany::Pipeline::Priority::MEDIUM>(std::bind(&MimeModule::_onHandleResponse, this, std::placeholders::_1));
}

std::string	MimeModule::_isAllowedExt(boost::filesystem::path const &path, zany::Pipeline::Instance const &i) const {
	constexpr std::pair<const char*, const char *> defexts[] = {
		{ ".html", "text/html" },
		{ ".htm", "text/html" },
		{ ".css", "text/css" },
		{ ".js", "text/javascript" },
		{ ".json", "text/plain" },
		{ ".xml", "text/plain" },
		{ ".ico", "image/x-icon" }
	};
	constexpr size_t size = sizeof(defexts) / sizeof(decltype(*defexts));

	auto	mext = boost::filesystem::extension(path);

	for (size_t i = 0; i < size; ++i) {
		if (mext == defexts[i].first) {
			return defexts[i].second;
		} 
	}

	try {
		auto &cexts = i.serverConfig["mime"];
		if (!cexts.isArray())
			return "";
		for (auto const &ext: cexts.value<zany::Array>()) {
			if (ext.isArray() == false) {
				std::cerr << "Bad Mime type" << std::endl;
				continue;
			}
			if (ext.isString() && mext == ext[0].value<zany::String>()) {
				return ext[1].value<zany::String>();
			}
		}
	} catch (...) {}
	return "";
}

void	MimeModule::_onHandleRequest(zany::Pipeline::Instance &i) {
	if (i.properties.find("reverse-proxy-enabled") != i.properties.end()) return;
	if (boost::filesystem::is_directory(i.request.path)) {
		for (auto &entry: boost::make_iterator_range(boost::filesystem::directory_iterator(i.request.path))) {
			boost::filesystem::path	p(entry);
			if (boost::filesystem::is_regular_file(p)
			&& boost::to_lower_copy(p.stem().string()) == "index") {
				i.request.path = p.lexically_normal().string();
			}
		}
	}
	
	auto mime = _isAllowedExt(i.request.path, i);

	if (mime.empty()) {
		i.response.status = 403;
	} else if (i.response.status == 200) {
		i.response.headers["content-type"] = mime;
		i.response.headers["content-length"] = std::to_string(boost::filesystem::file_size(i.request.path));
	}
}

void	MimeModule::_onHandleResponse(zany::Pipeline::Instance &i) {
	
}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::MimeModule();
}