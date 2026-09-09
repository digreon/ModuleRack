cmake_minimum_required(VERSION 3.22)

project(ModuleRack VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MODULERACK_BUILD_TESTS "Build the headless DSP test runner" ON)
option(MODULERACK_BUILD_AUV3 "Build the AUv3 target on Apple platforms (needs a Mac + Xcode toolchain)" ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Core: clock, modules and the fixed signal graph. No GUI and no plugin client,
# so the headless test runner can link the exact code the plugin runs.
# ---------------------------------------------------------------------------
add_library(ModuleRackCore STATIC
    Source/Core/Clock.h
    Source/Core/Euclidean.h
    Source/Core/MidiMapper.cpp
    Source/Core/MidiMapper.h
    Source/Core/ModuleSlot.cpp
    Source/Core/ModuleSlot.h
    Source/Core/PresetManager.cpp
    Source/Core/PresetManager.h
    Source/Core/Scale.h
    Source/Core/SignalGraph.cpp
    Source/Core/SignalGraph.h
    Source/Core/SimpleFilter.h
    Source/Core/SwingDelay.h
    Source/Modules/ArpSequencerModule.cpp
    Source/Modules/ArpSequencerModule.h
    Source/Modules/BassModule.cpp
    Source/Modules/BassModule.h
    Source/Modules/FxFilterDelayModule.cpp
    Source/Modules/FxFilterDelayModule.h
    Source/Modules/FxReverbCompModule.cpp
    Source/Modules/FxReverbCompModule.h
    Source/Modules/KickModule.cpp
    Source/Modules/KickModule.h
    Source/Modules/LeadModule.cpp
    Source/Modules/LeadModule.h
    Source/Modules/PadChordModule.cpp
    Source/Modules/PadChordModule.h
    Source/Modules/PercussionModule.cpp
    Source/Modules/PercussionModule.h
)

target_include_directories(ModuleRackCore PUBLIC Source)

# The core is linked into the plugin's shared-library formats (VST3/AU), so it
# has to be position-independent as well.
set_target_properties(ModuleRackCore PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_compile_definitions(ModuleRackCore PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ModuleRackCore
    PUBLIC
        juce::juce_audio_basics
        juce::juce_dsp
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# The plugin itself. AU/AUv3 are the targets that matter (macOS + iPad AUv3
# hosts such as Loopy Pro); elsewhere VST3 + Standalone are built so the DSP and
# UI can be compiled and run on any dev machine.
# ---------------------------------------------------------------------------
if (APPLE)
    set(MODULERACK_FORMATS AU Standalone)

    # AUv3 is the target that matters for the iPad, but it builds an app-extension
    # bundle, which is the fussiest thing here to produce on a machine that isn't a
    # configured Mac dev box. Off in CI, on by default everywhere else.
    if (MODULERACK_BUILD_AUV3)
        list(APPEND MODULERACK_FORMATS AUv3)
    endif()
else()
    set(MODULERACK_FORMATS VST3 Standalone)
endif()

juce_add_plugin(ModuleRack
    PRODUCT_NAME "ModuleRack"
    COMPANY_NAME "Horizon"
    BUNDLE_ID "com.horizon.modulerack"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Hrzn
    PLUGIN_CODE Mrk1
    FORMATS ${MODULERACK_FORMATS}
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
)

target_sources(ModuleRack PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/UI/ModulePanel.cpp
    Source/UI/ModulePanel.h
    Source/UI/MixOverviewComponent.cpp
    Source/UI/MixOverviewComponent.h
)

target_compile_definitions(ModuleRack PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ModuleRack
    PRIVATE
        ModuleRackCore
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Headless test runner: renders the real signal graph offline and checks the
# clock, the generative modules, the mapping and preset round-tripping. Needs no
# audio device, no window and no plugin host, so it runs in CI.
# ---------------------------------------------------------------------------
if (MODULERACK_BUILD_TESTS)
    juce_add_console_app(ModuleRackTests PRODUCT_NAME "ModuleRackTests")

    target_sources(ModuleRackTests PRIVATE Tests/ModuleRackTests.cpp)

    target_link_libraries(ModuleRackTests
        PRIVATE
            ModuleRackCore
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    enable_testing()
    add_test(NAME ModuleRackTests COMMAND ModuleRackTests)

    # Offline renderer: hear a patch as a WAV without a plugin host.
    juce_add_console_app(ModuleRackRender PRODUCT_NAME "ModuleRackRender")

    target_sources(ModuleRackRender PRIVATE Tools/RenderPreview.cpp)

    target_link_libraries(ModuleRackRender
        PRIVATE
            ModuleRackCore
            juce::juce_audio_formats
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )
endif()
