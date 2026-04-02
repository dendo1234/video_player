

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

add_library(Clay STATIC clay_impl.c ${CMAKE_CURRENT_SOURCE_DIR}/renderers/SDL3/clay_renderer_SDL3.c)

target_include_directories(Clay 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/renderers/SDL3)

target_link_libraries(Clay
    PRIVATE
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
)
