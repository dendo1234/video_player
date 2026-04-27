
add_library(font STATIC source/impl.c)

target_include_directories(font
    INTERFACE
        include)
