#ifndef LEXGINE_CORE_DX_D3D12_LEXGINE_CORE_DX_D3D12_FWD_H
#define LEXGINE_CORE_DX_D3D12_LEXGINE_CORE_DX_D3D12_FWD_H

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
class DebugInterface;
class DescriptorHeap;
class DescriptorTable_CBV_UAV_SRV;
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
class Signal;

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

template<typename> struct DescriptorTable;
struct tag_CBV_SRV_UAV_Sampler;
struct tag_Sampler;
struct tag_RTV;
struct tag_DSV;
using ShaderResourceDescriptorTable = DescriptorTable<tag_CBV_SRV_UAV_Sampler>;
using RenderTargetViewDescriptorTable = DescriptorTable<tag_RTV>;
using DepthStencilViewDescriptorTable = DescriptorTable<tag_DSV>;


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
