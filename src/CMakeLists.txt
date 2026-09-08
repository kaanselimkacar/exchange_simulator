add_library(src domain_gateway.cpp)

add_subdirectory(domain)
add_subdirectory(common)
#add_subdirectory(protocols)

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories(protocols PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(src PUBLIC domain)
target_link_libraries(src PUBLIC common)

