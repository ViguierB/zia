//
// Created by seb on 06/03/19.
//

#include <thread>
#include <boost/filesystem.hpp>
#include "TcpConnection.hpp"

namespace zia {
namespace manager {

void	TcpConnection::_unload(std::string &line) {
	std::stringstream comm;
	comm << line;
	comm >> line;

	while (comm.rdbuf()->in_avail() != 0) {
		line.clear();
		comm >> line;
		zany::Loader::AbstractModule *toRemove = nullptr;

		for (auto &module : const_cast<zany::Loader &>(_master.getLoader())) {
			if (module.name() == line) {
				toRemove = &module;
				break;
			}
		}
		if (!toRemove) {
			_stream << "Module " << line << " not found." << std::endl;
		} else {
			std::condition_variable		locker;
			std::mutex			mtx;
			std::unique_lock<decltype(mtx)>	ulock(mtx);

			_master.unloadModule(*toRemove,
				[&] {
					_stream
						<< "Unloading "
						<< line
						<< ": Ok"
						<< std::endl;
					locker.notify_all();
				},
				[&] (zany::Loader::Exception e) {
					_stream
						<< "Unloading "
						<< line
						<< ": Ko"
						<< std::endl
						<< e.what()
						<< std::endl;
					locker.notify_all();
				});
			locker.wait(ulock);
		}
	}
}

void	TcpConnection::_load(std::string &line) {
	std::stringstream comm;
	boost::filesystem::path path;
	comm << line;
	comm >> line;
	bool found = false;

	while (comm.rdbuf()->in_avail() != 0) {
		line.clear();
		comm >> line;
		found = false;
		path = line;

		if ((boost::filesystem::is_regular_file(path) || boost::filesystem::is_symlink(path))
		    && path.extension() ==
#if defined(ZANY_ISWINDOWS)
		       ".dll"
#else
		       ".so"
#endif
			) {
			std::condition_variable		locker;
			std::mutex			mtx;
			std::unique_lock<decltype(mtx)>	ulock(mtx);
			auto mp =
#if defined(ZANY_ISWINDOWS)
				it->path().lexically_normal();
#else
				boost::filesystem::path(
					(boost::filesystem::is_symlink(path)
					 ? boost::filesystem::read_symlink(path)
					 : path
					).string()
				).lexically_normal()
#endif
			;
			_master.loadModule(path.generic_string(), [&] (auto &module) {
				_stream << "Module " << module.name() << " loaded." << std::endl;
				locker.notify_all();
			}, [] (auto error) {
				std::cerr << error.what() << std::endl;
			});
			locker.wait(ulock);
			found = true;
		}
		if (!found)
			_stream << "Module " << line << " not found." << std::endl;
	}
}

}
}