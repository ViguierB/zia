/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Listener.hpp
*/

#pragma once

#include "../common/NetStream.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "Zany/Connection.hpp"
#include "Zany/Pipeline.hpp"
#include "Zany/Entity.hpp"
#include "Constants.hpp"

namespace zia {

struct	VirtualServersConfig {
	~VirtualServersConfig() {
		::SSL_CTX_free(ctx);
	}

	std::string		hostname;
	std::uint16_t	port;
	std::string		certificateChainFile;
	std::string		certificateFile;
	std::string		privateKeyFile;
	std::string		protocol;
	SSL_CTX			*ctx;

	void init() {
		// cctx = ::SSL_CONF_CTX_new();
		ctx = ::SSL_CTX_new(::SSLv23_server_method());
		if (!ctx) {
		 	throw std::runtime_error(std::string("OpenSSL: ") + ERR_error_string(ERR_get_error(), nullptr));
		}

		SSL_CTX_set_ecdh_auto(ctx, 1);

		if (!this->certificateChainFile.empty()) {
			if (::SSL_CTX_use_certificate_chain_file(ctx, this->certificateChainFile.c_str()) <= 0) {
				throw std::runtime_error(
					std::string("OpenSSL: SSL_CTX_use_certificate_chain_file: \n\t") +
					this->certificateFile + ": " +
					ERR_error_string(ERR_get_error(), nullptr)
				);
			}
		} else {
			if (::SSL_CTX_use_certificate_file(ctx, this->certificateFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
				throw std::runtime_error(
					std::string("OpenSSL: SSL_CTX_use_certificate_file: \n\t") +
					this->certificateFile + ": " +
					ERR_error_string(ERR_get_error(), nullptr)
				);
			}
		}

		if (::SSL_CTX_use_PrivateKey_file(ctx, this->privateKeyFile.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
			throw std::runtime_error(
				std::string("OpenSSL: SSL_CTX_use_PrivateKey_file: \n\t") +
				this->certificateFile + ": " +
				ERR_error_string(ERR_get_error(), nullptr)
			);
		}
	}

	void link(SSL *ssl) {
		::SSL_set_SSL_CTX(ssl, ctx);
	}
};

class Listener {
public:
	class Connection: zany::Connection {
	public:
		~Connection() {}

		template<typename ...Args>
		static auto make(Args &&...args) {
			return SharedInstance(reinterpret_cast<zany::Connection*>(
				new Connection(std::forward<Args>(args)...)
			));
		}

		virtual std::iostream	&stream() final { return *_stream; }

		auto		*parent() { return _parent; }
		auto		&socket() { return _socket; }
		auto		nativeSocket() { return (*_sslstream)->nativeSocket(); }
		static auto	&fromZany(zany::Connection &self) { return reinterpret_cast<Connection&>(self); }

		void	doHandshake(zany::Pipeline::Instance &instance);

		auto		onAccept() {
			_sslstream = std::make_unique<decltype(_sslstream)::element_type>(_socket.native_handle(), _parent->_baseCtx);
			_stream = std::make_unique<decltype(_stream)::element_type>(static_cast<std::streambuf*>(&*_sslstream));
		}
	private:
		Connection(boost::asio::io_service& ios, Listener *parent):
				zany::Connection(),
				_socket(ios),
				_parent(parent) {}

		boost::asio::ip::tcp::socket	_socket;
		std::unique_ptr<boost::iostreams::stream_buffer<SslTcpBidirectionalIoStream<boost::asio::detail::socket_type>>>
										_sslstream;
		std::unique_ptr<std::iostream>	_stream;
		Listener						*_parent;
		std::mutex						_handshakeMutex;
	};


	template<typename PVERSION>
	Listener(PVERSION pv, boost::asio::io_service& ios, std::uint16_t port)
	: _acceptor(ios, boost::asio::ip::tcp::endpoint(std::forward<PVERSION>(pv), port)) {
		_baseCtx = ::SSL_CTX_new(::SSLv23_server_method());

		if (!_baseCtx) {
			throw std::runtime_error(std::string("OpenSSL: ") + ERR_error_string(ERR_get_error(), nullptr));
		}
		SSL_CTX_set_ecdh_auto(_baseCtx, 1);
	}

	~Listener() {
		if (_baseCtx) SSL_CTX_free(_baseCtx);
		if (_baseCtxConf) SSL_CONF_CTX_free(_baseCtxConf);
	}

	void startAccept() {
		auto nc = Connection::make(_acceptor.get_io_service(), this);

		_acceptor.async_accept(Connection::fromZany(*nc).socket(),
			boost::bind(&Listener::handleAccept, this, nc,
			boost::asio::placeholders::error));
	}

	void handleAccept(zany::Connection::SharedInstance nConnection, const boost::system::error_code& error) {
		if (!error) {
			auto &&remoteEp = Connection::fromZany(*nConnection).socket().remote_endpoint();
			auto &&localEp = Connection::fromZany(*nConnection).socket().local_endpoint();
			auto &&remoteAd = remoteEp.address();
			nConnection->info.ip = remoteAd.to_string();
			nConnection->info.port = remoteEp.port();
			nConnection->info.localPort = localEp.port();

			std::cout
				<< boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) << ": "
				<< "New Connection from: " << '[' << nConnection->info.ip << "]:" << nConnection->info.port
					<< std::endl;
			Connection::fromZany(*nConnection).onAccept();
			onHandleAccept(nConnection);
			startAccept();
		}
	}

	void	initVHostConfig(zany::Entity const &cfg) {
		try {
			auto &vHosts = cfg["server"].value<zany::Array>();

			for (auto &vhost: vHosts) {
				try {
					auto it = vhost.value<zany::Object>().find("ssl");
					if (it == vhost.value<zany::Object>().end())
						continue;

					auto &sslc = it->second;
					auto targetPort = (std::uint16_t)vhost["port"].to<int>();
					if (targetPort != _acceptor.local_endpoint().port())
						continue;
					
					
					std::string const *chainfile = nullptr;
					std::string const *certfile = nullptr;
					std::string const *proto = nullptr;
					try { chainfile = &sslc["certificate-chain"].value<zany::String>(); } catch (...) {} 
					try { certfile = &sslc["certificate"].value<zany::String>(); } catch (...) {}
					try { proto = &sslc["protocol"].value<zany::String>(); } catch (...) {}
					
					auto vhit = vhostsConfigs.emplace(
						std::pair<std::string, VirtualServersConfig>(
							vhost["host"].value<zany::String>(),
							VirtualServersConfig {
								vhost["host"].value<zany::String>(),
								targetPort,
								(chainfile) ? *chainfile : "",
								(certfile) ? *certfile : "",
								sslc["private-key"].value<zany::String>(),
								(proto) ? *proto : constant::defSslProtocol
							}
						)
					);
					vhit->second.init();
				} catch (std::exception const &e) {
					std::cerr << e.what() << std::endl;
				}
			}
		} catch (...) {}
	}

	std::function<void (zany::Connection::SharedInstance nConnection)>
									onHandleAccept;

	boost::asio::ip::tcp::acceptor	_acceptor;
	std::unordered_multimap<std::string, VirtualServersConfig>
									vhostsConfigs;
	SSL_CTX							*_baseCtx = nullptr;
	SSL_CONF_CTX					*_baseCtxConf = nullptr;
};

void Listener::Connection::doHandshake(zany::Pipeline::Instance &pipeline) {
	if (!(*_sslstream)->isSslDisabled()) {
		pipeline.request.port = _parent->_acceptor.local_endpoint().port();

		struct Cap {
			Connection					*co;
			zany::Pipeline::Instance	*pipeline;
		} data {
			this,
			&pipeline
		};
		
		typedef int (*callback_t)(SSL*,int*,decltype(data)*);
		static callback_t callback = [] (SSL *ssl, int *, decltype(data) *cap) -> int {
			try {
				const char *hostname = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
				
				if (!hostname || !cap || !cap->co || !cap->co->parent()) {
					return -1;
				}
				auto vhit = cap->co->parent()->vhostsConfigs.find(hostname);
				if (vhit == cap->co->parent()->vhostsConfigs.end()) return 1;
				
				vhit->second.link(ssl);

				return 0;
			} catch (...) { return -1; }
		};

		auto &sslData = *(*_sslstream)->ssl;

		::SSL_set_fd(sslData.ssl, nativeSocket());

		::SSL_CTX_set_tlsext_servername_arg(sslData.ref_ctx, &data);
		::SSL_CTX_set_tlsext_servername_callback(sslData.ref_ctx, callback);

		if (::SSL_accept(sslData.ssl) <= 0) {
			throw std::runtime_error(std::string("OpenSSL: ") + ERR_error_string(ERR_get_error(), nullptr));
		}

	}
}

}
