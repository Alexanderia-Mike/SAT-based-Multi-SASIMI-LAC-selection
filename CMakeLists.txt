# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# Set the project name
project (sasimi-vecbee)

# collect source files
aux_source_directory(${PROJECT_SOURCE_DIR}/src sources)
aux_source_directory(${PROJECT_SOURCE_DIR}/depqbf depSources)

# Add include paths
include_directories(${PROJECT_SOURCE_DIR}/abc/src)
include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/depqbf)

# set boost find path
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "/opt/homebrew/opt/icu4c/sbin")

# Add link directories
link_directories(${PROJECT_SOURCE_DIR}/abc)
link_directories(${PROJECT_SOURCE_DIR}/depqbf)

# Add definitions
add_definitions(-DLIN64 -g -Wall -O3 -std=c++17)

# set output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

# to regard c files as cpp files
#SET_SOURCE_FILES_PROPERTIES(${SOURCE_FILES} PROPERTIES LANGUAGE CXX)

# Add an executable with the above sources
#add_executable(sasimi-vecbee ${sources} depqbf/examples/basic-clause-groups-api-example-assumptions.cpp)
add_executable(sasimi-vecbee src/test.cpp ${sources})
add_executable(depqbfEx depqbf/examples/basic-clause-groups-api-example-assumptions.cpp)

# link libraries, with -lrt, which seems not working on Mac
# target_link_libraries(sasimi-vecbee -labc -lm -ldl -rdynamic -lreadline -ltermcap -lpthread -lstdc++ -lrt -lstdc++fs)
# link libraries, without -lrt, cannot work because -lstdc++fs not working
# target_link_libraries(sasimi-vecbee -labc -lm -ldl -rdynamic -lreadline -ltermcap -lpthread -lstdc++ -lstdc++fs)
# get rid of -lrt and -lstdc++fs !
target_link_libraries(sasimi-vecbee -labc -lqdpll -lm -ldl -rdynamic -lreadline -ltermcap -lpthread -lstdc++)
target_link_libraries(depqbfEx libqdpll.a -lm -ldl -rdynamic -lreadline -ltermcap -lpthread -lstdc++)



# find boost
FIND_PACKAGE(Boost)
IF (Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    ADD_DEFINITIONS( "-DHAS_BOOST" )
ENDIF()

