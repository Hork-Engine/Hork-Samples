cmake_minimum_required(VERSION 3.18)

project(Hork-Samples)

include(Hork-Source/hork_config.cmake)

add_subdirectory(Hork-Source)

include_directories(Hork-Source)
include_directories(${CMAKE_BINARY_DIR}/include/ThirdParty)

set(HK_ASSET_DATA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Data")
set(HK_ASSET_DATA_INSTALL_PATH "${HK_PROJECT_BUILD_PATH}/Data")

make_link(${HK_ASSET_DATA_PATH} ${HK_ASSET_DATA_INSTALL_PATH})

add_subdirectory(01_Simple)
add_subdirectory(02_CharacterController)
add_subdirectory(03_Triggers)
add_subdirectory(04_Terrain)
add_subdirectory(05_ProceduralMesh)
add_subdirectory(06_TwoPlayers)
add_subdirectory(07_SkeletalAnimation)
add_subdirectory(08_RenderToTexture)
