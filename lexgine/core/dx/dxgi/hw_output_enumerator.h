#ifndef LEXGINE_CORE_DX_DXGI_HW_OUTPUT_ENUMERATOR_H

#include <dxgi1_5.h>
#include <wrl.h>

#include <list>
#include <vector>
#include <ratio>
#include <cstdint>

#include "../../math/rectangle.h"
#include "../../misc/flags.h"
#include "../../entity.h"
#include "../../class_names.h"

namespace lexgine {namespace core {namespace dx {namespace dxgi {


using namespace Microsoft::WRL;



//! Represents hardware output device
class HwOutput final : public NamedEntity<class_names::HwOutput>
{
    friend class HwOutputEnumerator;    // HwOutput objects can only be created by the corresponding enumerators, which is just logical
private:
    enum class tagDisplayModeEnumerationOptions
    {
        IncludeInterlaced = DXGI_ENUM_MODES_INTERLACED,    //!< enumerate interlaced modes
        IncludeScaling = DXGI_ENUM_MODES_SCALING,    //!< enumerate scaled display modes
        IncludeStereo = DXGI_ENUM_MODES_STEREO,    //!< enumerate stereo display modes
        IncludeDisabledStereo = DXGI_ENUM_MODES_DISABLED_STEREO    //!< enumerate stereo display modes that has been disabled via control panel
    };

public:
    struct Description  //! Description of DXGI output device
    {
        std::wstring name;  //!< User-friendly name of DXGI output device
        math::Rectangle desctop_coordinates;    //!< Rectangular area corresponding to the output in desktop coordinates
        bool is_attached_to_desktop;    //!< 'True' if the output is attached to desctop; otherwise 'False'
        DXGI_MODE_ROTATION rotation;    //!< DXGI rotation mode of the output. See MSDN pages for further details
        HMONITOR monitor;   //!< Windows monitor handle
    };

    struct DisplayMode //! Wraps information about display mode
    {
        uint32_t width; //!< Width of the display mode in pixels
        uint32_t height;    //!< Height of the display mode in pixels
        DXGI_RATIONAL refresh_rate; //!< Refresh rate of the display mode in hertz
        DXGI_FORMAT format; //!< Color format of the display mode
        DXGI_MODE_SCANLINE_ORDER scanline_order; //!< Scan-line order of the display mode
        DXGI_MODE_SCALING scaling;  //!< Scaling regime of the display mode
        bool is_stereo; //!< Equals 'true' if the display mode is a stereo mode
    };


    using DisplayModeEnumerationOptions = misc::Flags<tagDisplayModeEnumerationOptions, UINT>;    //!< options taken into account when enumerating display modes


    Description getDescription() const; //! Returns description of DXGI output

    std::vector<DisplayMode> getDisplayModes(DXGI_FORMAT format, DisplayModeEnumerationOptions const& options) const;   //! Enumerates display modes meeting requested DXGI color format

    DisplayMode findMatchingDisplayMode(DisplayMode const& mode_to_match) const;    //! Returns closest match for provided display mode

    void waitForVBI() const;    //! Halts the calling thread until the next vertical blank interruption occurs


private:
    explicit HwOutput(ComPtr<IDXGIOutput5> const& output);   //! Constructs wrapper over DXGI output

    ComPtr<IDXGIOutput5> m_output;  //!< interface representing DXGI output
};



//! Enumerates physical output devices attached to a certain DXGI adapter
class HwOutputEnumerator final : public std::iterator<std::bidirectional_iterator_tag, HwOutput>,
    public NamedEntity<class_names::HwOutputEnumerator>
{
    friend class HwAdapter; // output enumerator can only be created by adapter classes, which is logical

public:
    using iterator = std::list<HwOutput>::iterator;
    using const_iterator = std::list<HwOutput>::const_iterator;

    HwOutput& operator*();
    HwOutput const& operator*() const;

    iterator& operator->();
    HwOutput const* operator->() const;

    HwOutputEnumerator& operator++();
    HwOutputEnumerator operator++(int);

    HwOutputEnumerator& operator--();
    HwOutputEnumerator operator--(int);

    bool operator==(HwOutputEnumerator const& other) const;
    bool operator!=(HwOutputEnumerator const& other) const;

    HwOutputEnumerator();   //! Default constructor that creates a dummy enumerator. Needed only for compatibility with the notion of bidirectional iterators employed by the STL

    iterator begin();
    iterator end();

    const_iterator cbegin() const;
    const_iterator cend() const;

private:
    HwOutputEnumerator(ComPtr<IDXGIAdapter3> const& adapter, LUID adapter_luid);   //! Output enumerators can only be created by HwAdapter objects, because only adapter "knows" what outputs it has and hence, is able to enumerate

    ComPtr<IDXGIAdapter3> m_adapter;    //!< DXGI adapter associated with the output enumerator
    std::list<HwOutput> m_outputs;  //!< list of DXGI interfaces representing hardware output
    iterator m_outputs_iterator;    //!< iterator object for the list of DXGI output interfaces
    int32_t m_iterated_index;   //!< index of the currently iterated DXGI interface
    LUID m_adapter_luid;    //!< local identifier of the adapter that has created this enumerator
};

}}}}

#define LEXGINE_CORE_DX_DXGI_HW_OUTPUT_ENUMERATOR_H
#endif