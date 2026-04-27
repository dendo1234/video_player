set(PROJECT_SOURCES
    basic/Clock.cpp
    basic/GuiHandler.cpp
    core/Application.cpp
    core/Layer.cpp
    core/FontManager.cpp
    core/Window.cpp
    stream/AudioStream.cpp
    stream/Stream.cpp
    stream/VideoStream.cpp
    ui/VideoUI.cpp
    video/Deque.cpp
    video/MediaFile.cpp
    video/Video.cpp
    main.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})

if(NOT PRESET_NAME)
    set(PRESET_NAME "")
else()
    set(PRESET_NAME "-${PRESET_NAME}")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
    OUTPUT_NAME "${PROJECT_NAME}${PRESET_NAME}"
)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PUBLIC /W4)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/ui/VideoUI.cpp
      PROPERTIES COMPILE_OPTIONS "/wd26495"
  )
else()
  target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Wpedantic -Werror)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/ui/VideoUI.cpp
      PROPERTIES COMPILE_OPTIONS "-Wno-missing-field-initializers"
  )
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
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
        SDL3_ttf::SDL3_ttf
        Clay
        font
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
