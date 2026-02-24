cmake_minimum_required(VERSION 3.20)
project(PrimeStudio VERSION 0.1.0 LANGUAGES CXX)

option(PRIMESTUDIO_FETCH_PRIMESTAGE "Fetch PrimeStage dependency" ON)
set(PRIMESTUDIO_PRIMESTAGE_GIT_REPOSITORY "https://github.com/ChristofferGreen/PrimeStage.git"
    CACHE STRING "PrimeStage git repository")
set(PRIMESTUDIO_PRIMESTAGE_GIT_TAG "master" CACHE STRING "PrimeStage git tag or commit")

option(PRIMESTUDIO_ENABLE_PRIMEMANIFEST "Enable PrimeManifest integration" OFF)
set(PRIMESTUDIO_PRIMEMANIFEST_GIT_REPOSITORY "https://github.com/ChristofferGreen/PrimeManifest.git"
    CACHE STRING "PrimeManifest git repository")
set(PRIMESTUDIO_PRIMEMANIFEST_GIT_TAG "master" CACHE STRING "PrimeManifest git tag or commit")

set(CMAKE_DEBUG_POSTFIX "-debug")
set(CMAKE_RELEASE_POSTFIX "-release")
set(CMAKE_RELWITHDEBINFO_POSTFIX "-relwithdebinfo")
set(CMAKE_MINSIZEREL_POSTFIX "-minsizerel")

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

function(pst_require_cxx23 target)
  if(CMAKE_CXX_COMPILE_FEATURES)
    target_compile_features(${target} PUBLIC cxx_std_23)
  else()
    message(WARNING "CMake lacks feature metadata for ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}; falling back to CXX_STANDARD on target ${target}.")
    set_property(TARGET ${target} PROPERTY CXX_STANDARD 23)
    set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET ${target} PROPERTY CXX_EXTENSIONS OFF)
  endif()
endfunction()

option(PRIMESTUDIO_BUILD_TESTS "Build PrimeStudio tests" ON)
option(PRIMESTUDIO_BUILD_EXAMPLES "Build PrimeStudio example apps" OFF)

if(PRIMESTUDIO_ENABLE_PRIMEMANIFEST OR PRIMESTUDIO_BUILD_EXAMPLES)
  set(PRIMESTAGE_ENABLE_PRIMEMANIFEST ON CACHE BOOL "Enable PrimeManifest integration in PrimeStage" FORCE)
  set(PRIMESTAGE_PRIMEMANIFEST_GIT_REPOSITORY ${PRIMESTUDIO_PRIMEMANIFEST_GIT_REPOSITORY}
      CACHE STRING "PrimeManifest git repository" FORCE)
  set(PRIMESTAGE_PRIMEMANIFEST_GIT_TAG ${PRIMESTUDIO_PRIMEMANIFEST_GIT_TAG}
      CACHE STRING "PrimeManifest git tag or commit" FORCE)
endif()

if(PRIMESTUDIO_FETCH_PRIMESTAGE)
  include(FetchContent)
  set(PRIMEFRAME_BUILD_TESTS OFF CACHE BOOL "Build PrimeFrame tests" FORCE)
  set(PRIMEFRAME_BUILD_EXAMPLES OFF CACHE BOOL "Build PrimeFrame examples" FORCE)
  set(PRIMESTAGE_BUILD_TESTS OFF CACHE BOOL "Build PrimeStage tests" FORCE)
  set(PRIMESTAGE_BUILD_EXAMPLES OFF CACHE BOOL "Build PrimeStage examples" FORCE)
  FetchContent_Declare(PrimeStage
    GIT_REPOSITORY ${PRIMESTUDIO_PRIMESTAGE_GIT_REPOSITORY}
    GIT_TAG ${PRIMESTUDIO_PRIMESTAGE_GIT_TAG}
  )
  FetchContent_MakeAvailable(PrimeStage)
endif()

add_library(PrimeStudio src/PrimeStudio.cpp)

