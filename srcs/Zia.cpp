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
#include <boost/bind.hpp>
#include "Zia.hpp"
#include "Zany/Connection.hpp"

namespace zia {

void	Main::routine() {
	
}

template<auto PVERSION>
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


	_listener(boost::asio::io_service& ios, std::uint16_t port):
		_acceptor(ios, boost::asio::ip::tcp::endpoint(PVERSION(), port)) {}

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
	namespace bip = boost::asio::ip;

	std::vector<std::uint16_t> defaultPort{ 80 };

	auto &ports = (_vm.count("port")
					? _vm["port"].as<std::vector<std::uint16_t>>()
					: defaultPort);
	
	std::vector<_listener<&boost::asio::ip::tcp::v4>>	v4Acceptors;

	v4Acceptors.reserve(ports.size());
	for (auto port: ports) {
		auto &listener = v4Acceptors.emplace_back(ios, port);
		listener.onHandleAccept = [this] (zany::Connection &p) {
			this->_ctx.addTask(std::bind(&Main::startPipeline, this, 0));
		};

		listener.startAccept();
	}

	ios.run();
}

void	Main::run(int ac, char **av) {
	namespace po = boost::program_options;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("port", po::value<std::vector<std::uint16_t>>()->composing(), "set listening port")
	;

	po::store(po::parse_command_line(ac, av, desc), _vm);
	po::notify(_vm);

	if (_vm.count("help")) {
		std::cout << desc << "\n";
		return;
	}

	boost::asio::io_service ios;
	std::thread	t(std::bind(&Main::_listening, this, std::ref(ios)));

	_ctx.run();
	ios.stop();
	t.join();
}

}