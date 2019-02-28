/*
** EPITECH PROJECT, 2018
** zia
** File description:
** ModulesUtils.hpp
*/

#pragma once

#include "Zany/HttpBase.hpp"

namespace zia {

struct ModuleUtils {
	static auto getMethodFromString(std::string &method) {
		if (method == "GET") {
			return zany::HttpRequest::RequestMethods::GET;
		} else if (method == "POST") {
			return zany::HttpRequest::RequestMethods::POST;
		} else if (method == "HEAD") {
			return zany::HttpRequest::RequestMethods::HEAD;
		} else if (method == "OPTION") {
			return zany::HttpRequest::RequestMethods::OPTIONS;
		} else if (method == "PATCH") {
			return zany::HttpRequest::RequestMethods::PATCH;
		} else if (method == "CONNECT") {
			return zany::HttpRequest::RequestMethods::CONNECT;
		} else if (method == "PUT") {
			return zany::HttpRequest::RequestMethods::PUT;
		} else if (method == "TRACE") {
			return zany::HttpRequest::RequestMethods::TRACE;
		} else if (method == "DELETE") {
			return zany::HttpRequest::RequestMethods::DELETE;
		}
		return zany::HttpRequest::RequestMethods::ERROR;
	}
};

}