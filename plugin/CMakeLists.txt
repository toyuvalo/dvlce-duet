# Dvlce Duet JUCE plugin.  JUCE 8.0.13 (fetched; or a local checkout via FETCHCONTENT_SOURCE_DIR_JUCE).
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.13
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

set(DVLCE_DUET_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND DVLCE_DUET_FORMATS AU)
endif()
if(DVLCE_DUET_AAX_SDK)
    juce_set_aax_sdk_path("${DVLCE_DUET_AAX_SDK}")
    list(APPEND DVLCE_DUET_FORMATS AAX)
endif()

juce_add_plugin(DvlceDuet
    COMPANY_NAME                "DVLCE"
    BUNDLE_ID                   "ca.dvlce.DvlceDuet"
    PLUGIN_MANUFACTURER_CODE    Dvlc
    PLUGIN_CODE                 Duet
    FORMATS                     ${DVLCE_DUET_FORMATS}
    PRODUCT_NAME                "Dvlce Duet"
    VERSION                     "${PROJECT_VERSION}"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    HARDENED_RUNTIME_ENABLED    FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "Dvlce Duet (standalone) gates two live microphones."
    DESCRIPTION                 "Mutual ducking gate for two speakers"
    AU_MAIN_TYPE                kAudioUnitType_Effect
    VST3_CATEGORIES             Fx Dynamics
    AAX_CATEGORY                AAX_ePlugInCategory_Dynamics)

target_sources(DvlceDuet PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)

target_compile_definitions(DvlceDuet PUBLIC
    DVLCE_DUET_VERSION="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DvlceDuet
    PRIVATE
        dvlce_duet_core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Offscreen editor snapshot (a GUI check without a window): DvlceDuetSnapshot <out.png>
add_executable(DvlceDuetSnapshot tools/editor_snapshot.cpp)
target_link_libraries(DvlceDuetSnapshot PRIVATE DvlceDuet)
target_include_directories(DvlceDuetSnapshot PRIVATE "${juce_SOURCE_DIR}/modules")

# Sidechain mode on the real processor (pedalboard cannot feed a sidechain bus): DvlceDuetSidechainTest
add_executable(DvlceDuetSidechainTest tools/sidechain_test.cpp)
target_link_libraries(DvlceDuetSidechainTest PRIVATE DvlceDuet)
target_include_directories(DvlceDuetSidechainTest PRIVATE "${juce_SOURCE_DIR}/modules")
