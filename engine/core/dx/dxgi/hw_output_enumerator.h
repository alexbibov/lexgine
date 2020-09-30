#ifndef LEXGINE_CORE_DX_DXGI_HW_OUTPUT_ENUMERATOR_H
#define LEXGINE_CORE_DX_DXGI_HW_OUTPUT_ENUMERATOR_H

#include <list>
#include <vector>
#include <ratio>
#include <cstdint>

#include "engine/core/math/rectangle.h"
#include "engine/core/misc/flags.h"
#include "engine/core/entity.h"
#include "engine/core/class_names.h"

#include "common.h"

namespace lexgine::core::dx::dxgi {


using namespace Microsoft::WRL;



//! Represents hardware output device
class HwOutput final : public NamedEntity<class_names::DXGI_HwOutput>
{
    friend class HwOutputEnumerator;    // HwOutput objects can only be created by the corresponding enumerators, which is just logical

public:
    enum class DxgiRotation
    {
        unspecified = DXGI_MODE_ROTATION_UNSPECIFIED,
        identity = DXGI_MODE_ROTATION_IDENTITY,
        rotate90 = DXGI_MODE_ROTATION_ROTATE90,
        rotate180 = DXGI_MODE_ROTATION_ROTATE180,
        rotate270 = DXGI_MODE_ROTATION_ROTATE270
    };

    enum class DxgiModeScanlineOrder
    {
        unspecified = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
        progressive = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE,
        upper_field_first = DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST,
        lower_field_first = DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST
    };

    enum class DxgiModeScaling
    {
        unspecified = DXGI_MODE_SCALING_UNSPECIFIED,
        centered = DXGI_MODE_SCALING_CENTERED,
        stretched = DXGI_MODE_SCALING_STRETCHED
    };


    /*! DXGI color space definition. The naming convention is as follows:
     <color space>_<range>_<gamma correction exponent>_<siting>_<primaries>_<transfer_matrix>.
     For example rgb_full_g22_none_p709 stays for sRGB color space with full range,
     gamma correction exponent of 2.2, no specific siting, and primary colors as defined by the BT.709 standard.
     The transfer matrix is not used in this example. For more details refer to the sources documenting the
     color spaces in question (e.g. https://docs.microsoft.com/en-us/windows/desktop/api/dxgicommon/ne-dxgicommon-dxgi_color_space_type)
    */
    enum class DxgiColorSpaceType
    {
        rgb_full_g22_none_p709 = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,
        rgb_full_g10_none_p709 = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709,
        rgb_studio_g22_none_p709 = DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709,
        rgb_studio_g22_none_p2020 = DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020,
        reserved = DXGI_COLOR_SPACE_RESERVED,
        ycbcr_full_g22_none_p709_x601 = DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601,
        ycbcr_studio_g22_left_p601 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601,
        ycbcr_full_g22_left_p601 = DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601,
        ycbcr_studio_g22_left_p709 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709,
        ycbcr_full_g22_left_p709 = DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709,
        ycbcr_studio_g22_left_p2020 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020,
        ycbcr_full_g22_left_p2020 = DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020,
        rgb_full_g2084_none_p2020 = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020,
        ycbcr_studio_g2084_left_p2020 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020,
        rgb_studio_g2084_none_p2020 = DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020,
        ycbcr_studio_g22_topleft_p2020 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020,
        ycbcr_studio_g2084_topleft_p2020 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020,
        rgb_full_g22_none_p2020 = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020,
        ycbcr_studio_ghlg_topleft_p2020 = DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020,
        ycbcr_full_ghlg_topleft_p2020 = DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020,
        rgb_studio_g24_none_p709 = DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709,
        rgb_studio_g24_none_p2020 = DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020,
        ycbcr_studio_g24_left_p709 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709,
        ycbcr_studio_g24_left_p2020 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020,
        ycbcr_studio_g24_topleft_p2020 = DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020,
        custom = DXGI_COLOR_SPACE_CUSTOM
    };

