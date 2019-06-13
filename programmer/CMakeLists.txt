cmake_minimum_required (VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin/)

project (odc_prog)

include_directories(../libs/plog/include/)
include_directories(../libs/CRCalc/)
include_directories(../libs/argparse/)

set (CMAKE_CXX_STANDARD 11)

add_executable(odc_prog
	./src/i2c.cpp
	./src/devices/device.cpp
	./src/devices/rtd2660.cpp
	./src/flash.cpp
	./src/main.cpp
)

target_link_libraries(odc_prog i2c)
