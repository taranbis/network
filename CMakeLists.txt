# CMakeList.txt : CMake project for 007_TCP_Handler, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("007_TCP_Handler")

# Add source to this project's executable.
add_executable (007_TCP_Handler tcp_main.cpp  tcp_connection.cpp tcp_connection_manager.cpp)
add_executable (007_Simple_TCP_Server tcp_blocking.cpp)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_target_properties(007_TCP_Handler PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    set_target_properties(007_Simple_TCP_Server PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
endif()

# TODO: Add tests and install targets if needed.
find_package(Threads REQUIRED)
find_package(Boost REQUIRED)

target_link_libraries(007_TCP_Handler Threads::Threads)
target_link_libraries(007_TCP_Handler ${Boost_LIBRARIES})
target_link_libraries(007_TCP_Handler ws2_32)

target_include_directories(007_TCP_Handler PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(007_TCP_Handler PUBLIC include)

target_link_libraries(007_Simple_TCP_Server Threads::Threads)
target_link_libraries(007_Simple_TCP_Server ${Boost_LIBRARIES})
target_link_libraries(007_Simple_TCP_Server ws2_32)

target_include_directories(007_Simple_TCP_Server PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(007_Simple_TCP_Server PUBLIC include)
