add_executable(mario
  src/main.cpp
)

target_link_libraries(mario PRIVATE mario::core)
target_compile_features(mario PRIVATE cxx_std_20)

function(mario_copy_runtime_dir target src_dir dst_name)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${src_dir}"
      "$<TARGET_FILE_DIR:${target}>/${dst_name}"
    VERBATIM
  )
endfunction()

mario_copy_runtime_dir(mario "${PROJECT_SOURCE_DIR}/assets" "assets")
mario_copy_runtime_dir(mario "${PROJECT_SOURCE_DIR}/jumpman_assets" "jumpman_assets")

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

  set(SDL2IMAGE_VENDORED ON CACHE BOOL "" FORCE)
  set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    SDL2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.8.2
  )

  # SDL2_image determines shared/static from BUILD_SHARED_LIBS; ensure it matches our SDL2-static build.
  set(_mario_prev_build_shared_libs ${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS OFF)
  FetchContent_MakeAvailable(SDL2_image)
  set(BUILD_SHARED_LIBS ${_mario_prev_build_shared_libs})

  add_executable(mario_sdl
    src/sdl_main.cpp
  )
  target_link_libraries(mario_sdl PRIVATE mario::core SDL2::SDL2-static)
  if(TARGET SDL2_image::SDL2_image-static)
    target_link_libraries(mario_sdl PRIVATE SDL2_image::SDL2_image-static)
  elseif(TARGET SDL2_image::SDL2_image)
    target_link_libraries(mario_sdl PRIVATE SDL2_image::SDL2_image)
  else()
    message(FATAL_ERROR "SDL2_image target not found; cannot link mario_sdl")
  endif()
  target_compile_features(mario_sdl PRIVATE cxx_std_20)
  if(APPLE)
    target_link_options(mario_sdl PRIVATE "-Wl,-no_warn_duplicate_libraries")
  endif()

  mario_copy_runtime_dir(mario_sdl "${PROJECT_SOURCE_DIR}/assets" "assets")
  mario_copy_runtime_dir(mario_sdl "${PROJECT_SOURCE_DIR}/jumpman_assets" "jumpman_assets")
endif()
