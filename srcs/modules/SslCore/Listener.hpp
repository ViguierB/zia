/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Listener.hpp
*/

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "Zany/Connection.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Event.hpp"

namespace zia {

class Listener {
public:
	class Connection: zany::Connection {
	public:
		static auto make(boost::asio::io_service& ios)
			{ return SharedInstance(reinterpret_cast<zany::Connection*>(new Connection(ios))); }

		virtual std::iostream	&stream() final { return _stream; }

		auto		&socket() { return _stream.socket(); }
		static auto	&fromZany(zany::Connection &self) { return reinterpret_cast<Connection&>(self); }

		void	setInstance(zany::Pipeline::Instance &instance) { _instance = &instance; }
	private:
		Connection(boost::asio::io_service& ios):
				zany::Connection() {
			auto &m = zany::evt::Manager::get();

			_collector << m["onClose"]->addHandler([this] {
				if (_instance)
					_instance->context.stop();
			});
		}

		boost::asio::ip::tcp::iostream 	_stream;
		zany::Pipeline::Instance		*_instance = nullptr;
		zany::evt::HdlCollector			_collector;
	};


	template<typename PVERSION>
	Listener(PVERSION pv, boost::asio::io_service& ios, std::uint16_t port):
		_acceptor(ios, boost::asio::ip::tcp::endpoint(std::forward<PVERSION>(pv), port)) {}

	void startAccept() {
		auto nc = Connection::make(_acceptor.get_io_service());

		_acceptor.async_accept(Connection::fromZany(*nc).socket(),
			boost::bind(&Listener::handleAccept, this, nc,
			boost::asio::placeholders::error));
	}

	void handleAccept(zany::Connection::SharedInstance nConnection, const boost::system::error_code& error) {
		if (!error) {
			auto &&remoteEp = Connection::fromZany(*nConnection).socket().remote_endpoint();
			auto &&remoteAd = remoteEp.address();
			nConnection->info.ip = remoteAd.to_string();
			nConnection->info.port = remoteEp.port();

			std::cout << "New Connection from: " << '[' << nConnection->info.ip << "]:" << nConnection->info.port << std::endl;
			onHandleAccept(nConnection);
			startAccept();
		}
	}

	std::function<void (zany::Connection::SharedInstance nConnection)>
									onHandleAccept;

	boost::asio::ip::tcp::acceptor	_acceptor;
};

}