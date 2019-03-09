/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <fstream>
#include <thread>
#include <memory>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include "Zany/Orchestrator.hpp"
#include "Zany/Loader.hpp"
#include "TcpConnexion.hpp"

namespace zia {

class ManagerModule : public zany::Loader::AbstractModule {
public:
	~ManagerModule() {
		_continue = false;
		if (_worker.get()) {
			_worker->join();
		}
	}

	virtual auto	name() const -> const std::string&
		{ static const std::string name("Manager"); return name; }
	virtual void	init() final;
private:
	void	_verifConfig() {
		auto config = this->master->getConfig();

		try {
			if (config["manager"]["listening-port"].isNumber()) {
				ManagerModule::_basicConfig["manager"]["listening-port"] = config["manager"]["listening-port"];
			}
		} catch (...) {}
	}

	void	_entrypoint();
	void	handle_accept(TcpConnexion::pointer new_connection, const boost::system::error_code& error);

	void	Unload(std::string &line, std::shared_ptr<boost::asio::ip::tcp::iostream> tcpstream);
	void	Load(std::string &line, std::shared_ptr<boost::asio::ip::tcp::iostream> tcpstream);


	static inline zany::Entity _basicConfig = zany::makeObject {
	{ "manager", zany::makeObject {
		{ "listening-port", 5768 }
	} }
	};

	std::unique_ptr<std::thread>	_worker;
	std::unique_ptr<boost::asio::ip::tcp::acceptor>
					_acceptor;
	bool							_continue = true;
};

void	ManagerModule::init() {
	master->getContext().addTask([this] {
		_verifConfig();

		_worker = std::make_unique<decltype(_worker)::element_type>(
			std::bind(&ManagerModule::_entrypoint, this));
	});
}

void	ManagerModule::handle_accept(TcpConnexion::pointer new_connection, const boost::system::error_code& error) {
	if (!error) {
		TcpConnexion::pointer new_connection =
			TcpConnexion::create(_acceptor->get_io_service());


		auto tcpstream = std::shared_ptr<boost::asio::ip::tcp::iostream>();

		_acceptor->accept(tcpstream->socket());

		std::thread	thread([tcpstream] {
			std::cout << tcpstream->rdbuf();
		});
	}
}

void	ManagerModule::Unload(std::string &line, std::shared_ptr<boost::asio::ip::tcp::iostream> tcpstream) {
	std::stringstream comm;
	comm << line;
	comm >> line;

	while (comm.rdbuf()->in_avail() != 0) {
		line.clear();
		comm >> line;
		zany::Loader::AbstractModule *toRemove = nullptr;

		for (auto &module : const_cast<zany::Loader &>(this->master->getLoader())) {
			if (module.name() == line) {
				toRemove = &module;
				break;
			}
		}
		if (!toRemove) {
			*tcpstream << "Module " << line << " not found." << std::endl;
		} else {
			std::condition_variable		locker;
			std::mutex			mtx;
			std::unique_lock<decltype(mtx)>	ulock(mtx);

			this->master->unloadModule(*toRemove,
						   [&] {
							   *tcpstream
								   << "Unloading "
								   << line
								   << ": Ok"
								   << std::endl;
							   locker.notify_all();
						   },
						   [&] (zany::Loader::Exception e) {
							   *tcpstream
								   << "Unloading "
								   << line
								   << ": Ko"
								   << std::endl
								   << e.what()
								   << std::endl;
							   locker.notify_all();
						   });
			locker.wait(ulock);
		}

	}
}

void	ManagerModule::Load(std::string &line, std::shared_ptr<boost::asio::ip::tcp::iostream> tcpstream) {
	std::stringstream comm;
	boost::filesystem::path path;
	comm << line;
	comm >> line;
	bool found = false;

	while (comm.rdbuf()->in_avail() != 0) {
		line.clear();
		comm >> line;
		found = false;
		path = line;

		if ((boost::filesystem::is_regular_file(path) || boost::filesystem::is_symlink(path))
		    && path.extension() ==
#if defined(ZANY_ISWINDOWS)
		       ".dll"
#else
		       ".so"
#endif
			) {
			std::condition_variable		locker;
			std::mutex			mtx;
			std::unique_lock<decltype(mtx)>	ulock(mtx);
			auto mp =
#if defined(ZANY_ISWINDOWS)
				it->path().lexically_normal();
#else
				boost::filesystem::path(
					(boost::filesystem::is_symlink(path)
					 ? boost::filesystem::read_symlink(path)
					 : path
					).string()
				).lexically_normal()
#endif
			;
			this->master->loadModule(path.generic_string(), [&] (auto &module) {
				*tcpstream << "Module " << module.name() << " loaded." << std::endl;
				locker.notify_all();
			}, [] (auto error) {
				std::cerr << error.what() << std::endl;
			});
			locker.wait(ulock);
			found = true;
		}
		if (!found)
			*tcpstream << "Module " << line << " not found." << std::endl;
	}
}

void	ManagerModule::_entrypoint() {
	boost::asio::io_service io_service;
	_acceptor = std::make_unique<decltype(_acceptor)::element_type>(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 4222));
	TcpConnexion::pointer new_connection =
		TcpConnexion::create(_acceptor->get_io_service());

	std::list<std::thread>	ts;

	while (true) {
		auto tcpstream = std::make_shared<boost::asio::ip::tcp::iostream>();

		_acceptor->accept(tcpstream->socket());

		ts.emplace_back([this, tcpstream] {
			std::string line;

			*tcpstream << "--> ";
			while (std::getline(*tcpstream, line)) {

				if (line == "LIST") {
					for (auto const &module : const_cast<zany::Loader &>(master->getLoader())) {
						*tcpstream << module.name() << '\n';
					}
				} else if (line.find("UNLOAD") == 0) {
					Unload(line, tcpstream);
				} else if (line.find("LOAD") == 0) {
					Load(line, tcpstream);
				}else if (line == "QUIT") {
					*tcpstream << "FERME TOI." << std::endl;
					tcpstream->socket().close();
					return;
				}
				*tcpstream << std::endl << "--> ";
			}
		});
	}
}
}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::ManagerModule();
}

void salut() {

}