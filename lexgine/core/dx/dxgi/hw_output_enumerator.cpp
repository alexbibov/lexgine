#include "hw_output_enumerator.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/misc/misc.h"

using namespace lexgine::core::dx::dxgi;


HwOutputEnumerator::HwOutputEnumerator(ComPtr<IDXGIAdapter4> const& adapter, LUID adapter_luid) :
    m_adapter{ adapter },
    m_adapter_luid{ adapter_luid.LowPart, adapter_luid.HighPart }
{
    HRESULT rv = S_OK;
    UINT id = 0;
    IDXGIOutput* dxgi_output{ nullptr };
    while ((rv = m_adapter->EnumOutputs(id, &dxgi_output)) != DXGI_ERROR_NOT_FOUND)
    {
        if (rv != S_OK)
        {
            DXGI_ADAPTER_DESC3 desc;
            m_adapter->GetDesc3(&desc);

            std::string output_description = misc::wstringToAsciiString(desc.Description);
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
                "unable to enumerate output devices for adapter \"" + output_description + "\"");
        }

        ComPtr<IDXGIOutput6> dxgi_output6;
        dxgi_output->QueryInterface(IID_PPV_ARGS(&dxgi_output6));
        dxgi_output->Release();
        m_outputs.emplace_back(HwOutput{ dxgi_output6 });

        ++id;
    }
}

HwOutputEnumerator::const_iterator HwOutputEnumerator::begin() const
{
    return m_outputs.begin();
}

HwOutputEnumerator::const_iterator HwOutputEnumerator::end() const
{
    return m_outputs.end();
}

HwOutputEnumerator::const_iterator HwOutputEnumerator::cbegin() const
{
    return m_outputs.cbegin();
}

HwOutputEnumerator::const_iterator HwOutputEnumerator::cend() const
{
    return m_outputs.cend();
}




HwOutput::HwOutput(ComPtr<IDXGIOutput6> const& output): 
    m_output{ output }
{

}

HwOutput::Description HwOutput::getDescription() const
{
    DXGI_OUTPUT_DESC1 desc;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_output->GetDesc1(&desc),
        S_OK
    );

    Description rv;
    rv.name = desc.DeviceName;

    {
        math::Rectangle desktop_coordinates_rect{};
        desktop_coordinates_rect.setUpperLeft(math::Vector2f{
            static_cast<float>(desc.DesktopCoordinates.left),
            static_cast<float>(desc.DesktopCoordinates.top) });
        desktop_coordinates_rect.setSize(
            static_cast<float>(desc.DesktopCoordinates.right - desc.DesktopCoordinates.left),
            static_cast<float>(desc.DesktopCoordinates.top - desc.DesktopCoordinates.bottom));

        rv.desktop_coordinates = desktop_coordinates_rect;
    }

    rv.is_attached_to_desktop = desc.AttachedToDesktop;;
    rv.rotation = static_cast<DxgiRotation>(desc.Rotation);
    rv.monitor = desc.Monitor;
    rv.bits_per_color = desc.BitsPerColor;
    rv.color_space_type = static_cast<DxgiColorSpaceType>(desc.ColorSpace);
    rv.red_primary = math::Vector2f{ desc.RedPrimary[0], desc.RedPrimary[1] };
    rv.green_primary = math::Vector2f{ desc.GreenPrimary[0], desc.GreenPrimary[1] };
    rv.blue_primary = math::Vector2f{ desc.BluePrimary[0], desc.BluePrimary[1] };
    rv.white_point = math::Vector2f{ desc.WhitePoint[0], desc.WhitePoint[1] };
    rv.minimal_luminance = desc.MinLuminance;
    rv.maximal_luminance = desc.MaxLuminance;
    rv.maximal_full_frame_luminance = desc.MaxFullFrameLuminance;

    return rv;
}

std::vector<HwOutput::DisplayMode> HwOutput::getDisplayModes(DXGI_FORMAT format, HwOutput::DisplayModeEnumerationOptions const& options) const
{
    UINT num_modes;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_output->GetDisplayModeList1(format, static_cast<DisplayModeEnumerationOptions::int_type>(options), &num_modes, NULL),
        S_OK
    );

    DXGI_MODE_DESC1* p_descs = new DXGI_MODE_DESC1[num_modes];
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_output->GetDisplayModeList1(format, static_cast<DisplayModeEnumerationOptions::int_type>(options), &num_modes, p_descs),
        S_OK
    );

    std::vector<DisplayMode> rv{ num_modes };
    for (uint32_t i = 0; i < num_modes; ++i)
    {
        rv[i].width = p_descs[i].Width;
        rv[i].height = p_descs[i].Height;
        rv[i].refresh_rate = p_descs[i].RefreshRate;
        rv[i].format = p_descs[i].Format;
        rv[i].scanline_order = static_cast<DxgiModeScanlineOrder>(p_descs[i].ScanlineOrdering);
        rv[i].scaling = static_cast<DxgiModeScaling>(p_descs[i].Scaling);
        rv[i].is_stereo = p_descs[i].Stereo == TRUE;
    }

    delete[] p_descs;

    return rv;
}

HwOutput::DisplayMode HwOutput::findMatchingDisplayMode(DisplayMode const& mode_to_match) const
{
    DXGI_MODE_DESC1 mode_to_seek{ mode_to_match.width, mode_to_match.height, mode_to_match.refresh_rate,
        mode_to_match.format, static_cast<DXGI_MODE_SCANLINE_ORDER>(mode_to_match.scanline_order), 
        static_cast<DXGI_MODE_SCALING>(mode_to_match.scaling), mode_to_match.is_stereo };

    DXGI_MODE_DESC1 found_mode_desc;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_output->FindClosestMatchingMode1(&mode_to_seek, &found_mode_desc, NULL),
        S_OK
    );

    return DisplayMode{ found_mode_desc.Width, found_mode_desc.Height, found_mode_desc.RefreshRate,
    found_mode_desc.Format, static_cast<DxgiModeScanlineOrder>(found_mode_desc.ScanlineOrdering), 
        static_cast<DxgiModeScaling>(found_mode_desc.Scaling), found_mode_desc.Stereo == TRUE };
}

void HwOutput::waitForVBI() const
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_output->WaitForVBlank(),
        S_OK
    );
}