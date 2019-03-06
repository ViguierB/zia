/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <iostream>
#include "Zany/Loader.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Orchestrator.hpp"

namespace zia {

class ReverseProxyModule : public zany::Loader::AbstractModule {
public:
	virtual auto	name() const -> const std::string&
		{ static const std::string name("ReverseProxy"); return name; }
	virtual void	init() final;
private:
	inline void		_ready();
	// inline void		_onHandleRequest(zany::Pipeline::Instance &i);
	// inline void		_onDataReady(zany::Pipeline::Instance &i);
	// inline void		_onHandleResponse(zany::Pipeline::Instance &i);

	
};

void	ReverseProxyModule::init() {
	master->getContext().addTask(std::bind(&ReverseProxyModule::_ready, this));
}

void	ReverseProxyModule::_ready() {
	auto config = master->getConfig();


}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::ReverseProxyModule();
}