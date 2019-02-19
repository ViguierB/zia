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
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include "Zia.hpp"
#include "Zany/Connection.hpp"
#include "Constants.hpp"

namespace zia {

void	Main::routine() {
	
}

class _listener {
public:
	class Connection: zany::Connection {
	public:
		static auto make(boost::asio::io_service& ios)
			{ return SharedInstance(static_cast<zany::Connection*>(new Connection(ios))); }

		virtual void	onDataAvailable(std::size_t len) final {

		}

		virtual void	write(const char *buffer, std::size_t len) final {
			
		}

		auto& socket() { return _socket; }

	private:
		Connection(boost::asio::io_service& ios)
			: _socket(ios) {}

		boost::asio::ip::tcp::socket	_socket;
	};


	template<typename PVERSION>
	_listener(PVERSION pv, boost::asio::io_service& ios, std::uint16_t port):
		_acceptor(ios, boost::asio::ip::tcp::endpoint(pv(), port)) {}

	void startAccept() {
		auto nc = Connection::make(_acceptor.get_io_service());

		_acceptor.async_accept(reinterpret_cast<Connection*>(nc.get())->socket(),
			boost::bind(&_listener::handleAccept, this, nc,
			boost::asio::placeholders::error));
	}

	void handleAccept(zany::Connection::SharedInstance nConnection, const boost::system::error_code& error) {
		if (!error) {
			onHandleAccept(*nConnection);
			startAccept();
		}
	}

	std::function<void (zany::Connection &nConnection)>
									onHandleAccept;

	boost::asio::ip::tcp::acceptor	_acceptor;
};

void	Main::_listening(boost::asio::io_service &ios) {
	auto &ports = _config["listen"].value<zany::Array>();
	
	std::vector<_listener>	acceptors;

	acceptors.reserve(ports.size() * 2);
	for (auto port__: ports) {
		std::uint16_t	port = port__.to<int>();
		auto 			&v4listener = acceptors.emplace_back(&boost::asio::ip::tcp::v4, ios, port);
		// auto &v6listener = acceptors.emplace_back(&boost::asio::ip::tcp::v6, ios, port);
		v4listener.onHandleAccept = /* v6listener.onHandleAccept = */ [this] (zany::Connection &p) {
			this->_ctx.addTask(std::bind(&Main::startPipeline, this, 0));
		};

		v4listener.startAccept();
		// v6listener.startAccept();
	}

	ios.run();
}

void	Main::_bootstrap() {
	if (!boost::filesystem::exists(_vm["parser"].as<std::string>()))
		throw std::runtime_error((_vm["parser"].as<std::string>() + ": Parser module not found").c_str());

	auto &parser = _loader.load(_vm["parser"].as<std::string>());

	_config = parser.parse(_vm["config"].as<std::string>());
	if (_config == false || _config.isObject() == false) {
		_config = constant::defConfig.clone();
	}
}

void	Main::run(int ac, char **av) {
	namespace po = boost::program_options;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("parser", po::value<std::string>()->default_value(constant::parserPath), "Parser module path")
		("config", po::value<std::string>()->default_value(constant::mainConfigPath), "Main config path")
		("worker-thread-number", po::value<std::uint16_t>()->default_value(constant::defThreadNbr), "Number of worker threads")
	;

	po::store(po::parse_command_line(ac, av, desc), _vm);
	po::notify(_vm);

	if (_vm.count("help")) {
		std::cout << desc << "\n";
		return;
	}

	auto tp = std::make_unique<zany::ThreadPool>(std::max(std::uint16_t(1), _vm["worker-thread-number"].as<std::uint16_t>()));
	
	this->_pline.linkThreadPool(*tp);

	_bootstrap();

	boost::asio::io_service ios;
	std::thread	t(std::bind(&Main::_listening, this, std::ref(ios)));

	_ctx.run();
	ios.stop();
	t.join();
}

}