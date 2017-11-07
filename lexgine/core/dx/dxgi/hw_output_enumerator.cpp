#include "hw_output_enumerator.h"

using namespace lexgine::core::dx::dxgi;



HwOutputEnumerator::HwOutputEnumerator() :
    m_adapter{ nullptr },
    m_iterated_index{ -1 }
{

}

HwOutputEnumerator::HwOutputEnumerator(ComPtr<IDXGIAdapter3> const& adapter, LUID adapter_luid) :
    m_adapter{ adapter },
    m_adapter_luid{ adapter_luid.LowPart, adapter_luid.HighPart }
{
    HRESULT rv = S_OK;
    UINT id = 0;
    IDXGIOutput* p_dxgi_output;
    while ((rv = m_adapter->EnumOutputs(id, &p_dxgi_output)) != DXGI_ERROR_NOT_FOUND)
    {
        if (rv != S_OK)
        {
            DXGI_ADAPTER_DESC2 desc2;
            m_adapter->GetDesc2(&desc2);

            char* adapter_name = new char[wcslen(desc2.Description)];
            for (size_t i = 0; i < wcslen(desc2.Description); ++i)
                adapter_name[i] = static_cast<char>(desc2.Description[i]);
            std::string err_msg = "unable to enumerate output devices for adapter \"" + std::string{ adapter_name } +"\"";
            delete[] adapter_name;
            logger().out(err_msg, misc::LogMessageType::error);
            raiseError(err_msg);
        }
        IDXGIOutput5* p_dxgi_output5;
        p_dxgi_output->QueryInterface(__uuidof(IDXGIOutput5), reinterpret_cast<void**>(&p_dxgi_output5));
        m_outputs.emplace_back(HwOutput{ p_dxgi_output5 });
        p_dxgi_output5->Release();
        p_dxgi_output->Release();

        ++id;
    }

    m_outputs_iterator = m_outputs.begin();
    m_iterated_index = 0;
}

HwOutput& HwOutputEnumerator::operator*()
{
    return *m_outputs_iterator;
}

HwOutput const& HwOutputEnumerator::operator*() const
{
    return *m_outputs_iterator;
}

HwOutputEnumerator::iterator& HwOutputEnumerator::operator->()
{
    return m_outputs_iterator;
}

HwOutput const* HwOutputEnumerator::operator->() const
{
    return &(*m_outputs_iterator);
}

HwOutputEnumerator& HwOutputEnumerator::operator++()
{
    ++m_outputs_iterator;
    return *this;
}

HwOutputEnumerator HwOutputEnumerator::operator++(int)
{
    HwOutputEnumerator aux{ *this };
    ++m_outputs_iterator;
    return aux;
}

HwOutputEnumerator& HwOutputEnumerator::operator--()
{
    --m_outputs_iterator;
    return *this;
}

HwOutputEnumerator HwOutputEnumerator::operator--(int)
{
    HwOutputEnumerator aux{ *this };
    --m_outputs_iterator;
    return aux;
}

bool HwOutputEnumerator::operator==(HwOutputEnumerator const& other) const
{
    return m_adapter_luid.LowPart == other.m_adapter_luid.LowPart
        && m_adapter_luid.HighPart == other.m_adapter_luid.HighPart
        && m_iterated_index == other.m_iterated_index;
}

bool HwOutputEnumerator::operator!=(HwOutputEnumerator const& other) const
{
    return !(*this == other);
}

HwOutputEnumerator::iterator HwOutputEnumerator::begin()
{
    return m_outputs.begin();
}

HwOutputEnumerator::iterator HwOutputEnumerator::end()
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




HwOutput::HwOutput(ComPtr<IDXGIOutput5> const& output) : m_output{ output }
{

}

HwOutput::Description HwOutput::getDescription() const
{
    DXGI_OUTPUT_DESC desc;
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_output->GetDesc(&desc),
        S_OK
    );

    return Description{ desc.DeviceName, math::Rectangle{math::vector2f{static_cast<float>(desc.DesktopCoordinates.left), static_cast<float>(desc.DesktopCoordinates.top)},
        static_cast<float>(desc.DesktopCoordinates.right - desc.DesktopCoordinates.left), static_cast<float>(desc.DesktopCoordinates.top - desc.DesktopCoordinates.bottom)} };
}

std::vector<HwOutput::DisplayMode> HwOutput::getDisplayModes(DXGI_FORMAT format, HwOutput::DisplayModeEnumerationOptions const& options) const
{
    UINT num_modes;
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_output->GetDisplayModeList1(format, static_cast<DisplayModeEnumerationOptions::base_int_type>(options), &num_modes, NULL),
        S_OK
    );

    DXGI_MODE_DESC1* p_descs = new DXGI_MODE_DESC1[num_modes];
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_output->GetDisplayModeList1(format, static_cast<DisplayModeEnumerationOptions::base_int_type>(options), &num_modes, p_descs),
        S_OK
    );

    std::vector<DisplayMode> rv{ num_modes };
    for (uint32_t i = 0; i < num_modes; ++i)
    {
        rv[i].width = p_descs[i].Width;
        rv[i].height = p_descs[i].Height;
        rv[i].refresh_rate = p_descs[i].RefreshRate;
        rv[i].format = p_descs[i].Format;
        rv[i].scanline_order = p_descs[i].ScanlineOrdering;
        rv[i].scaling = p_descs[i].Scaling;
        rv[i].is_stereo = p_descs[i].Stereo == TRUE;
    }

    delete[] p_descs;

    return rv;
}

HwOutput::DisplayMode HwOutput::findMatchingDisplayMode(DisplayMode const& mode_to_match) const
{
    DXGI_MODE_DESC1 mode_to_seek{ mode_to_match.width, mode_to_match.height, mode_to_match.refresh_rate,
        mode_to_match.format, mode_to_match.scanline_order, mode_to_match.scaling, mode_to_match.is_stereo };

    DXGI_MODE_DESC1 found_mode_desc;
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_output->FindClosestMatchingMode1(&mode_to_seek, &found_mode_desc, NULL),
        S_OK
    );

    return DisplayMode{ found_mode_desc.Width, found_mode_desc.Height, found_mode_desc.RefreshRate,
    found_mode_desc.Format, found_mode_desc.ScanlineOrdering, found_mode_desc.Scaling, found_mode_desc.Stereo == TRUE };
}

void HwOutput::waitForVBI() const
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_output->WaitForVBlank(),
        S_OK
    );
}