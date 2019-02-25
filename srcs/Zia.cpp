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

void	Main::routine() {}

class _Listener {
public:
	class Connection: zany::Connection {
	public:
		static auto make(boost::asio::io_service& ios)
			{ return SharedInstance(reinterpret_cast<zany::Connection*>(new Connection(ios))); }

		void	onDataAvailable(std::size_t len) {
			std::cout << len << std::endl;
			_startWaitForReadyRead();
		}

		virtual std::size_t	write(const char *buffer, std::size_t len) final {
			try {
				_socket.wait((decltype(_socket)::wait_write));
				return _socket.send(boost::asio::buffer(buffer, len));
			} catch (...) {

			}
		}

		void	start() { _startWaitForReadyRead(); }

		auto& socket() { return _socket; }
		static auto &fromZany(zany::Connection &self) { return reinterpret_cast<Connection&>(self); }

		void	setInstance(zany::Pipeline::Instance &instance) { _instance = &instance; }
	private:
		Connection(boost::asio::io_service& ios):
				zany::Connection(),
				_socket(ios) {
			auto &m = zany::evt::Manager::get();

			_collector << m["onClose"]->addHandler([this] {
				if (_instance)
					_instance->context.stop();
			});
		}

		void	_startWaitForReadyRead() {
			_socket.async_wait(
				decltype(_socket)::wait_read,
				[this] (auto error) {
					if (!error) {
						this->onDataAvailable(_socket.available());
					} else {
						std::cerr << "Reading error: " << error.message() << std::endl;
					}
				}
			);
		}

		boost::asio::ip::tcp::socket	_socket;
		zany::Pipeline::Instance		*_instance = nullptr;
		zany::evt::HdlCollector			_collector;
	};


	template<typename PVERSION>
	_Listener(PVERSION pv, boost::asio::io_service& ios, std::uint16_t port):
		_acceptor(ios, boost::asio::ip::tcp::endpoint(std::forward<PVERSION>(pv), port)) {}

	void startAccept() {
		auto nc = Connection::make(_acceptor.get_io_service());

		_acceptor.async_accept(Connection::fromZany(*nc).socket(),
			boost::bind(&_Listener::handleAccept, this, nc,
			boost::asio::placeholders::error));
	}

	void handleAccept(zany::Connection::SharedInstance nConnection, const boost::system::error_code& error) {
		if (!error) {
			auto &&remoteEp = Connection::fromZany(*nConnection).socket().remote_endpoint();
			auto &&remoteAd = remoteEp.address();
			nConnection->info.ip = remoteAd.to_string();

			std::cout << "New Connection from: " << nConnection->info.ip << std::endl;
			onHandleAccept(nConnection);
			startAccept();
		}
	}

	std::function<void (zany::Connection::SharedInstance nConnection)>
									onHandleAccept;

	boost::asio::ip::tcp::acceptor	_acceptor;
};

void	Main::_onSignal() {
	std::cout << "Closing..." << std::endl;
	_ctx.stop();
}

/**
 *  Thread Safe !!
 **/
void	Main::onPipelineReady(zany::Pipeline::Instance &instance) {
	_Listener::Connection::fromZany(*(instance.connection)).setInstance(instance);

	instance.context.addTask([&] {
		this->getPipeline().getHookSet<zany::Pipeline::Hooks::BEFORE_HANDLE_REQUEST>().execute(instance);
	});
	instance.context.run();
}

void	Main::_listening() {
	auto &ports = _config["listen"].value<zany::Array>();
	
	std::vector<_Listener>	acceptors;

	acceptors.reserve(ports.size() * 2);
	for (auto port__: ports) {
		std::uint16_t	port = port__.to<int>();
		auto 			&v4listener = acceptors.emplace_back(boost::asio::ip::tcp::v4(), _ios, port);
		// auto &v6listener = acceptors.emplace_back(&boost::asio::ip::tcp::v6, ios, port);
		v4listener.onHandleAccept = /* v6listener.onHandleAccept = */ [this] (zany::Connection::SharedInstance p) {
			this->_ctx.addTask(std::bind(&Main::startPipeline, this, p));
		};

		v4listener.startAccept();
		// v6listener.startAccept();
	}

	_ios.run();
}

void	Main::startPipeline(zany::Connection::SharedInstance c) {
	constexpr auto sp = &zany::Orchestrator::startPipeline;

	_Listener::Connection::fromZany(*c).start();
	try {
		(this->*sp)(c);
	} catch (std::exception &e) {
		std::cerr
			<< "Connection rejected because of internal server error:\n\t"
			<< e.what() << std::endl;
	}
}

void	Main::_bootstrap() {
	if (!boost::filesystem::exists(_vm["parser"].as<std::string>()))
		throw std::runtime_error((_vm["parser"].as<std::string>() + ": Parser module not found").c_str());

	auto &parser = _loader.load(_vm["parser"].as<std::string>());

	_config = parser.parse(_vm["config"].as<std::string>());
	if (_config == false || _config.isObject() == false) {
		_config = constant::defConfig.clone();
	} else {
		for (auto &cp: constant::defConfig.value<zany::Object>()) {
			if (_config.value<zany::Object>().find(cp.first) == _config.value<zany::Object>().end()) {
				_config[cp.first] = cp.second;
			}
		}
	}

	std::cout << "Ready" << std::endl;
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

	zany::ThreadPool tp(std::max(std::uint16_t(1), _vm["worker-thread-number"].as<std::uint16_t>()));
	
	this->_pline.linkThreadPool(tp);

	_bootstrap();

	std::thread	t(std::bind(&Main::_listening, this));

	_ctx.run();
	zany::evt::Manager::get()["onClose"]->fire();
	_ios.stop();
	t.join();
}

}