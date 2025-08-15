set(PROJECT_SOURCES
    basic/Clock.cpp
    basic/Player.cpp
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
        ${AVCODEC_INCLUDE_DIR}
        ${AVFORMAT_INCLUDE_DIR}
        ${AVUTIL_INCLUDE_DIR}
        ${SWSCALE_INCLUDE_DIR}
        ${SWRESAMPLE_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        ${AVFORMAT_LIBRARY}
        ${AVCODEC_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${SWRESAMPLE_LIBRARY}
)


add_custom_target(copy_data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Copying data/ -> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data"
)

add_dependencies(${PROJECT_NAME} copy_data)
