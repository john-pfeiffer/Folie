# Pinned JUCE fetch. Do not bump casually — CI caches build/_deps keyed on this tag,
# and plugin binary compatibility should be re-validated (pluginval) on any change.
include(FetchContent)

set(FOLIE_JUCE_TAG "9.0.1" CACHE STRING "JUCE git tag to build against")

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${FOLIE_JUCE_TAG}
    GIT_SHALLOW    TRUE
    SYSTEM)

FetchContent_MakeAvailable(JUCE)
