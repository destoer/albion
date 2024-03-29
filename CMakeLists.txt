#project config

cmake_minimum_required(VERSION 3.10)

project(albion)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


#set(FRONTEND "QT")
set(FRONTEND "SDL")
#set(FRONTEND "IMGUI")
#set(FRONTEND "DESTOER")

# external frontend
#set(FRONTEND "HEADLESS")


#add_definitions(-DBOUNDS_CHECK)
if(${FRONTEND} STREQUAL "IMGUI")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_IMGUI -DIMGUI_IMPL_OPENGL_LOADER_GLEW -DDEBUG -DLOG_CONSOLE)

	set(GB "TRUE")
	set(GBA "TRUE")
	set(N64 "TRUE")

endif()

if(${FRONTEND} STREQUAL "SDL")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DDEBUG -DCONTROLLER_SDL -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_SDL)

	set(N64 "TRUE")
	#set(GB "TRUE")
	#set(GBA "TRUE")
endif()


if(${FRONTEND} STREQUAL "DESTOER")
	add_definitions(-DAUDIO_ENABLE -DSDL_REQUIRED -DAUDIO_SDL -DCONTROLLER_SDL -DDEBUG -DFETCH_SPEEDHACK)
	add_definitions(-DFRONTEND_DESTOER)
endif()

if(${FRONTEND} STREQUAL "HEADLESS")
	add_definitions(-DFRONTEND_HEADLESS)
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
include_directories("src/headers")
include_directories("src/common/thirdparty")

include_directories("src")

#destoer lib
include_directories("destoer-lib/src/")
add_definitions(-DFORWARD_DEF_DESTOER_LIB)

if(${FRONTEND} STREQUAL "IMGUI")
	include_directories("/usr/include/SDL2")
	include_directories("src/common/thirdparty/imgui")
	file(GLOB frontend_files
		"src/frontend/imgui/*.cpp"
		"src/common/thirdparty/imgui/*.cpp"
		"src/common/thirdparty/imgui/backends/imgui_impl_sdl2.cpp"
		"src/common/thirdparty/imgui/backends/imgui_impl_opengl3.cpp"	
	)
endif()


if(${FRONTEND} STREQUAL "SDL")
	file(GLOB frontend_files
		"src/frontend/sdl/sdl_window.cpp"
	)
endif()

if(${FRONTEND} STREQUAL "DESTOER")
	file(GLOB frontend_files
		"src/frontend/destoer/*.cpp"
	)

	include_directories("src/frontend/destoer")
endif()



add_executable(albion  ${src_files} ${frontend_files})

# make these paths less of a pain to modify :)

if(WIN32)
	if(${FRONTEND} STREQUAL "IMGUI")  # needed only for imgui 
		# external libs

		# opengl
		find_package(OPENGL REQUIRED)
		target_link_libraries(albion OpenGL::GL)

		# glew
		list(APPEND CMAKE_PREFIX_PATH "C:/glew-2.1.0")
		find_package(GLEW REQUIRED)
		target_link_libraries(albion GLEW::GLEW)

		# for some reason building is broken and extra steps are required
		add_library(GLEW SHARED IMPORTED)
		set_target_properties(GLEW PROPERTIES
			IMPORTED_IMPLIB "C:/glew-2.1.0/lib/Release/x64/glew32.lib"
			LINKER_LANGUAGE C
		)
		target_link_libraries(albion GLEW)

	endif()


	# sdl (allways needed)
    add_library(SDL2 SHARED IMPORTED)
    set_target_properties(SDL2 PROPERTIES
        IMPORTED_LOCATION "C:/SDL2-2.0.14/lib/x64/SDL2.dll"
        IMPORTED_IMPLIB "C:/SDL2-2.0.14/lib/x64/SDL2.lib"
        LINKER_LANGUAGE C
    )
	target_link_libraries(albion SDL2)
	
    include_directories("C:/SDL2-2.0.14/include")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror -Wall -Wextra -march=native -pthread -O1 -fomit-frame-pointer -Wno-missing-braces")

	if(${FRONTEND} STREQUAL "HEADLESS")

	else ()
		target_link_libraries(albion SDL2)
	endif()
endif()





if(DEFINED N64)
	add_definitions(-DN64_ENABLED)
	add_library(n64 STATIC "src/n64/n64.cpp")
	include_directories("beyond-all-repair/src")

	target_link_libraries(albion n64)
endif()

if(DEFINED GBA)
	file(GLOB gba_files
		"src/gba/*.cpp"
		"src/gba/cpu/*.cpp"
		"src/gba/memory/*.cpp"
		"src/gba/ppu/*.cpp"
		"src/gba/apu/*.cpp"
	)

	add_definitions(-DGBA_ENABLED)
	add_library(gba STATIC ${gba_files})

	# TODO: gba requires gb for psg support 
	# we need to split the files up

	target_link_libraries(albion gba)
endif()

if(DEFINED GB)
	file(GLOB gb_files
		"src/gb/*.cpp"	
		"src/gb/cpu/*.cpp"
		"src/gb/apu/*.cpp"
		"src/gb/ppu/*.cpp"
		"src/gb/memory/*.cpp"
	)
	add_definitions(-DGB_ENABLED)

	add_library(gb STATIC ${gb_files})


	target_link_libraries(albion gb)
endif()

# build gb psg in seperate lib as two cores depend on it
if(DEFINED GB OR DEFINED GBA)
	file(GLOB psg_files
		"src/gb/apu/psg/*cpp"
	)

	add_library(psg STATIC ${psg_files})

	target_link_libraries(albion psg)
endif()


if(${FRONTEND} STREQUAL "IMGUI") 
	target_link_libraries(albion GL GLEW)
endif()
