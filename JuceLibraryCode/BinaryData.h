/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   chassis_png;
    const int            chassis_pngSize = 2017356;

    extern const char*   header_png;
    const int            header_pngSize = 7085;

    extern const char*   header_brand_svg;
    const int            header_brand_svgSize = 17680;

    extern const char*   display_png;
    const int            display_pngSize = 136013;

    extern const char*   input_meter_png;
    const int            input_meter_pngSize = 182725;

    extern const char*   output_meter_png;
    const int            output_meter_pngSize = 182857;

    extern const char*   delay_core_png;
    const int            delay_core_pngSize = 177456;

    extern const char*   tone_stereo_png;
    const int            tone_stereo_pngSize = 178429;

    extern const char*   mod_character_png;
    const int            mod_character_pngSize = 337426;

    extern const char*   utility_bar_png;
    const int            utility_bar_pngSize = 182347;

    extern const char*   utility_bar_legacy_png;
    const int            utility_bar_legacy_pngSize = 158277;

    extern const char*   preset_bar_png;
    const int            preset_bar_pngSize = 102730;

    extern const char*   knob_body_png;
    const int            knob_body_pngSize = 15122;

    extern const char*   knob_ticks_svg;
    const int            knob_ticks_svgSize = 3626;

    extern const char*   knob_shadow_svg;
    const int            knob_shadow_svgSize = 448;

    extern const char*   knob_bezel_svg;
    const int            knob_bezel_svgSize = 1500;

    extern const char*   knob_pointer_glow_svg;
    const int            knob_pointer_glow_svgSize = 915;

    extern const char*   knob_pointer_svg;
    const int            knob_pointer_svgSize = 720;

    extern const char*   knob_pointer_highlight_svg;
    const int            knob_pointer_highlight_svgSize = 489;

    extern const char*   freeze_icon_svg;
    const int            freeze_icon_svgSize = 717;

    extern const char*   bypass_icon_svg;
    const int            bypass_icon_svgSize = 497;

    extern const char*   BreeSerifRegular_ttf;
    const int            BreeSerifRegular_ttfSize = 46572;

    extern const char*   Arimo_ttf;
    const int            Arimo_ttfSize = 496268;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 23;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
