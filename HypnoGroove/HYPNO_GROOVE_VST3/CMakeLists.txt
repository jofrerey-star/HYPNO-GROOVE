cmake_minimum_required(VERSION 3.22)
project(HypnoGroove VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If JUCE is installed locally, set JUCE_DIR; otherwise download it at configure time.
find_package(JUCE CONFIG QUIET)
if(NOT JUCE_FOUND)
  include(FetchContent)
  FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(HypnoGroove
  COMPANY_NAME "Hypno Audio"
  BUNDLE_ID "com.hypnoaudio.hypnogroove"
  PLUGIN_MANUFACTURER_CODE Hgrv
  PLUGIN_CODE Hgr1
  FORMATS VST3
  PRODUCT_NAME "HYPNO GROOVE"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(HypnoGroove PRIVATE
  src/GrooveCore.h
  src/PluginProcessor.h
  src/PluginProcessor.cpp
  src/PluginEditor.h
  src/PluginEditor.cpp)

target_compile_definitions(HypnoGroove PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(HypnoGroove PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_recommended_config_flags)

# Ensure the GUI's UTF-8 labels compile correctly on MSVC.
if(MSVC)
  target_compile_options(HypnoGroove PRIVATE /utf-8)
endif()
