cmake_minimum_required(VERSION 3.22)
project(LaarLibrary VERSION 0.1.0)

# Clona JUCE 8 junto a este archivo:  git clone https://github.com/juce-framework/JUCE.git
add_subdirectory(JUCE)

juce_add_plugin(LaarLibrary
    COMPANY_NAME "Laar"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    PLUGIN_MANUFACTURER_CODE Laar
    PLUGIN_CODE Llib
    FORMATS VST3 Standalone
    PRODUCT_NAME "laar library")

target_sources(LaarLibrary PRIVATE Source/Plugin.cpp)

target_compile_definitions(LaarLibrary PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LaarLibrary
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
