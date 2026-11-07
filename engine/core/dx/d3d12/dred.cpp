#include <algorithm>
#include <format>
#include <sstream>

#include <d3d12.h>
#include <wrl.h>

#include "engine/core/misc/misc.h"

#include "device.h"
#include "dred.h"


namespace lexgine::core::dx::d3d12 {

namespace
{
    constexpr uint32_t c_reported_operations_before_failure = 4;
    constexpr uint32_t c_reported_operations_after_failure = 4;

    std::string operationName(D3D12_AUTO_BREADCRUMB_OP operation)
    {
        switch (operation)
        {
        case D3D12_AUTO_BREADCRUMB_OP_SETMARKER: return "SetMarker";
        case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT: return "BeginEvent";
        case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT: return "EndEvent";
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED: return "DrawInstanced";
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED: return "DrawIndexedInstanced";
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT: return "ExecuteIndirect";
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCH: return "Dispatch";
        case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION: return "CopyBufferRegion";
        case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION: return "CopyTextureRegion";
        case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE: return "CopyResource";
        case D3D12_AUTO_BREADCRUMB_OP_COPYTILES: return "CopyTiles";
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE: return "ResolveSubresource";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW: return "ClearRenderTargetView";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW: return "ClearUnorderedAccessView";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW: return "ClearDepthStencilView";
        case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER: return "ResourceBarrier";
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE: return "ExecuteBundle";
        case D3D12_AUTO_BREADCRUMB_OP_PRESENT: return "Present";
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA: return "ResolveQueryData";
        case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION: return "BeginSubmission";
        case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION: return "EndSubmission";
        case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE: return "WriteBufferImmediate";
        case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE: return "BuildRaytracingAccelerationStructure";
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS: return "DispatchRays";
        case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1: return "SetPipelineState1";
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH: return "DispatchMesh";
        case D3D12_AUTO_BREADCRUMB_OP_BARRIER: return "Barrier";
        default: return std::format("operation {}", static_cast<int>(operation));
        }
    }

    std::string debugName(wchar_t const* wide_name, char const* narrow_name)
    {
        if (wide_name) return misc::wstringToAsciiString(wide_name);
        if (narrow_name) return narrow_name;
        return "<unnamed>";
    }

    void describeBreadcrumbs(D3D12_AUTO_BREADCRUMB_NODE1 const* p_node, std::ostringstream& report)
    {
        bool any_incomplete = false;
        for (; p_node; p_node = p_node->pNext)
        {
            uint32_t const completed_count = p_node->pLastBreadcrumbValue ? *p_node->pLastBreadcrumbValue : 0;
            if (completed_count >= p_node->BreadcrumbCount) continue;

            any_incomplete = true;
            report << std::format("queue \"{}\", command list \"{}\": {} of {} operations completed\n",
                debugName(p_node->pCommandQueueDebugNameW, p_node->pCommandQueueDebugNameA),
                debugName(p_node->pCommandListDebugNameW, p_node->pCommandListDebugNameA),
                completed_count, p_node->BreadcrumbCount);

            uint32_t const first_reported = completed_count > c_reported_operations_before_failure
                ? completed_count - c_reported_operations_before_failure : 0;
            uint32_t const last_reported = (std::min)(p_node->BreadcrumbCount,
                completed_count + c_reported_operations_after_failure + 1);
            for (uint32_t i = first_reported; i < last_reported; ++i)
            {
                report << std::format("  {} [{}] {}", i == completed_count ? '>' : ' ', i,
                    operationName(p_node->pCommandHistory[i]));

                for (uint32_t c = 0; c < p_node->BreadcrumbContextsCount; ++c)
                {
                    D3D12_DRED_BREADCRUMB_CONTEXT const& context = p_node->pBreadcrumbContexts[c];
                    if (context.BreadcrumbIndex == i && context.pContextString)
                    {
                        report << " \"" << misc::wstringToAsciiString(context.pContextString) << '"';
                    }
                }

                report << '\n';
            }
        }

        if (!any_incomplete) report << "no incomplete command lists\n";
    }

    void describeAllocations(char const* title, D3D12_DRED_ALLOCATION_NODE1 const* p_node, std::ostringstream& report)
    {
        if (!p_node) return;

        report << title << ":\n";
        for (; p_node; p_node = p_node->pNext)
        {
            report << std::format("  \"{}\" (allocation type {})\n",
                debugName(p_node->ObjectNameW, p_node->ObjectNameA), static_cast<int>(p_node->AllocationType));
        }
    }

    void describePageFault(D3D12_DRED_PAGE_FAULT_OUTPUT1 const& page_fault, std::ostringstream& report)
    {
        if (!page_fault.PageFaultVA)
        {
            report << "no page fault\n";
            return;
        }

        report << std::format("page fault at GPU virtual address 0x{:016X}\n", page_fault.PageFaultVA);
        describeAllocations("live allocations covering the address", page_fault.pHeadExistingAllocationNode, report);
        describeAllocations("recently freed allocations covering the address", page_fault.pHeadRecentFreedAllocationNode, report);
    }
}  // namespace

std::string describeDeviceRemoval(Device const& device)
{
    Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
    if (FAILED(device.native().As(&dred))) return {};

    std::ostringstream report{};

    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs{};
    if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&breadcrumbs)))
    {
        describeBreadcrumbs(breadcrumbs.pHeadAutoBreadcrumbNode, report);
    }

    D3D12_DRED_PAGE_FAULT_OUTPUT1 page_fault{};
    if (SUCCEEDED(dred->GetPageFaultAllocationOutput1(&page_fault)))
    {
        describePageFault(page_fault, report);
    }

    return report.str();
}

}
