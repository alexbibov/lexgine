#ifndef LEXGINE_CORE_DX_D3D12_LEXGINE_CORE_DX_D3D12_FWD_H
#define LEXGINE_CORE_DX_D3D12_LEXGINE_CORE_DX_D3D12_FWD_H

namespace lexgine::core::dx::d3d12 {

class CommandAllocatorRing;
class CommandList;
class CommandQueue;
class ConstantBuffer;
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
template<size_t> class ResourceBarrier;
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

template<typename T> struct TableReference;
class ResourceViewDescriptorTableBuilder;
class SamplerTableBuilder;
class RenderTargetViewTableBuilder;
class DepthStencilViewTableBuilder;

}

#endif
