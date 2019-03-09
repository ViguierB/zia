//
// Created by seb on 06/03/19.
//

#pragma once

#include <string>
#include <memory>
#include <thread>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "Zany/Orchestrator.hpp"

namespace zia {
namespace manager {

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
	TcpConnection(zany::Orchestrator &master): _master(master) {}

	typedef std::shared_ptr<TcpConnection> SharedInstance;

	static SharedInstance create(zany::Orchestrator &master) {
		return std::shared_ptr<TcpConnection>(new TcpConnection(master));
	}

	void	start(std::string &&line) {
		if (line == "LIST") {
			for (auto const &module : const_cast<zany::Loader&>(_master.getLoader())) {
				_stream << module.name() << '\n';
			}
		} else if (line.find("UNLOAD") == 0) {
			_unload(line);
		} else if (line.find("LOAD") == 0) {
			_load(line);
		}
	}

	auto	getResponse() { return _stream.str(); }

private:
	void	_unload(std::string &line);
	void	_load(std::string &line);

	zany::Orchestrator	&_master;
	std::ostringstream	_stream;
};

}
}
