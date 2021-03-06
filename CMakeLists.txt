cmake_minimum_required(VERSION 3.0.0)
project(ABC        
    VERSION 2018.6.27
    LANGUAGES C CXX
)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_HOME_DIRECTORY}/bin")
#include_directories(3rdParty/openssl-1.0.2d/include/)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     ON)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -DDEBUG" )

# for AWS
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -DDEBUG -DUSE_AWS")
# for MacOS
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -DDEBUG" )


#set(CMAKE_OSX_ARCHITECTURES "x86_64")


#add_subdirectory(src)
#add_subdirectory(3rdParty/openssl-1.0.2d)
#file(GLOB_RECURSE SOURCE_FILES   "src/*.cpp" "src/*.c" "3rdParty/openssl-1.0.2d/*.c" )
file(GLOB_RECURSE SOURCE_FILES   "src/*.cpp" "src/*.c" )
#file(GLOB_RECURSE SOURCE_FILES_CLI    )


add_executable(ABC ${SOURCE_FILES})

#find_package(Boost 1.57.0 REQUIRED COMPONENTS thread signals filesystem system coroutine context chrono container)
find_package(Boost 1.57.0 REQUIRED COMPONENTS thread signals filesystem system coroutine context )
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR})
    target_link_libraries(ABC boost_coroutine ${Boost_LIBRARIES} pthread )
else()
    message(FATAL_ERROR "boost not found")
endif()

find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "Fail to find OpenSSL")
endif()
#target_link_libraries(ABC OpenSSL::SSL OpenSSL::Crypto )
target_link_libraries(ABC ${OPENSSL_LIBRARIES} )
message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
include_directories(${OPENSSL_INCLUDE_DIR})



#find_package(Boost 1.57.0 COMPONENTS thread signals filesystem system context chrono container)
#if(Boost_FOUND)
#    include_directories(${Boost_INCLUDE_DIRS})
#    add_executable(client ${SOURCE_FILES_CLI})
#    target_link_libraries(client pthread boost_coroutine ${Boost_LIBRARIES}  Common)
#endif()
