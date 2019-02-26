/*
** EPITECH PROJECT, 2018
** zia
** File description:
** Module.cpp
*/

#include <fstream>
#include <thread>
#include <memory>
#include "Zany/Loader.hpp"

namespace zia {

class ManagerModule : public zany::Loader::AbstractModule {
public:
	~ManagerModule() {
		_continue = false;
		if (_worker.get()) {
			_worker->join();
		}
	}

	virtual auto	name() const -> const std::string&
		{ static const std::string name("ManagerModule"); return name; }
	virtual void	init() final;
private:
	void	_verifConfig() {
		auto &config = this->master->getConfig();

		try {
			if (config["manager"]["listening-port"].isNumber()) {
				ManagerModule::_basicConfig["manager"]["listening-port"] = config["manager"]["listening-port"]
			}
		} catch (...) {}
	}

	void	_entrypoint();

	static inline _basicConfig = zany::makeObject {
		{ "manager", zany::makeObject {
			{ "listening-port", 5768 }
		} }
	}

	std::unique_ptr<std::thread>	_worker;
	bool							_continue = true;
};

void	ManagerModule::init() {
	_verifConfig();

	_worker = std::make_unique<decltype(_worker)::element_type>(
		std::bind(&ManagerModule::_entrypoint, this));
}

void	ManagerModule::_entrypoint() {


}

extern "C" ZANY_DLL
zany::Loader::AbstractModule	*entrypoint() {
	return new zia::ManagerModule();
}