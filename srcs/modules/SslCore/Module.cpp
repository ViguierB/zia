/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <fstream>
#include <thread>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include "./Listener.hpp"
#include "Zany.hpp"

namespace zia {

class CoreSslModule : public zany::Loader::AbstractModule {
public:
	CoreSslModule() {}

	~CoreSslModule();

	virtual auto	name() const -> const std::string&
		{ static const std::string name("CoreSsl"); return name; }
	virtual void	init();
	virtual bool	isACoreModule() final { return true; }
	virtual void	startListening(std::vector<std::uint16_t> &ports) final;
private:
	void	_onSignal();
	void	_listening(std::condition_variable &cv);
	void	_startPipeline(zany::Connection::SharedInstance c);

	std::vector<std::uint16_t>					_ports;
	std::unique_ptr<boost::asio::io_context>	_ios;
	std::unique_ptr<std::thread>				_t;
};

void	CoreSslModule::init() { 
	SSL_library_init();
	OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();
	ERR_load_crypto_strings();
}

CoreSslModule::~CoreSslModule() {
	if (_ios) _ios->stop();
	if (_t) _t->join();
	
	FIPS_mode_set(0);
	ENGINE_cleanup();
	CONF_modules_unload(1);
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
}

void	CoreSslModule::_onSignal() {
	std::cout << "Closing..." << std::endl;
	
	zany::evt::Manager::get()["onClose"]->fire();
	this->master->getContext().stop();
}

void	CoreSslModule::_listening(std::condition_variable &cv) {
	_ios = std::make_unique<decltype(_ios)::element_type>();
	boost::asio::signal_set	signals(*_ios);
	signals.add(SIGINT);
	signals.add(SIGTERM);
#	if defined(SIGQUIT)
		signals.add(SIGQUIT);
#	endif // defined(SIGQUIT)

	auto &ports = _ports;
	
	std::vector<std::unique_ptr<Listener>>	acceptors;

	acceptors.reserve(ports.size());
	for (auto port: ports) {
		auto	&v6listener = acceptors.emplace_back(
			std::make_unique<Listener>(boost::asio::ip::tcp::v6(), *_ios, port)
		);

		v6listener->onHandleAccept = [this] (zany::Connection::SharedInstance c) {
			// boost::asio::ip::v6_only option;

			// Listener::Connection::fromZany(*c).socket().get_option(option);
			// if (option.value() == true) {
			// 	boost::asio::ip::v6_only noption(false);

			// 	Listener::Connection::fromZany(*c).socket().set_option(noption);
			// }
			this->master->getContext().addTask(std::bind(&CoreSslModule::_startPipeline, this, c));
		};

		v6listener->initVHostConfig(master->getConfig());
		v6listener->startAccept();
		std::cout << "Starting listener on port " << port << std::endl;
	}

	std::this_thread::yield();
	cv.notify_all();

	signals.async_wait(boost::bind(&CoreSslModule::_onSignal, this));
	_ios->run();
}

void	CoreSslModule::_startPipeline(zany::Connection::SharedInstance c) {
	constexpr auto	sp = &zany::Orchestrator::startPipeline;
	auto			&connection = Listener::Connection::fromZany(*c);

	try {
		(this->master->*sp)(c, std::bind(&Listener::Connection::doHandshake, &connection, std::placeholders::_1));
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