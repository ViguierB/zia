/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <fstream>
#include <thread>
#include "./Listener.hpp"
#include "Zany.hpp"

namespace zia {

class CoreSslModule : public zany::Loader::AbstractModule {
public:
	CoreSslModule(): _signals(_ios) {
		_signals.add(SIGINT);
		_signals.add(SIGTERM);
	#if defined(SIGQUIT)
		_signals.add(SIGQUIT);
	#endif // defined(SIGQUIT)
		_signals.async_wait(boost::bind(&CoreSslModule::_onSignal, this));
	}

	~CoreSslModule() {
		_ios.stop();
		if (_t) {
			_t->join();
		}
	}

	virtual auto	name() const -> const std::string&
		{ static const std::string name("CoreSsl"); return name; }
	virtual void	init() {};
	virtual bool	isACoreModule() final { return true; }
	virtual void	startListening(std::vector<std::uint16_t> &ports) final;
private:
	void	_onSignal();
	void	_listening(std::condition_variable &cv);
	void	_startPipeline(zany::Connection::SharedInstance c);

	std::vector<std::uint16_t>		_ports;
	boost::asio::io_context			_ios;
	std::unique_ptr<std::thread>	_t;
	boost::asio::signal_set			_signals;
};

void	CoreSslModule::_onSignal() {
	std::cout << "Closing..." << std::endl;
	
	zany::evt::Manager::get()["onClose"]->fire();
	this->master->getContext().stop();
}

void	CoreSslModule::_listening(std::condition_variable &cv) {
	auto &ports = _ports;
	
	std::vector<Listener>	acceptors;

	acceptors.reserve(ports.size());
	for (auto port: ports) {
		auto 			&v6listener = acceptors.emplace_back(boost::asio::ip::tcp::v6(), _ios, port);

		v6listener.onHandleAccept = [this] (zany::Connection::SharedInstance c) {
			boost::asio::ip::v6_only option;

			Listener::Connection::fromZany(*c).socket().get_option(option);
			if (option.value() == true) {
				boost::asio::ip::v6_only noption(false);

				Listener::Connection::fromZany(*c).socket().set_option(noption);
			}
			this->master->getContext().addTask(std::bind(&CoreSslModule::_startPipeline, this, c));
		};

		v6listener.startAccept();
		std::cout << "Starting listener on port " << port << std::endl;
	}

	std::this_thread::yield();
	cv.notify_all();
	_ios.run();
}

void	CoreSslModule::_startPipeline(zany::Connection::SharedInstance c) {
	constexpr auto sp = &zany::Orchestrator::startPipeline;

	try {
		std::cout << "hummm-1" << std::endl;
		(this->master->*sp)(c);
		std::cout << "hummm-2" << std::endl;
	} catch (std::exception &e) {
		std::cerr
			<< "Connection rejected because of internal server error:\n\t"
			<< e.what() << std::endl;
	}
}

void	CoreSslModule::startListening(std::vector<std::uint16_t> &ports) {
	std::mutex						mtx;
	std::unique_lock<decltype(mtx)>	ulock(mtx);
	std::condition_variable			cv;

	_ports = ports;
	_t = std::make_unique<std::thread>(std::bind(&CoreSslModule::_listening, this, std::ref(cv)));
	cv.wait(ulock);
}

}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::CoreSslModule();
}