The project leverages CMake, a cross-platform build management tool.
Most popular IDEs, like CLion and Visual Studio, understand and support CMake based projects. 

The C++ project structure I have chosen to follow is detailed on [Project Heuristics](https://medium.com/heuristics/c-application-development-part-1-project-structure-454b00f9eddc).
The C++ convention for CMake builds is what is called an "out of source" build, meaning you give room for your build to generate artifacts that dont clutter your source directories. 
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