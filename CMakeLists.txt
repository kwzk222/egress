cmake_minimum_required(VERSION 3.22)
project(EchoValhallaSuperPlugin VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

message(STATUS "Fetching JUCE 8...")
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(EchoValhallaSuperPlugin
    COMPANY_NAME "EchoValhallaAudio"
    BUNDLE_ID "com.echovalhalla.superplugin"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE EVSA
    PLUGIN_CODE Evsp
    FORMATS VST3 Standalone
    PRODUCT_NAME "EchoValhalla SuperPlugin"
)

juce_generate_juce_header(EchoValhallaSuperPlugin)

target_sources(EchoValhallaSuperPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/DelayEngine.cpp
        Source/DSP/ReverbEngine.cpp
        Source/DSP/ParametricEQ.cpp
        Source/DSP/FXBus.cpp
)

target_compile_definitions(EchoValhallaSuperPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(EchoValhallaSuperPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Unit Test executable for verifying DSP logic during build
add_executable(DSPTests Tests/DSPTests.cpp Source/DSP/DelayEngine.cpp Source/DSP/ParametricEQ.cpp Source/DSP/ReverbEngine.cpp Source/DSP/FXBus.cpp)
target_include_directories(DSPTests PRIVATE Source)
target_link_libraries(DSPTests PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)
