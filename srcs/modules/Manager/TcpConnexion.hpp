//
// Created by seb on 06/03/19.
//

#pragma once

#include <string>
#include <iostream>
#include <boost/system/error_code.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

class TcpConnexion : public boost::enable_shared_from_this<TcpConnexion>
{
public:
	typedef boost::shared_ptr<TcpConnexion> pointer;

	static pointer create(boost::asio::io_service& io_service)
	{
		return pointer(new TcpConnexion(io_service));
	}

	boost::asio::ip::tcp::socket& socket()
	{
		return _socket;
	}

	void start()
	{
		boost::asio::async_read(_socket, boost::asio::buffer(_message),
				 boost::bind(&TcpConnexion::handle_write, this,
					     boost::asio::placeholders::error,
					     boost::asio::placeholders::bytes_transferred));
	}

private:
	TcpConnexion(boost::asio::io_service& io_service): _socket(io_service)
	{
	}

	void handle_write(const boost::system::error_code& e/*error*/,
			  size_t size)
	{
		if (e) {
			std::cout << e.message() << std::endl;
		}
		std::cout << size << std::endl;
		start();
	}

	boost::asio::ip::tcp::socket _socket;
	std::string _message;
};
