cmake_minimum_required(VERSION 3.14)

project(proxy_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_PREFIX_PATH "/Users/yousof/Qt/5.15.2/clang_64/lib/cmake")


find_package(Qt5 COMPONENTS
        Core
        Network
        REQUIRED)

    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core Network)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)
include_directories(/usr/local/Cellar/boost/1.85.0/include/)
link_directories(/usr/local/Cellar/boost/1.85.0/lib/)


add_executable(proxy_server main.cpp proxy_server.cpp proxy_server.h
    defines.h)


target_link_libraries(proxy_server Qt${QT_VERSION_MAJOR}::Core
Qt${QT_VERSION_MAJOR}::Network
 libboost_iostreams.a)



