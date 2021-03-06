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
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include "Zia.hpp"
#include "Zany/Connection.hpp"
#include "Constants.hpp"
#include "Zany/Event.hpp"

namespace zia {

Main::~Main() {
	_moduleLoaderThread.detach();
}

void	Main::_bootstrap() {
	auto	pmodules = _vm.count("modules")
							? _vm["modules"].as<std::vector<std::string>>()
							: std::vector<std::string>{};
	bool	parsed = false;

	boost::filesystem::current_path(_vm["work-dir"].as<std::string>());

	boost::filesystem::path	p("./modules");
	if(boost::filesystem::exists(p) && boost::filesystem::is_directory(p)) {
		boost::filesystem::recursive_directory_iterator it(p);
		boost::filesystem::recursive_directory_iterator endit;

		while(it != endit)
		{
			if (
			(boost::filesystem::is_regular_file(*it) || boost::filesystem::is_symlink(*it))
			&& it->path().extension() ==
#				if defined(ZANY_ISWINDOWS)
					".dll"
#				else
					".so"
#				endif
			) {
				auto mp =		
#				if defined(ZANY_ISWINDOWS)
					it->path().lexically_normal();
#				else
					boost::filesystem::path(
						boost::filesystem::current_path().string() +
						(boost::filesystem::is_symlink(*it)
							? boost::filesystem::read_symlink(it->path())
							: *it
						).string()
					).lexically_normal()
#				endif
				;
				pmodules.push_back(mp.string());
			}
			++it;
		}
	};

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
}

void	Main::_moduleLoaderEntrypoint() {
	std::string line;
	while (std::getline(std::cin, line)) {
		std::istringstream	sstm(line);
		std::string command;
		sstm >> command;

		if (boost::to_lower_copy(command) == "list") {
			for (auto &m : _loader) {
				std::cout << m.name() << std::endl;
			}
		} else if (boost::to_lower_copy(command) == "load") {
			std::string path;

			sstm >> path;
			loadModule(path, [] (auto &module) {
				std::cout << "Module: " << module.name() << " loaded\n"; 
			}, [] (auto e) {
				std::cerr << e.what() << std::endl;
			});
		} else if (boost::to_lower_copy(command) == "unload") {
			std::string name;
			zany::Loader::AbstractModule *module = nullptr;

			sstm >> name;
			for (auto &m: _loader) {
				if (m.name() == name) {
					module = &m;
					break;
				}
			}
			if (!module) {
				std::cerr << "Unkown module" << std::endl;
				return;
			}
			unloadModule(*module, [name] () {
				std::cout << "Module: " << name << " loaded\n"; 
			}, [] (auto e) {
				std::cerr << e.what() << std::endl;
			});
		} else if (boost::to_lower_copy(command) == "reload") {
			reload();
		}
	}
}

void	Main::run(int ac, char **av) {
	namespace po = boost::program_options;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("work-dir", po::value<std::string>()->default_value(constant::defWorkDir), "workdir")
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

	_basePwd = boost::filesystem::current_path().string();

	_refreshing = true;
	while (_refreshing) {
		_refreshing = false;
		for (auto it = _loader.begin(); it != _loader.end();) {
			auto name = it->name();
			auto toDelete = it++;

			unloadModule(*toDelete, [name] {
				std::cout << "Module: " << name << " unloaded" << std::endl;
			});
		}
		
		_bootstrap();
		_start();

		_ctx.run();

	}
}

void	Main::onPipelineThrow(PipelineExecutionError const &exception) {
	std::cerr << "Error: " << exception.what() << std::endl;
}

void	Main::_start() {

	static constexpr auto __initCore = [] (Main *self) { // capturing this cause sigsegv ??? wtf
		std::vector<std::uint16_t>	ports;
		auto &cports = self->_config["listen"].value<zany::Array>();

		ports.reserve(cports.size());
		for (auto &cport: cports) {
			ports.push_back(cport.to<int>());
		}
		self->getCore()->startListening(ports);
		
		self->_ctx.addTask([] { std::cout << "Ready" << std::endl; });
	};

	auto initCore = std::make_shared<std::function<void()>>();
	
	*initCore = [this, initCore] {
		if (getCore()) {
			__initCore(this);
			return;
		} else {
			std::cout << "Waiting for a core module" << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		}
		_ctx.addTask([this, initCore] { _ctx.addTask(*initCore); });
	};

	_ctx.addTask(*initCore);
}

void	Main::reload() {
	this->_ctx.addTask([this] {
		_refreshing = true;
		std::cout << "Refreshing" << std::endl;
		boost::filesystem::current_path(_basePwd);
		this->_ctx.stop();
	});
}

}