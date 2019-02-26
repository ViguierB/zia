/*
** EPITECH PROJECT, 2018
** zia
** File description:
** zia.hpp
*/

#pragma once

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include "Zany/Orchestrator.hpp"

namespace zia {

class Main: public zany::Orchestrator {
public:
	Main(zany::Context	&_ctx): zany::Orchestrator(_ctx) {}

	virtual auto	getConfig() const -> const zany::Entity final { return _config; }
	void			run(int ac, char **av);
	void			startPipeline(zany::Connection::SharedInstance c);
	void			onPipelineReady(zany::Pipeline::Instance &);
private:
	void			_listening();
	void			_bootstrap();
	void			_onSignal();

	boost::program_options::variables_map	_vm;
	zany::Entity							_config;
	boost::asio::io_service 				_ios;
};

}