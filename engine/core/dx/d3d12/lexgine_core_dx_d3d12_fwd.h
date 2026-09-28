#ifndef LEXGINE_CORE_DX_D3D12_LEXGINE_CORE_DX_D3D12_FWD_H
#define LEXGINE_CORE_DX_D3D12_LEXGINE_CORE_DX_D3D12_FWD_H

#include <cstdint>

#include <d3d12.h>
#include <wrl.h>

namespace lexgine::core::dx::d3d12 {

class DebugInterface;

class CommandAllocatorRing;
class CommandList;
class CommandQueue;
class ConstantBuffer;
class ConstantBufferReflection;
class ConstantBufferDataWriter;
class ConstantBufferDataMapper;
class D3D12PSOXMLParser;
class D3DDataBlob;
class DebugInterface;
class DescriptorHeap;
class DescriptorAllocator;
class DescriptorAllocationManager;
class UnorderedSRVTableAllocationManager;
class Device;
class DxResourceFactory;
class Fence;
class Heap;
class ResourceDataUploader;
class HeapResourcePlacer;
struct GraphicsPSODescriptor;
struct ComputePSODescriptor;
class PipelineState;
class Resource;
class PlacedResource;
class CommittedResource;
class ResourceBarrierPack;
class DynamicResourceBarrierPack;
template<unsigned int capacity> class StaticResourceBarrierPack;
class RootSignature;
class CompiledRootSignature;
class Signal;

//! Enumerates shader-visible memory resource types used in descriptor table ranges. Complies with D3D12 constant definitions
enum class ShaderVisibleMemoryResourceType : uint8_t
{
    srv, uav, cbv, sampler, count      // note: the order is important for compliance with D3D12 API
};

struct SRVBufferInfo;
struct SRVTextureInfo;
struct SRVTextureArrayInfo;
class SRVDescriptor;

struct UAVBufferInfo;
struct UAVTextureInfo;
struct UAVTextureArrayInfo;
class UAVDescriptor;

class CBVDescriptor;

struct RTVBufferInfo;
struct RTVTextureInfo;
struct RTVTextureArrayInfo;
class RTVDescriptor;

struct DSVTextureInfo;
struct DSVTextureArrayInfo;
class DSVDescriptor;

class SamplerDescriptor;

struct DescriptorTable;

class ResourceViewDescriptorTableBuilder;
class SamplerDescriptorTableBuilder;
class RenderTargetViewTableBuilder;
class DepthStencilViewTableBuilder;

class RenderingTasks;
class BasicRenderingServices;

class RenderingTarget;
class SwapChainLink;

class VertexBufferBinding;
class IndexBufferBinding;

class FrameProgressTracker;

class UploadBufferAllocator;
class DedicatedUploadDataStreamAllocator;
class PerFrameUploadDataStreamAllocator;

class QueryCache;
struct QueryHandle;

class DxgiFormatFetcher;

}

#endif
