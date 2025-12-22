set(PROJECT_SOURCES
    basic/Clock.cpp
    basic/GuiHandler.cpp
    core/Application.cpp
    core/Layer.cpp
    core/Window.cpp
    stream/AudioStream.cpp
    stream/Stream.cpp
    stream/VideoStream.cpp
    video/Deque.cpp
    video/MediaFile.cpp
    video/Video.cpp
    main.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        $<TARGET_PROPERTY:DearImGui,INCLUDE_DIRECTORIES>
        ${AVCODEC_INCLUDE_DIR}
        ${AVFORMAT_INCLUDE_DIR}
        ${AVUTIL_INCLUDE_DIR}
        ${SWSCALE_INCLUDE_DIR}
        ${SWRESAMPLE_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        DearImGui
        SDL3::SDL3
        ${AVFORMAT_LIBRARY}
        ${AVCODEC_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${SWRESAMPLE_LIBRARY}
)

if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -pthread)   

    target_link_options(${PROJECT_NAME}
        PRIVATE
            -pthread
            -s INITIAL_MEMORY=536870912
            --preload-file input.mp4)   
endif()


add_custom_target(copy_data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Copying data/ -> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data"
)

add_dependencies(${PROJECT_NAME} copy_data)
