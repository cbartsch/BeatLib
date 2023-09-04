cmake_minimum_required(VERSION 3.16)

project(BeatLib VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Qml Multimedia Widgets Quick)

file(GLOB Sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp src/*.h)

if(ANDROID)
  list(APPEND Sources ${CMAKE_CURRENT_SOURCE_DIR}/src/android/audiofileselector_platform.cpp)
elseif(IOS)
  list(APPEND Sources ${CMAKE_CURRENT_SOURCE_DIR}/src/ios/audiofileselector_platform.mm)
else()
  list(APPEND Sources ${CMAKE_CURRENT_SOURCE_DIR}/src/generic/audiofileselector_platform.cpp)
endif()

qt_add_qml_module(BeatLib
    PLUGIN_TARGET BeatLib
    URI at.cb.BeatLib
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/at/cb/BeatLib
    VERSION 1.0
    SOURCES ${Sources}
    NO_GENERATE_PLUGIN_SOURCE CLASS_NAME BeatLibPlugin
)
set_target_properties(BeatLib PROPERTIES OUTPUT_NAME $<IF:$<CONFIG:DEBUG>,BeatLibd,BeatLib>)

add_library(FFTW3 STATIC IMPORTED)
add_library(MPG123 STATIC IMPORTED)

set(LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

if(MSVC)
  message("Link FFTW3 and MPG123 MSVC 64-bit build.")

  # MSVC requires debug libraries in debug mode
  set_target_properties(FFTW3 PROPERTIES IMPORTED_LOCATION_RELEASE ${LIBDIR}/libfftw3-msvc.lib)
  set_target_properties(MPG123 PROPERTIES IMPORTED_LOCATION_RELEASE ${LIBDIR}/libmpg123-msvc.lib)

  set_target_properties(FFTW3 PROPERTIES IMPORTED_LOCATION_DEBUG ${LIBDIR}/libfftw3-msvcd.lib)
  set_target_properties(MPG123 PROPERTIES IMPORTED_LOCATION_DEBUG ${LIBDIR}/libmpg123-msvcd.lib)
elseif(MINGW)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message("Link FFTW3 and MPG123 MinGW 64-bit build.")
    set_target_properties(FFTW3 PROPERTIES IMPORTED_LOCATION ${LIBDIR}/libfftw3-mingw64.a)
    set_target_properties(MPG123 PROPERTIES IMPORTED_LOCATION ${LIBDIR}/libmpg123-mingw64.a)
  else()
    message("Link FFTW3 and MPG123 MinGW 32-bit build.")
    set_target_properties(FFTW3 PROPERTIES IMPORTED_LOCATION ${LIBDIR}/libfftw3-mingw.a)
    set_target_properties(MPG123 PROPERTIES IMPORTED_LOCATION ${LIBDIR}/libmpg123-mingw.a)
  endif()
elseif(ANDROID)
  message("Link FFTW3 and MPG123 Android-${ANDROID_ABI} build.")
  set_target_properties(FFTW3 PROPERTIES IMPORTED_LOCATION ${LIBDIR}/libfftw3-android-${ANDROID_ABI}.a)
  set_target_properties(MPG123 PROPERTIES IMPORTED_LOCATION ${LIBDIR}/libmpg123-android-${ANDROID_ABI}.a)
else()
  # TODO macOS and iOS support
endif()

target_include_directories(BeatLib PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(BeatLib PUBLIC FFTW3 MPG123 Qt6::Qml Qt6::Multimedia Qt6::Widgets Qt6::Quick)

if(MSVC)
  target_compile_options(BeatLib PUBLIC $<$<CONFIG:Release>:-Ox>)
else()
  target_compile_options(BeatLib PUBLIC $<$<CONFIG:Release>:-O3>)
endif()

message("Build plugin with ${CMAKE_BUILD_TYPE}")

target_compile_definitions(BeatLib
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

