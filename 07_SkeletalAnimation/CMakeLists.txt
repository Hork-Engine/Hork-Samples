set( PROJECT 07_SkeletalAnimation )

file( GLOB Code *.h *.cpp )
file( GLOB Resources res/resource.rc res/hork.ico res/resource.h )

source_group( "" FILES ${Code} )
source_group( "res" FILES ${Resources} )

set( SRCS
${Code}
${Resources}
)

setup_msvc_runtime_library()

if(WIN32)
add_executable( ${PROJECT} WIN32 ${SRCS} )
else()
add_executable( ${PROJECT} ${SRCS} )
endif()

target_link_libraries( ${PROJECT} HorkEngine )

target_compile_definitions(${PROJECT} PUBLIC ${HK_COMPILER_DEFINES})
target_compile_options(${PROJECT} PUBLIC ${HK_COMPILER_FLAGS})
