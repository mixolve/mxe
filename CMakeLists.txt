cmake_minimum_required(VERSION 3.22)

set(mx6_target_system_name "${CMAKE_SYSTEM_NAME}")

if(mx6_target_system_name STREQUAL "")
    set(mx6_target_system_name "${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(mx6_project_languages C CXX)

if(mx6_target_system_name STREQUAL "Darwin")
    list(APPEND mx6_project_languages OBJC OBJCXX)
endif()

project(mx6 VERSION 0.1.0 LANGUAGES ${mx6_project_languages})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MX6_SDK_SHIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/apple-sdk-shims")
option(MX6_USE_LOCAL_JUCE "Use a local JUCE checkout via JUCE_DIR" OFF)

if(APPLE)
    set(MX6_DEFAULT_DEPLOY_DIR "/Library/Audio/Plug-Ins/ALL")
    set(MX6_DEFAULT_RELEASE_ARCHIVE_NAME "mx6-macos.zip")
    set(MX6_ENABLE_VST3_DEPLOY_DEFAULT ON)
elseif(WIN32)
    if(DEFINED ENV{CommonProgramW6432} AND NOT "$ENV{CommonProgramW6432}" STREQUAL "")
        set(MX6_DEFAULT_DEPLOY_DIR "$ENV{CommonProgramW6432}/VST3")
    elseif(DEFINED ENV{COMMONPROGRAMFILES} AND NOT "$ENV{COMMONPROGRAMFILES}" STREQUAL "")
        set(MX6_DEFAULT_DEPLOY_DIR "$ENV{COMMONPROGRAMFILES}/VST3")
    else()
        set(MX6_DEFAULT_DEPLOY_DIR "C:/Program Files/Common Files/VST3")
    endif()

    set(MX6_DEFAULT_RELEASE_ARCHIVE_NAME "mx6-windows.zip")
    set(MX6_ENABLE_VST3_DEPLOY_DEFAULT OFF)
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" mx6_platform_suffix)

    if(mx6_platform_suffix STREQUAL "")
        set(mx6_platform_suffix "unknown")
    endif()

    set(MX6_DEFAULT_DEPLOY_DIR "")
    set(MX6_DEFAULT_RELEASE_ARCHIVE_NAME "mx6-${mx6_platform_suffix}.zip")
    set(MX6_ENABLE_VST3_DEPLOY_DEFAULT OFF)
endif()

option(MX6_ENABLE_VST3_DEPLOY "Deploy the built VST3 bundle after build" ${MX6_ENABLE_VST3_DEPLOY_DEFAULT})
set(MX6_DEPLOY_DIR "${MX6_DEFAULT_DEPLOY_DIR}" CACHE PATH "Directory where the built VST3 bundle will be deployed after build")
set(MX6_ARCHIVE_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Directory where the packaged VST3 zip will be written after build")
set(MX6_RELEASE_ARCHIVE_NAME "${MX6_DEFAULT_RELEASE_ARCHIVE_NAME}" CACHE STRING "Filename for the packaged VST3 release archive")
set(MX6_PLUGIN_BUNDLE_NAME "mx6.vst3")
set(MX6_PACKAGE_STAGE_DIR "${MX6_ARCHIVE_OUTPUT_DIR}/${MX6_PLUGIN_BUNDLE_NAME}")
set(MX6_PACKAGE_ZIP_PATH "${MX6_ARCHIVE_OUTPUT_DIR}/${MX6_RELEASE_ARCHIVE_NAME}")

if(APPLE)
    if(DEFINED ENV{CPATH} AND NOT "$ENV{CPATH}" STREQUAL "")
        set(ENV{CPATH} "${MX6_SDK_SHIM_DIR}:$ENV{CPATH}")
    else()
        set(ENV{CPATH} "${MX6_SDK_SHIM_DIR}")
    endif()
endif()

if(MX6_USE_LOCAL_JUCE)
    if(NOT DEFINED JUCE_DIR)
        message(FATAL_ERROR "Set -DJUCE_DIR=/absolute/path/to/JUCE when MX6_USE_LOCAL_JUCE=ON")
    endif()

    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)

    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12
    )

    FetchContent_MakeAvailable(juce)
endif()

juce_add_binary_data(mx6_assets
    SOURCES
        assets/fonts/SometypeMono-Bold.ttf
        assets/fonts/SometypeMono-Regular.ttf
        assets/fonts/SometypeMono-OFL.txt
)

juce_add_plugin(mx6
    COMPANY_NAME "mixolve"
    BUNDLE_ID "com.mixolve.mx6"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE MxLv
    PLUGIN_CODE Mx60
    FORMATS VST3
    PRODUCT_NAME "mx6"
)

juce_generate_juce_header(mx6)

target_sources(mx6
    PRIVATE
        source/PluginEditor.cpp
        source/PluginEditor.h
        source/PluginProcessor.cpp
        source/PluginProcessor.h
        source/dsp/DspCore.cpp
        source/dsp/DspCore.h
        source/dsp/MultibandCrossover.cpp
        source/dsp/MultibandCrossover.h
        source/dsp/MultibandProcessor.cpp
        source/dsp/MultibandProcessor.h
)

target_compile_definitions(mx6
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(mx6
    PRIVATE
        "${MX6_SDK_SHIM_DIR}"
)

target_link_libraries(mx6
    PRIVATE
        mx6_assets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MX6_ENABLE_VST3_DEPLOY AND NOT MX6_DEPLOY_DIR STREQUAL "")
    add_custom_command(TARGET mx6_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MX6_DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${MX6_DEPLOY_DIR}/${MX6_PLUGIN_BUNDLE_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:mx6_VST3>" "${MX6_DEPLOY_DIR}/${MX6_PLUGIN_BUNDLE_NAME}"
        VERBATIM
    )
endif()

add_custom_command(TARGET mx6_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MX6_ARCHIVE_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${MX6_PACKAGE_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:mx6_VST3>" "${MX6_PACKAGE_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${MX6_PACKAGE_ZIP_PATH}"
    COMMAND ${CMAKE_COMMAND} -E chdir "${MX6_ARCHIVE_OUTPUT_DIR}" ${CMAKE_COMMAND} -E tar "cf" "${MX6_PACKAGE_ZIP_PATH}" --format=zip -- "${MX6_PLUGIN_BUNDLE_NAME}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${MX6_PACKAGE_STAGE_DIR}"
    VERBATIM
)
