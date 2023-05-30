The project leverages CMake, a cross-platform build management tool.
Most popular IDEs, like CLion and Visual Studio, understand and support CMake based projects. 

The C++ project structure I have chosen to follow is detailed on [Project Heuristics](https://medium.com/heuristics/c-application-development-part-1-project-structure-454b00f9eddc).
The C++ convention for CMake builds is what is called an "out of source" build, meaning you give room for your build to generate artifacts that don't clutter your source directories. 
The `.gitignore` has been tuned for out of source builds to happen in the `./build/` dir. 

Consider using a pattern like the one below for your builds should you want to build the project in a non-IDE, command line way.

*nix environments
```sh
mkdir -p build
cd build
cmake ..
make
```

windows ms visual studio environments
```sh
\path\to\VsDevCmd.bat
mkdir build
cd build
cmake ..
devenv bmai.sln /build
```


------

UPDATE: I have learned about a more modern approach to using CMake. There are 2 patterns that are becoming common. 

For command line only builds
```sh
echo configuring Debug and Release dirs
cmake -S . -B build/Debug -D CMAKE_BUILD_TYPE=Debug
cmake -S . -B build/Release -D CMAKE_BUILD_TYPE=Release

echo building Debug and Release binaries
cmake --build build/Debug
cmake --build build/Release
```

and for a multi-config / IDE based approach
```sh
echo configuring build dir
cmake -S . -B build
echo you can point your IDE at build/ dir

echo or build from command line
cmake --build build --config Debug
echo for a subsequent Release build you will need to rebuild the build/ dir
cmake -S . -B build
cmake --build build --config Release
```

TODO: add notes about buidling and execute the `bmai_tests` target.