/*
** EPITECH PROJECT, 2018
** zia
** File description:
** zia.cpp
*/

#include <thread>
#include <iostream>
#include <vector>
#include <functional>
#include <type_traits>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include "Zia.hpp"
#include "Zany/Connection.hpp"
#include "Constants.hpp"
#include "Zany/Event.hpp"

namespace zia {

/**
 *  Thread Safe !!
 **/
void	Main::onPipelineReady(zany::Pipeline::Instance &instance) {
	instance.context.addTask([&] {
		zany::Pipeline::Hooks::forEach([&] (auto hook) {
			this->getPipeline().getHookSet(hook).execute(instance);
		});
		instance.context.stop();
	});
	instance.context.run();
}

void	Main::_bootstrap() {
	std::vector<std::string>
			def{ constant::parserPath };
	auto	&pmodules =  _vm.count("modules")
							? _vm["modules"].as<std::vector<std::string>>()
							: def;
	bool	parsed = false;


	for (auto &pm : pmodules) {
		loadModule(pm, [this, &parsed] (auto &module) {
			std::cout << "Module: " << module.name() << " loaded" << std::endl;

			if (module.isAParser() && !parsed) {
				_config = module.parse(_vm["config"].as<std::string>());
				
				if (!(_config == false || _config.isObject() == false)) {
					for (auto &cp: constant::defConfig.value<zany::Object>()) {
						if (_config.value<zany::Object>().find(cp.first) == _config.value<zany::Object>().end()) {
							_config[cp.first] = cp.second;
						}
					}
					parsed = true;
				}
			}
		}, [] (auto error) {
			std::cerr << error.what() << std::endl;
		});
	}

	if (parsed == false) {
		_config = constant::defConfig.clone();
	}

	std::cout << "Ready" << std::endl;
}

void	Main::run(int ac, char **av) {
	namespace po = boost::program_options;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("modules", po::value<std::vector<std::string>>(), "modules path")
		("config", po::value<std::string>()->default_value(constant::mainConfigPath), "Main config path")
		("worker-thread-number", po::value<std::uint16_t>()->default_value(constant::defThreadNbr), "Number of worker threads")
	;

	po::store(po::parse_command_line(ac, av, desc), _vm);
	po::notify(_vm);

	if (_vm.count("help")) {
		std::cout << desc << "\n";
		return;
	}

	zany::ThreadPool tp(std::max(std::uint16_t(1), _vm["worker-thread-number"].as<std::uint16_t>()));
	
	this->_pline.linkThreadPool(tp);

	_bootstrap();

	if (getCore()) {
		std::vector<std::uint16_t>	ports;
		auto &cports = _config["listen"].value<zany::Array>();

		ports.reserve(cports.size());
		for (auto &cport: cports) {
			ports.push_back(cport.to<int>());
		}
		getCore()->startListening(ports);
	} else {
		throw std::runtime_error("Critical error: No core module loaded !");
	}

	_ctx.run();
}

}