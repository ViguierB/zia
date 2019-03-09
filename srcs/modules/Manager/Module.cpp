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
#include <boost/filesystem.hpp>
#include "Zany/Orchestrator.hpp"
#include "Zany/Loader.hpp"
#include "TcpConnection.hpp"

namespace zia {
namespace manager {

class ManagerModule : public zany::Loader::AbstractModule {
public:
	ManagerModule() = default;
	~ManagerModule() {
		_continue = false;
		_ios.stop();
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
	void	_startListener();
	void	_handleReceive(const boost::system::error_code& error, std::size_t);
	void	_handleSend(boost::shared_ptr<std::string>, const boost::system::error_code&, std::size_t);

	static inline zany::Entity _basicConfig = zany::makeObject {
		{ "manager", zany::makeObject {
			{ "listening-port", 4222 }
		} }
	};

	std::unique_ptr<std::thread>	_worker;
	std::unique_ptr<boost::asio::ip::udp::socket>
									_socket;
  	boost::asio::ip::udp::endpoint	_remoteEndpoint;
	std::array<char, 4096>			_buffer;
	bool							_continue = true;
	boost::asio::io_service 		_ios;
};

void	ManagerModule::init() {
	master->getContext().addTask([this] {
		_verifConfig();

		_worker = std::make_unique<decltype(_worker)::element_type>(
			std::bind(&ManagerModule::_entrypoint, this));
	});
}

void	ManagerModule::_handleReceive(const boost::system::error_code& error, std::size_t size) {
	TcpConnection	connection(*this->master);

	connection.start(std::string(_buffer.data(), size));

	boost::shared_ptr<std::string> message(new std::string(connection.getResponse()));

	_socket->async_send_to(boost::asio::buffer(*message), _remoteEndpoint,
		boost::bind(
			&ManagerModule::_handleSend,
		  	this, message,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	connection.getResponse();

	_startListener();
}

void	ManagerModule::_handleSend(boost::shared_ptr<std::string>, const boost::system::error_code&, std::size_t) {}

void	ManagerModule::_startListener() {
	_socket->async_receive_from(
		boost::asio::buffer(_buffer), _remoteEndpoint,
		boost::bind(&ManagerModule::_handleReceive, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void	ManagerModule::_entrypoint() {
	_socket = std::make_unique<decltype(_socket)::element_type>(
		_ios,
		boost::asio::ip::udp::endpoint(
			boost::asio::ip::udp::v4(),
			ManagerModule::_basicConfig["manager"]["listening-port"].to<int>())
	);

	_startListener();

	_ios.run();
}

}
}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::manager::ManagerModule();
}