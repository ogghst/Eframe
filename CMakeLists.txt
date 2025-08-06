# CMakeLists.txt for eink-dashboard project

# Boilerplate for ESP-IDF projects
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name
project(eink-dashboard)

# Embed the 'www' directory into a SPIFFS image.
# This image will be flashed to the 'storage' partition.
# The partition table must define a 'storage' partition.
spiffs_create_partition_image(storage www)
