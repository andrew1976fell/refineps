# CMakeLists.txt — top-level ESP-IDF project file
#
# Boilerplate required by ESP-IDF. Sets the project name (refine_firmware),
# which determines the output binary name: build/refine_firmware.bin
#
# All source files and IDF component dependencies are declared in
# main/CMakeLists.txt — edit that file when adding source files or components.
#
# Related:
#   firmware/main/CMakeLists.txt    — source file list and component dependencies
#   firmware/CLAUDE.md              — build commands and rules
#   firmware/notes/source-map.md    — what each source file does

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(refine_firmware)
