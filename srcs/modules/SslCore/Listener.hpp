/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Listener.hpp
*/

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp> 
#include <boost/iostreams/stream.hpp>
#include <boost/bind.hpp>
#include "Zany/Connection.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Event.hpp"

#define SSL_IDENTIFIER (22)

namespace zia {

class Listener {
public:
	class SslTcpIoStreamBidirectional: public boost::iostreams::device<boost::iostreams::bidirectional> {

	public:
		SslTcpIoStreamBidirectional(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> *ssls)
		: stream(*ssls), disableSsl(false) {}

		std::streamsize write(const char* s, std::streamsize n) {
			if (disableSsl) {
				return boost::asio::write(stream.next_layer(), boost::asio::buffer(s, n));
			} else {
				return boost::asio::write(stream, boost::asio::buffer(s, n));
			}
		}

		std::streamsize read(char* s, std::streamsize n)  {
			if (disableSsl) {
				return ::recv(stream.next_layer().native_handle(), s, n, 0);
			} else {
				return boost::asio::read(stream, boost::asio::buffer(s, n));
			}
		}

		void	setSslDisabled(bool status) { disableSsl = status; }
	private:
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>	&stream;
		bool													disableSsl;
	};

	class Connection: zany::Connection {
	public:
		static auto make(boost::asio::io_service& ios)
			{ return SharedInstance(reinterpret_cast<zany::Connection*>(new Connection(ios))); }

		virtual std::iostream	&stream() final { return _sslstream; }

		auto		&socket() { return _ssock.lowest_layer(); }
		static auto	&fromZany(zany::Connection &self) { return reinterpret_cast<Connection&>(self); }
		auto	peek() {
			char buffer = 0;
			::recv(socket().native_handle(), &buffer, 1, MSG_PEEK);
			return buffer;
		}

		auto		onAccept() {
			auto first = peek();

			if (first != SSL_IDENTIFIER) {
				_sslstream->setSslDisabled(true);
			}
		}
	private:
		Connection(boost::asio::io_service& ios):
				zany::Connection(),
				_sslctx{boost::asio::ssl::context::sslv23},
				_ssock{ios, _sslctx},
				_sslstream(&_ssock) {}

		
		boost::asio::ssl::context 		_sslctx;
    	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
										_ssock;
		boost::iostreams::stream<SslTcpIoStreamBidirectional>
										_sslstream;
		std::iostream					*_stream;
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
			Connection::fromZany(*nConnection).onAccept();
			onHandleAccept(nConnection);
			startAccept();
		}
	}

	std::function<void (zany::Connection::SharedInstance nConnection)>
									onHandleAccept;

	boost::asio::ip::tcp::acceptor	_acceptor;
};

}