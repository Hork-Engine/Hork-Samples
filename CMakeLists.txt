cmake_minimum_required(VERSION 3.7.2)

project(Hork_Samples)

set( PROJECT_OUTPUT_DIR "D:/Hork_Samples" )

include( Hork-Source/Source/hork_config.cmake )

# Engine
include_directories( Hork-Source/Source )
add_definitions( -DANGIE_STATIC_LIBRARY )
add_subdirectory( Hork-Source/Source )

add_subdirectory( 01_Simple )
set_target_properties( 01_Simple PROPERTIES FOLDER "Samples" )

add_subdirectory( 02_CharacterController )
set_target_properties( 02_CharacterController PROPERTIES FOLDER "Samples" )

add_subdirectory( 03_Triggers)
set_target_properties( 03_Triggers PROPERTIES FOLDER "Samples" )

add_subdirectory( 04_Terrain )
set_target_properties( 04_Terrain PROPERTIES FOLDER "Samples" )

add_subdirectory( 05_ProceduralMesh )
set_target_properties( 05_ProceduralMesh PROPERTIES FOLDER "Samples" )
