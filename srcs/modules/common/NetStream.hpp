/*
** EPITECH PROJECT, 2018
** zia
** File description:
** NetStream.hpp
*/

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <csignal>
#include <cstring>
#include "Zany/Platform.hpp"
#include "ModulesUtils.hpp"

#define SSL_IDENTIFIER (22)

namespace zia {

struct SslUtils {
	~SslUtils() {
		if (ssl) {
#			if defined(ZANY_ISUNIX)
				sigset_t origmask;
				sigset_t sigmask;

				sigemptyset(&sigmask);
				sigaddset(&sigmask, SIGPIPE);

				sigprocmask(SIG_BLOCK, &sigmask, &origmask);
#			endif
			::SSL_shutdown(ssl);
#			if defined(ZANY_ISUNIX)
				sigprocmask(SIG_SETMASK, &origmask, NULL);
#			endif
			::SSL_free(ssl);
		}
		if (free_ctx) {
			::SSL_CTX_free(ref_ctx);
		}
	}

	SSL_CTX	*ref_ctx;
	SSL		*ssl;
	bool	free_ctx = false;
};

template<typename NativeSocket>
class SslTcpBidirectionalIoStream {
private:

	static inline auto	**getThis() {
		thread_local SslTcpBidirectionalIoStream	*thisRef = nullptr;
		thread_local bool							first = false;

		if (!first) {
			first = true;
			signal(SIGPIPE, &_sigpipeCatchHandler);
		}

		return &thisRef;
	}

#if defined(ZANY_ISUNIX)
	template<typename Handler>
	inline std::streamsize	sigWrapper(Handler &&hdl) {
		if (_error) return 0;
		auto **__this = getThis();
		sigset_t origmask;
		sigset_t sigmask;

		*__this = this;
		sigemptyset(&sigmask);
		sigaddset(&sigmask, SIGPIPE);

		sigprocmask(SIG_BLOCK, &sigmask, &origmask);
		auto res = hdl();
		sigprocmask(SIG_SETMASK, &origmask, NULL);
		return res;
	}
#else
	template<typename Handler>
	inline std::streamsize	sigWrapper(Handler &&hdl) {
		return hdl();
	}
#endif

	static void _sigpipeCatchHandler(int) {
		auto **__this = getThis();

		(*__this)->_error = true;
	}

	auto	peek() {
		char buffer = 0;
		::recv(_nsocket, &buffer, 1, MSG_PEEK);
		return buffer;
	}
public:
	typedef char										char_type;
	typedef boost::iostreams::bidirectional_device_tag	category;

	SslTcpBidirectionalIoStream(NativeSocket ssls, SSL_CTX *_baseCtx = nullptr)
	: _nsocket(ssls), _disableSsl(false), _pos(0) {
		auto tv = ModuleUtils::getTimeout(15);
		setsockopt(_nsocket, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(tv));

		auto	first = peek();

		if (first != SSL_IDENTIFIER) {
			setSslDisabled(true);
		} else {
			ssl = std::make_shared<SslUtils>();
			ssl->ref_ctx = _baseCtx;
			ssl->ssl = ::SSL_new(_baseCtx);
		}
	}

	SslTcpBidirectionalIoStream(NativeSocket ssls, bool sslOn)
	: _nsocket(ssls), _disableSsl(!sslOn), _pos(0) {
		auto tv = ModuleUtils::getTimeout(5);
		setsockopt(_nsocket, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(tv));

		if (sslOn) {
			ssl = std::make_shared<SslUtils>();
			ssl->ref_ctx = ::SSL_CTX_new(::SSLv23_client_method());
			ssl->ssl = ::SSL_new(ssl->ref_ctx);
			ssl->free_ctx = true;
		}
	}

	void	connect(std::string &virtualhost) {
		::SSL_set_tlsext_host_name(ssl->ssl, virtualhost.c_str());

		if (::SSL_connect(ssl->ssl) < 0)
			throw std::runtime_error(std::string("OpenSSL: ") + ERR_error_string(ERR_get_error(), nullptr));
	}

	std::streamsize write(const char* s, std::streamsize n) {
		return sigWrapper([&] () -> std::streamsize { 
			if (_disableSsl) {
				return ::send(_nsocket, s, n, 0);
			} else {
				return ::SSL_write(ssl->ssl, s, n);
			}
		});
	}

	std::streamsize read(char* s, std::streamsize n) {
		return sigWrapper([&] () -> std::streamsize {
			if (_disableSsl) {
				return ::recv(_nsocket, s, n, 0);
			} else {
				return ::SSL_read(ssl->ssl, s, n);
			}
		});
	}

	void	setSslDisabled(bool status) { _disableSsl = status; }
	auto	isSslDisabled() { return _disableSsl; }
	auto	nativeSocket() { return _nsocket; }

	std::shared_ptr<SslUtils>	ssl;
private:
	NativeSocket		_nsocket;
	bool				_disableSsl;
	std::streamsize		_pos;
	bool				_error = false;
};

}
