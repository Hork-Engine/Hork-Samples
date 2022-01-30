cmake_minimum_required(VERSION 3.7.2)

project(Hork_Samples)

set(PROJECT_OUTPUT_DIR "D:/Hork_Samples")

include(Hork-Source/Source/hork_config.cmake)
include_directories(Hork-Source/Source)
add_definitions(-DANGIE_STATIC_LIBRARY)
add_subdirectory(Hork-Source/Source)

add_subdirectory(01_Simple)
add_subdirectory(02_CharacterController )
add_subdirectory(03_Triggers)
add_subdirectory(04_Terrain)
add_subdirectory(05_ProceduralMesh)
