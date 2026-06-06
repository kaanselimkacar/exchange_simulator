add_library(domain market.cpp matching_engine.cpp)

target_include_directories(domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(domain PUBLIC project_warnings)
