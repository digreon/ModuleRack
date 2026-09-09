cmake_minimum_required(VERSION 3.22)

project(ModuleRack VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (IOS)
    set(MODULERACK_TESTS_DEFAULT OFF) # a console-app target makes no sense on a device
else()
    set(MODULERACK_TESTS_DEFAULT ON)
endif()

option(MODULERACK_BUILD_TESTS "Build the headless DSP test runner" ${MODULERACK_TESTS_DEFAULT})
option(MODULERACK_BUILD_AUV3 "Build the AUv3 target on Apple platforms (needs a Mac + Xcode toolchain)" ON)

# An AUv3 extension runs in its own sandbox container, separate from the standalone
# app that carries it, so the two cannot see each other's preset files. Putting both
# in one App Group fixes that -- but the group has to exist in your Apple developer
# account and in the provisioning profile, so it is opt-in:
#
#   cmake -B build -DMODULERACK_APP_GROUP_ID=group.com.yourteam.modulerack ...
#
# Left empty, each side keeps its own presets (the host still saves and restores
# plugin state either way).
set(MODULERACK_APP_GROUP_ID "" CACHE STRING "App Group id shared by the standalone app and the AUv3 extension")

if (IOS)
    # AUv3 needs iOS 9; 14.0 is a sane floor for an iPad instrument in 2026.
    if (NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "" FORCE)
    endif()
endif()

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

if (MODULERACK_APP_GROUP_ID)
    # PresetManager reads this to find the container both the app and the AUv3
    # extension can write to.
    target_compile_definitions(ModuleRackCore PUBLIC
        MODULERACK_APP_GROUP_ID="${MODULERACK_APP_GROUP_ID}")
endif()

target_link_libraries(ModuleRackCore
    PUBLIC
        juce::juce_audio_basics
        juce::juce_dsp
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# The plugin itself. The iPad is the deployment target: there it builds an AUv3
# extension plus the standalone app that carries it onto the device. On macOS it
# builds AU/AUv3 for development, and elsewhere VST3 + Standalone so the DSP and
# UI can be compiled and run on any dev machine.
# ---------------------------------------------------------------------------
if (IOS)
    # On iOS an AUv3 is installed by installing the app that contains it, so the
    # standalone target is not optional -- it is the delivery vehicle.
    set(MODULERACK_FORMATS Standalone AUv3)

    set(MODULERACK_IOS_OPTIONS
        # An AUv3 host (AUM, Loopy Pro, Cubasis) keeps making sound while it is in
        # the background or the screen is locked, and so must the app carrying it.
        BACKGROUND_AUDIO_ENABLED TRUE

        # The MPK Mini connects over USB (camera adapter) and needs no permission,
        # but a Bluetooth controller does, and BLE MIDI has to survive backgrounding.
        BACKGROUND_BLE_ENABLED TRUE
        BLUETOOTH_PERMISSION_ENABLED TRUE
        BLUETOOTH_PERMISSION_TEXT "ModuleRack uses Bluetooth to connect to wireless MIDI controllers."

        # Split View / Slide Over: an instrument gets used next to its host.
        REQUIRES_FULL_SCREEN FALSE
        IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft
                                 UIInterfaceOrientationLandscapeRight
                                 UIInterfaceOrientationPortrait
                                 UIInterfaceOrientationPortraitUpsideDown

        # Patches show up in the Files app under On My iPad > ModuleRack.
        FILE_SHARING_ENABLED TRUE)

    if (MODULERACK_APP_GROUP_ID)
        list(APPEND MODULERACK_IOS_OPTIONS
             APP_GROUPS_ENABLED TRUE
             APP_GROUP_IDS ${MODULERACK_APP_GROUP_ID})
    endif()
elseif (APPLE)
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
    ${MODULERACK_IOS_OPTIONS}
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
    Source/UI/PresetBrowser.cpp
    Source/UI/PresetBrowser.h
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