target_include_directories(PrimeStudio
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

pst_require_cxx23(PrimeStudio)

if(TARGET PrimeStage)
  target_link_libraries(PrimeStudio PUBLIC PrimeStage)
endif()

if(PRIMESTUDIO_BUILD_TESTS)
  enable_testing()

  add_executable(PrimeStudio_tests
    tests/unit/test_main.cpp
    tests/unit/test_builder.cpp
    tests/unit/test_interaction.cpp
    tests/unit/test_tree_view.cpp
    tests/unit/test_widgets.cpp
    tests/unit/test_tabs_dropdown.cpp
    tests/unit/test_text_selection.cpp
  )
  target_link_libraries(PrimeStudio_tests PRIVATE PrimeStudio)
  target_include_directories(PrimeStudio_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  pst_require_cxx23(PrimeStudio_tests)
  add_test(NAME PrimeStudio_tests COMMAND $<TARGET_FILE:PrimeStudio_tests>)
endif()

if(PRIMESTUDIO_BUILD_EXAMPLES)
  include(FetchContent)

  option(PRIMESTUDIO_PRIMEMANIFEST_ENABLE_FONTS "Enable PrimeManifest font rendering for examples" ON)

  set(PRIMEMANIFEST_BUILD_TESTS OFF CACHE BOOL "Build PrimeManifest tests" FORCE)
  set(PRIMEMANIFEST_ENABLE_FONTS ${PRIMESTUDIO_PRIMEMANIFEST_ENABLE_FONTS} CACHE BOOL "Enable PrimeManifest fonts" FORCE)
  set(PRIMEMANIFEST_BUILD_EXAMPLES OFF CACHE BOOL "Build PrimeManifest examples" FORCE)

  if(NOT TARGET PrimeManifest)
    FetchContent_Declare(
      PrimeManifest
      GIT_REPOSITORY ${PRIMESTUDIO_PRIMEMANIFEST_GIT_REPOSITORY}
      GIT_TAG ${PRIMESTUDIO_PRIMEMANIFEST_GIT_TAG}
    )
    FetchContent_MakeAvailable(PrimeManifest)
  endif()

  add_executable(primestudio_render_demo examples/primestage_render_demo.cpp)
  target_link_libraries(primestudio_render_demo PRIVATE PrimeStudio PrimeStage PrimeManifest)
  target_include_directories(primestudio_render_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  if(DEFINED primemanifest_SOURCE_DIR)
    file(GLOB_RECURSE PRIMESTUDIO_PM_FONTS
      "${primemanifest_SOURCE_DIR}/*.ttf"
      "${primemanifest_SOURCE_DIR}/*.otf"
      "${primemanifest_SOURCE_DIR}/*.ttc"
      "${primemanifest_SOURCE_DIR}/*.otc"
    )
    if(PRIMESTUDIO_PM_FONTS)
      list(GET PRIMESTUDIO_PM_FONTS 0 PRIMESTUDIO_PM_FONT_SAMPLE)
      get_filename_component(PRIMESTUDIO_PM_FONT_DIR "${PRIMESTUDIO_PM_FONT_SAMPLE}" DIRECTORY)
      target_compile_definitions(primestudio_render_demo PRIVATE
        PRIMESTAGE_BUNDLED_FONT_DIR=\"${PRIMESTUDIO_PM_FONT_DIR}\"
        PRIMESTAGE_HAS_BUNDLED_FONT=1
      )
    else()
      target_compile_definitions(primestudio_render_demo PRIVATE PRIMESTAGE_HAS_BUNDLED_FONT=0)
    endif()
  else()
    target_compile_definitions(primestudio_render_demo PRIVATE PRIMESTAGE_HAS_BUNDLED_FONT=0)
  endif()
  pst_require_cxx23(primestudio_render_demo)

  add_executable(primestudio_widgets_demo examples/primestage_widgets_demo.cpp)
  target_link_libraries(primestudio_widgets_demo PRIVATE PrimeStudio PrimeStage PrimeManifest)
  target_include_directories(primestudio_widgets_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  if(DEFINED primemanifest_SOURCE_DIR)
    if(PRIMESTUDIO_PM_FONTS)
      target_compile_definitions(primestudio_widgets_demo PRIVATE
        PRIMESTAGE_BUNDLED_FONT_DIR=\"${PRIMESTUDIO_PM_FONT_DIR}\"
        PRIMESTAGE_HAS_BUNDLED_FONT=1
      )
    else()
      target_compile_definitions(primestudio_widgets_demo PRIVATE PRIMESTAGE_HAS_BUNDLED_FONT=0)
    endif()
  else()
    target_compile_definitions(primestudio_widgets_demo PRIVATE PRIMESTAGE_HAS_BUNDLED_FONT=0)
  endif()
  pst_require_cxx23(primestudio_widgets_demo)
endif()
