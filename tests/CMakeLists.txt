# Offline DSP render tests — console executable, no plugin wrapper needed.
# Real render assertions arrive with the supersaw engine (FOL-4); this target
# exists from day one so the ctest wiring in CI is proven with the scaffold.
juce_add_console_app(FolieTests PRODUCT_NAME "FolieTests")

target_sources(FolieTests PRIVATE RenderTests.cpp)
target_include_directories(FolieTests PRIVATE ${CMAKE_SOURCE_DIR}/source)

target_compile_definitions(FolieTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(FolieTests PRIVATE
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME FolieRenderTests COMMAND FolieTests)

# Dev tool (not a test, not in CI): renders the editor to PNGs headlessly.
# Run under xvfb-run on Linux. Compiles the plugin sources directly (the
# plugin target's JUCE modules are linked PRIVATE and don't propagate).
juce_add_console_app(FolieSnapshot PRODUCT_NAME "FolieSnapshot")
target_sources(FolieSnapshot PRIVATE
    SnapshotTool.cpp
    ${CMAKE_SOURCE_DIR}/source/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/source/PluginEditor.cpp
    ${CMAKE_SOURCE_DIR}/source/params/ParameterLayout.cpp)
target_include_directories(FolieSnapshot PRIVATE ${CMAKE_SOURCE_DIR}/source)
target_compile_definitions(FolieSnapshot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JucePlugin_Name="Folie")
target_link_libraries(FolieSnapshot PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags)
