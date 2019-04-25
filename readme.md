# How To Compile

- ### Linux (require docker installed)
    ``` bash
	cd "$path_to_zia";
	./compileDebug.sh -- -j 8;
	./build-Linux/bin/zia --config config.json;
	```
- ### Windows (require Visual Studio, conan, git bash and cmake installed)
	``` bash
	cd "$path_to_zia";
	./compileDebug.sh --nodocker -j 8;
	./build-MINGW64_NT-10.0/bin/zia --config config.json;
	```
	#### Download links:
	- [Visual Studio Community](https://visualstudio.microsoft.com/fr/vs/community/)
	- [git bash](https://gitforwindows.org/)
	- cmake [x64](https://github.com/Kitware/CMake/releases/download/v3.13.3/cmake-3.13.3-win64-x64.msi) | [x86](https://github.com/Kitware/CMake/releases/download/v3.13.3/cmake-3.13.3-win32-x86.msi)
	- [conan](https://conan.io/)
- ### MacOs
	Not tested yet :/q