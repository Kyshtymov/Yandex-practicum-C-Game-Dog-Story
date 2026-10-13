cmake_minimum_required(VERSION 3.11)

project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)

# Принудительно указываем статические библиотеки
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${importTargets}")

find_package(Boost 1.78.0 REQUIRED COMPONENTS program_options log thread)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Подключаем Conan targets
add_library(MyLib STATIC 
  src/model.cpp 
  src/boost_json.cpp 
  src/json_loader.cpp 
  src/players.cpp 
  src/body_maker.cpp 
  src/respond_maker.cpp 
  src/application_handler.cpp
  src/loot_generator.cpp
  src/collision_detector.cpp
  src/extra_objects.cpp
  src/tagged_uuid.cpp
  src/application_listener.cpp
  src/serialization_listener.cpp
  src/model_serialization.cpp
  src/players_serialization.cpp
  src/connection_pool.cpp
)

target_include_directories(MyLib PUBLIC 
  ${ZLIB_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(MyLib PUBLIC 
  CONAN_PKG::boost
  CONAN_PKG::zlib
  Threads::Threads
  CONAN_PKG::libpq
  CONAN_PKG::libpqxx
)

add_executable(game_server 
  src/main.cpp 
  src/http_server.cpp
)

#add_executable(game_server_tests 
#  tests/loot_generator_tests.cpp
#)

#add_executable(collision_detector_tests
#  tests/collision-detector-tests.cpp
#)

target_link_libraries(game_server MyLib)


#target_link_libraries(game_server_tests 
#  CONAN_PKG::catch2
#  MyLib
#)

#target_link_libraries(collision_detector_tests 
#  CONAN_PKG::catch2
#  MyLib
#)
