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
		{ static const std::string name("ManagerModule"); return name; }
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

			while (std::getline(*tcpstream, line)) {
				if (line.compare("LIST") == 0) {
					for (auto const &module : const_cast<zany::Loader &>(master->getLoader())) {
						*tcpstream << module.name() << '\n';
					}
				} else if (line.compare("UNLOAD")) {

				} else if (line.compare("QUIT") == 0)
					return;
			}
		});
	}
}
}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::ManagerModule();
}