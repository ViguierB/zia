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
	Main(zany::Context	&_ctx):
	zany::Orchestrator(_ctx), 
	_moduleLoaderThread(std::bind(&Main::_moduleLoaderEntrypoint, this)) {}

	~Main();

	virtual auto	getConfig() const -> const zany::Entity final { return _config; }
	void			run(int ac, char **av);
	void			startPipeline(zany::Connection::SharedInstance c);
	virtual void	reload() final;
protected:
	virtual void	onPipelineThrow(PipelineExecutionError const &exception) final;
private:
	void	_start();
	void	_listening();
	void	_bootstrap();
	void	_onSignal();
	void	_moduleLoaderEntrypoint();

	boost::program_options::variables_map	_vm;
	zany::Entity							_config;
	boost::asio::io_service 				_ios;
	std::string								_basePwd;
	bool									_refreshing;
	std::thread								_moduleLoaderThread;
};

}