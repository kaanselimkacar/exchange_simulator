add_library(src top_level.cpp)

add_subdirectory(domain)
#add_subdirectory(protocols)

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories(protocols PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

