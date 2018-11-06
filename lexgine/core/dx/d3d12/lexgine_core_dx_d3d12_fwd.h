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

struct ShaderResourceViewBufferInfo;
struct ShaderResourceViewTextureInfo;
struct ShaderResourceViewTextureArrayInfo;
class ShaderResourceViewDescriptor;

class UnorderedAccessViewDescriptor;
class ConstantBufferViewDescriptor;

struct RenderTargetViewBufferInfo;
struct RenderTargetViewTextureInfo;
struct RenderTargetViewTextureArrayInfo;
class RenderTargetViewDescriptor;

class DepthStencilViewDescriptor;


}

#endif
