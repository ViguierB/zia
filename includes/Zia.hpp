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
	Main(zany::Context	&_ctx): zany::Orchestrator(_ctx), _signals(_ios) {
		_signals.add(SIGINT);
		_signals.add(SIGTERM);
	#if defined(SIGQUIT)
		_signals.add(SIGQUIT);
	#endif // defined(SIGQUIT)
		_signals.async_wait(boost::bind(&Main::_onSignal, this));
	}

	virtual auto	getConfig() const -> const zany::Entity final { return _config; }
	virtual void	routine() final;
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
	boost::asio::signal_set					_signals;
};

}