    struct Description  //! Description of DXGI output device
    {
        std::wstring name;  //!< User-friendly name of DXGI output device
        math::Rectangle desktop_coordinates;    //!< Rectangular area corresponding to the output in desktop coordinates
        bool is_attached_to_desktop;    //!< 'True' if the output is attached to desctop; otherwise 'False'
        DxgiRotation rotation;    //!< DXGI rotation mode of the output. See MSDN pages for further details
        HMONITOR monitor;   //!< Windows monitor handle
        uint32_t bits_per_color;    //!< The number of bits per color channel for the active wire format of the display attached to this output
        DxgiColorSpaceType color_space_type;    //!< Color space used by the display attached to the given output
        math::Vector2f red_primary;    //!< red primary chromaticity value of the display attached to the given output
        math::Vector2f green_primary;    //!< green primary chromaticity value of the display attached to the given output
        math::Vector2f blue_primary;    //!< blue primary chromaticity value of the display attached to the given output
        math::Vector2f white_point;    //!< chromaticity coordinates of the white point of the display attached to the given output
        float minimal_luminance;    //!< minimal luminance, in nits, that the display attached to the given output is capable of rendering
        float maximal_luminance;    //!< maximal luminance, in nits, that the display attached to the given output is capable of rendering
        float maximal_full_frame_luminance;    //!< maximal luminance, in nits, that the display attached to the given output is capable of rendering, when the color fills the entire area of the panel
    };

    struct DisplayMode //! Wraps information about display mode
    {
        uint32_t width; //!< Width of the display mode in pixels
        uint32_t height;    //!< Height of the display mode in pixels
        DXGI_RATIONAL refresh_rate; //!< Refresh rate of the display mode in hertz
        DXGI_FORMAT format; //!< Color format of the display mode
        DxgiModeScanlineOrder scanline_order; //!< Scan-line order of the display mode
        DxgiModeScaling scaling;  //!< Scaling regime of the display mode
        bool is_stereo; //!< Equals 'true' if the display mode is a stereo mode
    };


    //!< options taken into account when enumerating display modes
    BEGIN_FLAGS_DECLARATION(DisplayModeEnumerationOptions)
        FLAG(include_interlaced, DXGI_ENUM_MODES_INTERLACED)    //!< enumerate interlaced modes
        FLAG(include_scaling, DXGI_ENUM_MODES_SCALING)    //!< enumerate scaled display modes
        FLAG(include_stereo, DXGI_ENUM_MODES_STEREO)    //!< enumerate stereo display modes
        FLAG(include_disabled_stereo, DXGI_ENUM_MODES_DISABLED_STEREO)    //!< enumerate stereo display modes that has been disabled via control panel
        END_FLAGS_DECLARATION(DisplayModeEnumerationOptions)


        Description getDescription() const; //! Returns description of DXGI output

    std::vector<DisplayMode> getDisplayModes(DXGI_FORMAT format, DisplayModeEnumerationOptions const& options) const;   //! Enumerates display modes meeting requested DXGI color format.

    DisplayMode findMatchingDisplayMode(DisplayMode const& mode_to_match) const;    //! Returns closest match for provided display mode.

    void waitForVBI() const;    //! Halts the calling thread until the next vertical blank interruption occurs


private:
    explicit HwOutput(ComPtr<IDXGIOutput6> const& output);   //! Constructs wrapper over DXGI output

    ComPtr<IDXGIOutput6> m_output;  //!< interface representing DXGI output
};



//! Enumerates physical output devices attached to a certain DXGI adapter
class HwOutputEnumerator final : public NamedEntity<class_names::DXGI_HwOutputEnumerator>
{
    friend class HwAdapter; // output enumerator can only be created by adapter classes, which is logical

public:
    using list_of_outputs_type = std::list<HwOutput>;
    using const_iterator = list_of_outputs_type::const_iterator;

public:
    const_iterator begin() const;
    const_iterator end() const;

    const_iterator cbegin() const;
    const_iterator cend() const;

private:
    HwOutputEnumerator(ComPtr<IDXGIAdapter4> const& adapter, LUID adapter_luid);   //! Output enumerators can only be created by HwAdapter objects, because only adapter "knows" what outputs it has and is able to enumerate them

private:
    ComPtr<IDXGIAdapter4> m_adapter;    //!< DXGI adapter associated with the output enumerator
    std::list<HwOutput> m_outputs;  //!< list of DXGI interfaces representing hardware output
    LUID m_adapter_luid;    //!< local identifier of the adapter that has created this enumerator
};

}

#endif