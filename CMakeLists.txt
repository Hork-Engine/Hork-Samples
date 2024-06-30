cmake_minimum_required(VERSION 3.18)

project(Hork-Samples)

include(Hork-Source/hork_config.cmake)

add_subdirectory(Hork-Source)

include_directories(Hork-Source)
include_directories(${CMAKE_BINARY_DIR}/include/ThirdParty)

set(HK_ASSET_DATA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Data")
set(HK_ASSET_DATA_INSTALL_PATH "${HK_PROJECT_BUILD_PATH}/Data")

make_link(${HK_ASSET_DATA_PATH} ${HK_ASSET_DATA_INSTALL_PATH})

add_subdirectory(00_Empty)
add_subdirectory(01_HelloWorld)
add_subdirectory(02_FirstPersonShooter)
add_subdirectory(03_ThirdPerson)
add_subdirectory(04_RenderToTexture)
add_subdirectory(05_NavMesh)
