cmake_minimum_required(VERSION 3.22)
project(pre_glow VERSION 0.1.3 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG d6181bde38d858c283c3b7bf699ce6340c050b5d)
FetchContent_MakeAvailable(JUCE)

set(formats VST3 Standalone)
if(APPLE)
    list(APPEND formats AU)
endif()
juce_add_plugin(pre_glow
    COMPANY_NAME "pre-glow"
    BUNDLE_ID "audio.preglow.plugin"
    PLUGIN_MANUFACTURER_CODE Pglo
    PLUGIN_CODE PrGl
    FORMATS ${formats}
    PLUGIN_NAME "pre-glow ${PROJECT_VERSION}"

    PRODUCT_NAME "pre-glow")
target_sources(pre_glow PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_features(pre_glow PUBLIC cxx_std_17)
target_compile_definitions(pre_glow PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(pre_glow PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

include(CTest)
if(BUILD_TESTING)
    add_executable(reverb_test Tests/Unit/ReverbTest.cpp)
    target_compile_features(reverb_test PRIVATE cxx_std_17)
    add_test(NAME reverb_impulse COMMAND reverb_test)
    add_executable(pitch_shifter_test Tests/Unit/PitchShifterTest.cpp)
    target_compile_features(pitch_shifter_test PRIVATE cxx_std_17)
    add_test(NAME pitch_shifter COMMAND pitch_shifter_test)
endif()
