FetchContent_Declare(
	SDL3
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG b5c3eab6b447111d3c7879bb547b80fb4abd9063
)

set (SDL_TEST_LIBRARY OFF CACHE BOOL "")
 
FetchContent_MakeAvailable(SDL3)
