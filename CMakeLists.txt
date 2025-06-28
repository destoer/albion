#project config

cmake_minimum_required(VERSION 3.10)

project(albion)
include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(FRONTEND "SDL")

# external frontend

include("thirdparty/spdlog.cmake")
# include("thirdparty/sdl3.cmake")

# set(GB "TRUE")
# set(GBA "TRUE")
set(N64 "TRUE")

# add_compile_options(-fsanitize=undefined)
# add_link_options(-fsanitize=undefined)

#add_definitions(-DBOUNDS_CHECK)


if(${FRONTEND} STREQUAL "SDL")
	add_definitions(-DFETCH_SPEEDHACK)
endif()


# source files 

# main source files
file(GLOB src_files
	"src/main.cpp"
	"src/common/albion/*.cpp"
	"src/frontend/gb/*.cpp"
	"src/frontend/gba/*.cpp"
	"src/frontend/*.cpp"
)

# add some includes
include_directories("src/common/albion/headers")
include_directories("src/common/thirdparty")
include_directories("src/")

if(${FRONTEND} STREQUAL "SDL")
	file(GLOB frontend_files
		"src/frontend/sdl/sdl_window.cpp"
	)
endif()


add_executable(albion  ${src_files} ${frontend_files})

if(WIN32)

else()
	# prevent O3 being injected
	SET( CMAKE_CXX_FLAGS_DEBUG  "")
	SET( CMAKE_CXX_FLAGS_RELEASE  "")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -march=native -pthread -O3 -fomit-frame-pointer -Wno-missing-braces -fmax-errors=15")
endif()

add_subdirectory("destoer-lib")

if(DEFINED N64)
	add_subdirectory("src/n64")
	add_subdirectory("beyond-all-repair")
	target_link_libraries(albion PUBLIC n64)
endif()

# build gb psg in seperate lib as two cores depend on it
if(DEFINED GB OR DEFINED GBA)
	add_subdirectory("src/psg")
endif()

if(DEFINED GBA)
	add_subdirectory("src/gba")
	target_link_libraries(albion PUBLIC gba)
endif()

if(DEFINED GB)
	add_subdirectory("src/gb")
	target_link_libraries(albion PUBLIC gb)
endif()

target_link_libraries(albion PRIVATE destoer spdlog::spdlog SDL3)