# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.24

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = C:\tools\CMake\bin\cmake.exe

# The command to remove a file.
RM = C:\tools\CMake\bin\cmake.exe -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\Users\symon\Documents\Projects\Drone\Dron

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\Users\symon\Documents\Projects\Drone\Dron\build

# Utility rule file for main.bin.

# Include any custom commands dependencies for this target.
include CMakeFiles/main.bin.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/main.bin.dir/progress.make

CMakeFiles/main.bin:
	arm-none-eabi-objcopy -O binary -S C:/Users/symon/Documents/Projects/Drone/Dron/bin/main.elf C:/Users/symon/Documents/Projects/Drone/Dron/bin/main.bin

main.bin: CMakeFiles/main.bin
main.bin: CMakeFiles/main.bin.dir/build.make
.PHONY : main.bin

# Rule to build all files generated by this target.
CMakeFiles/main.bin.dir/build: main.bin
.PHONY : CMakeFiles/main.bin.dir/build

CMakeFiles/main.bin.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\main.bin.dir\cmake_clean.cmake
.PHONY : CMakeFiles/main.bin.dir/clean

CMakeFiles/main.bin.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\Users\symon\Documents\Projects\Drone\Dron C:\Users\symon\Documents\Projects\Drone\Dron C:\Users\symon\Documents\Projects\Drone\Dron\build C:\Users\symon\Documents\Projects\Drone\Dron\build C:\Users\symon\Documents\Projects\Drone\Dron\build\CMakeFiles\main.bin.dir\DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/main.bin.dir/depend
