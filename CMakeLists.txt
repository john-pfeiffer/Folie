cmake_minimum_required(VERSION 3.25)

project(Folie VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FOLIE_COPY_PLUGIN "Copy plugin into system plugin folders after build (local dev only)" OFF)
option(FOLIE_BUILD_TESTS "Build offline DSP render tests" ON)

include(cmake/GetJUCE.cmake)

set(FOLIE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND FOLIE_FORMATS AU)
endif()

juce_add_plugin(Folie
    COMPANY_NAME "EVS"
    BUNDLE_ID com.evs.folie
    # 4-char codes are immutable once builds are in real DAW projects.
    PLUGIN_MANUFACTURER_CODE Evsy
    PLUGIN_CODE Foli
    FORMATS ${FOLIE_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD ${FOLIE_COPY_PLUGIN}
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    PRODUCT_NAME "Folie")

target_sources(Folie PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/params/ParameterLayout.cpp)

target_include_directories(Folie PRIVATE source)

target_compile_definitions(Folie PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Folie
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(FOLIE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
