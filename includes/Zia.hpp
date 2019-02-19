/*
** EPITECH PROJECT, 2018
** zia
** File description:
** zia.hpp
*/

#pragma once

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "Zany/Orchestrator.hpp"

namespace zia {

class Main: public zany::Orchestrator {
public:
	Main(zany::Context	&_ctx): zany::Orchestrator(_ctx) {}

	virtual auto	getConfig() const -> const zany::Entity final { return _config; }
	virtual void	routine() final;
	void			run(int ac, char **av);
private:
	void			_listening(boost::asio::io_service &);
	void			_bootstrap();

	boost::program_options::variables_map	_vm;
	zany::Entity							_config;
};

}