#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererDX12/RendererDX12DLL.h>

#include <d3dx12/d3dx12.h>
using namespace Microsoft::WRL;
struct plD3D12AllocationCreateFlags
{
  using StorageType = plUInt32;
  enum Enum
  {
    Committed = 0x0000001,
    Reserved = 0x0000002,
    Placed = 0x00000002,

    Defualt = 0,
  };
  struct Bits
  {
    StorageType DedicatedMemory : 1;
    StorageType NeverAllocate : 1;
    StorageType Mapped : 1;
    StorageType UnusedBit3 : 1;

    StorageType UnusedBit4 : 1;
    StorageType UnusedBit5 : 1;
    StorageType UnusedBit6 : 1;
    StorageType UnusedBit7 : 1;

    StorageType UnusedBit8 : 1;
    StorageType CanAlias : 1;
    StorageType HostAccessSequentialWrite : 1;
    StorageType HostAccessRandom : 1;

    StorageType UnusedBit12 : 1;
    StorageType UnusedBit13 : 1;
    StorageType UnusedBit14 : 1;
    StorageType StrategyMinMemory : 1;

    StorageType StrategyMinTime : 1;
  };
};
PLASMA_DECLARE_FLAGS_OPERATORS(plD3D12AllocationCreateFlags);

struct plD3D12MemoryUsage
{
  typedef plUInt8 StorageType;
  enum Enum
  {
    Unknown = 0,
    GpuLazilyAllocated = 6,
    Auto = 7,
    AutoPreferDevice = 8,
    AutoPreferHost = 9,
    Default = Unknown,
  };
};

struct plD3D12AllocationCreateInfo
{
  plBitflags<plD3D12AllocationCreateFlags> m_flags;
  plEnum<plD3D12MemoryUsage> m_usage;
  const char* m_pUserData = nullptr;

  bool m_bExportSharedAllocation = false; // If this allocation should be exported so other processes can access it.
};

/// \brief Subset of VmaAllocationInfo. Duplicated for abstraction purposes.
struct plD3D12AllocationInfo
{
  uint32_t m_memoryType;
  plUInt64 m_deviceMemory;
  plUInt64 m_offset;
  plUInt64 m_size;
  void* m_pMappedData;
  void* m_pUserData;
  const char* m_pName;
};

struct plD3D12ImageCreationInfo
{
  D3D12_RESOURCE_DESC rdesc = {};
  D3D12_SUBRESOURCE_DATA srdesc = {};
};

namespace D3D12MA
{
  class Allocation;
}

class IDXGIAdapter3;
class ID3D12Device;
using plD3D12Allocation = D3D12MA::Allocation;

class PLASMA_RENDERERDX12_DLL plMemoryAllocatorD3D12
{
public:
  static plResult Initialize(IDXGIAdapter3* physicalDevice, ID3D12Device* device);
  static void DeInitialize();

  static plResult CreateImage(plD3D12ImageCreationInfo& ImageInfo, const plD3D12AllocationCreateInfo allocationCreateInfo, ComPtr<ID3D12Resource>& out_image_resource, plD3D12Allocation& out_alloc, D3D12_RESOURCE_STATES image_state = D3D12_RESOURCE_STATE_COMMON, plD3D12AllocationInfo* pAllocInfo = nullptr);
  static void DestroyImage(ComPtr<ID3D12Resource>& image_resource, plD3D12Allocation& alloc);

  static plResult CreateBuffer(plD3D12ImageCreationInfo& ImageInfo, const plD3D12AllocationCreateInfo allocationCreateInfo, ComPtr<ID3D12Resource>& out_image_resource, plD3D12Allocation& out_alloc, plD3D12AllocationInfo* pAllocInfo = nullptr);
  static void DestroyBuffer(ComPtr<ID3D12Resource>& image_resource, plD3D12Allocation& alloc);

  static plD3D12AllocationInfo GetAllocationInfo(plD3D12Allocation alloc);
  static void SetAllocationUserData(plD3D12Allocation alloc, const char* pUserData);

private:
  struct Impl;
  static Impl* m_pImpl;
};