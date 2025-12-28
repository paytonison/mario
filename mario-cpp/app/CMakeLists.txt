add_executable(mario
  src/main.cpp
)

target_link_libraries(mario PRIVATE mario::core)
target_compile_features(mario PRIVATE cxx_std_20)

if(MARIO_CPP_BUILD_SDL_APP)
  include(FetchContent)

  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.10
  )
  FetchContent_MakeAvailable(SDL2)

  add_executable(mario_sdl
    src/sdl_main.cpp
  )
  target_link_libraries(mario_sdl PRIVATE mario::core SDL2::SDL2-static)
  target_compile_features(mario_sdl PRIVATE cxx_std_20)
endif()

