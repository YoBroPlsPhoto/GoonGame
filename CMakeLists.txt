cmake_minimum_required(VERSION 3.10)
project(AdasGooner)

set(CMAKE_CXX_STANDARD 17)

set(FETCHCONTENT_QUIET OFF)


include(FetchContent)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)
include_directories(${asio_SOURCE_DIR}/asio/include)

# Threads (Required for asio)
find_package(Threads REQUIRED)

# Sources and binaries
set(SOURCES 
    src/main.cpp 
    src/player/player.cpp
    src/enemies/enemy.cpp
    src/hud/menu.cpp
    src/network/network_manager.cpp
    src/map/desert_map.cpp
    src/map/city_map.cpp
    src/weapons/glock.cpp
    src/weapons/ak47.cpp
    src/weapons/minigun.cpp
    src/weapons/awp.cpp
    src/weapons/shotgun.cpp
    src/weapons/revolver.cpp
    src/weapons/rpg.cpp
    src/weapons/knife.cpp
    src/weapons/builder_tool.cpp
    src/effects/particle_system.cpp
    src/vehicles/vehicle.cpp
    src/vehicles/tank.cpp
    src/bosses/adas_gooner.cpp
    src/bosses/adas_prime.cpp
    src/bosses/gang_boss.cpp
    src/bosses/gibon_rzygacz.cpp
    src/structures/structure.cpp
)

# Detect platform paths
if(WIN32)
    set(RAYLIB_PATH ${CMAKE_SOURCE_DIR}/include/win)
else()
    set(RAYLIB_PATH ${CMAKE_SOURCE_DIR}/include/linux64)
endif()
set(RAYLIB_LIB ${RAYLIB_PATH}/libraylib.a)

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()
add_executable(AdasServer src/server_main.cpp src/network/network_manager.cpp)

# Common settings
target_include_directories(${PROJECT_NAME} PRIVATE ${RAYLIB_PATH} src src/player src/enemies src/map)
target_include_directories(AdasServer PRIVATE ${RAYLIB_PATH} src src/map)

target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIB} Threads::Threads)
target_link_libraries(AdasServer PRIVATE ${RAYLIB_LIB} Threads::Threads)

# Platform-specific setup (order matters!)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 mswsock gdi32 winmm)
    target_link_libraries(AdasServer PRIVATE ws2_32 mswsock gdi32 winmm)
    # Static linking for standalone Windows executables
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -static -mwindows)
    target_link_options(AdasServer PRIVATE -static-libgcc -static-libstdc++ -static)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 GL m dl rt)
    target_link_libraries(AdasServer PRIVATE X11 GL m dl rt)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE ASIO_STANDALONE)
target_compile_definitions(AdasServer PRIVATE ASIO_STANDALONE)
