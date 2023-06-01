## TOOLING

The project leverages CMake, a cross-platform build management tool.
Most popular IDEs, like CLion, Xcode, and Visual Studio, understand and support CMake based projects. 

The C++ project structure I have chosen to follow is detailed on [Project Heuristics](https://medium.com/heuristics/c-application-development-part-1-project-structure-454b00f9eddc).
The C++ convention for CMake builds is what is called an "out of source" build, meaning you give room for your build to generate artifacts that don't clutter your source directories. 
The `.gitignore` has been tuned for out of source builds to happen in the `./build/` dir. 

Consider using a pattern like the one below for your builds should you want to build the project in a non-IDE, command line way.

## COMPATIBILITY

We are coding to the lowest common denominator of the contributors. This is currently:

* Visual Studio (15) 2017 (which supports up to CMake 3.12)
* C++ 14
* CMake 3.11

There are lots of nice features added in later versions of CMake (3.14+) and C++ (17+) that we will work to progress towards. 

## COMPILING 
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

UPDATE: I have learned about a more modern approach to using CMake. There are 2 patterns that are becoming common. However some of these commands may only work on later versions of cmake. 

For command line only builds
```sh
echo configuring, building, testing Debug
cmake -S . -B build/Debug -D CMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
ctest --test-dir build/Debug

echo configuring, building, testing Release
cmake -S . -B build/Release -D CMAKE_BUILD_TYPE=Release
cmake --build build/Release
ctest --test-dir build/Release
```

and for a multi-config / IDE based approach
```sh
echo configuring for IDE
cmake -S . -B build -G Xcode
# OR
cmake -S . -B build -G 'Visual Studio 17 2022'

echo or build from command line
cmake --build build --config Debug
ctest --test-dir build

echo for a subsequent Release build you will need to rebuild the build/ dir
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build
```
