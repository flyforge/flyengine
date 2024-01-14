#include <RendererDX12/MemoryAllocation/D3D12MA.h>
#include <RendererDX12/MemoryAllocation/D3D12MemoryAllocation.h>
#include <RendererDX12/RendererDX12PCH.h>
#include <RendererDX12/RendererDX12Helpers.h>

PLASMA_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT == plD3D12AllocationCreateFlags::DedicatedMemory);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT == plD3D12AllocationCreateFlags::NeverAllocate);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_MAPPED_BIT == plD3D12AllocationCreateFlags::Mapped);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_CAN_ALIAS_BIT == plD3D12AllocationCreateFlags::CanAlias);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT == plD3D12AllocationCreateFlags::HostAccessSequentialWrite);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT == plD3D12AllocationCreateFlags::HostAccessRandom);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT == plD3D12AllocationCreateFlags::StrategyMinMemory);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT == plD3D12AllocationCreateFlags::StrategyMinTime);

PLASMA_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_UNKNOWN == plD3D12MemoryUsage::Unknown);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED == plD3D12MemoryUsage::GpuLazilyAllocated);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_AUTO == plD3D12MemoryUsage::Auto);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_AUTO_PREFER_DEVICE == plD3D12MemoryUsage::AutoPreferDevice);
PLASMA_CHECK_AT_COMPILETIME(D3D12MA_MEMORY_USAGE_AUTO_PREFER_HOST == plD3D12MemoryUsage::AutoPreferHost);
struct ExportedSharedPool
{
  D3D12MA::Pool m_pool;
};
struct plMemoryAllocatorD3D12::Impl
{
  Impl();
  D3D12MA::Allocator m_allocator;
  plMutex m_exportedSharedPoolsMutex;
  plHashTable<uint32_t, ExportedSharedPool> m_exportedSharedPools;
};

plMemoryAllocatorD3D12::Impl* plMemoryAllocatorD3D12::m_pImpl = nullptr;

plResult plMemoryAllocatorD3D12::Initialize(IDXGIAdapter3* physicalDevice, ID3D12Device* device)
{
  PLASMA_ASSERT_DEV(m_pImpl == nullptr, "plMemoryAllocatorD3D12::Initialize was already called");
  m_pImpl = PLASMA_DEFAULT_NEW(Impl);

  D3D12MA::ALLOCATOR_DESC desc = {};

  desc.pDevice = device;
  desc.pAdapter = physicalDevice;

  HRESULT hr = D3D12MA::CreateAllocator(&desc, (D3D12MA::Allocator**)&m_pImpl->m_allocator);
  if (FAILED(hr))
  {
    plLog::Error("D3D12MemoryAllocator: Failed to create AMDD3D12MA Allocator Object. is the DX12 Device or Adapter valid? HRESULT:{0}", HrToString(hr));
    PLASMA_DEFAULT_DELETE(m_pImpl);
    return PLASMA_FAILURE;
  }
  return PLASMA_SUCCESS;
}

void plMemoryAllocatorD3D12::DeInitialize()
{
  PLASMA_ASSERT_DEV(m_pImpl != nullptr, "nsMemoryAllocatorD3D12 is not initialized. it is either already destroyed or invalid.");
  for (auto it : m_pImpl->m_exportedSharedPools)
  {
    it.Value().m_pool.Release();
  }
  m_pImpl->m_exportedSharedPools.Clear();
  m_pImpl->m_allocator.Release();
  PLASMA_DEFAULT_DELETE(m_pImpl);
}

plResult plMemoryAllocatorD3D12::CreateImage(plD3D12ImageCreationInfo& ImageInfo, const plD3D12AllocationCreateInfo allocationCreateInfo, ComPtr<ID3D12Resource>& out_image_resource, plD3D12Allocation& out_alloc, D3D12_RESOURCE_STATES image_state = D3D12_RESOURCE_STATE_COMMON, plD3D12AllocationInfo* pAllocInfo /*= nullptr*/)
{
  D3D12MA::ALLOCATION_DESC allocdesc = {};
  allocdesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
  if (allocationCreateInfo.m_bExportSharedAllocation)
  {
    allocdesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
  }
  D3D12MA::POOL_DESC pooldesc = {};
  pooldesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  HRESULT hr = m_pImpl->m_allocator.CreateResource(&allocdesc, &ImageInfo.rdesc, image_state, NULL, (D3D12MA::Allocation**)& out_alloc, IID_PPV_ARGS(&out_image_resource));
  if (FAILED(hr))
  {
    plLog::Error("D3D12MemoryAllocator: Failed to create AMDD3D12MA Allocator Object. is the DX12 Device or Adapter valid? HRESULT:{0}", HrToString(hr));
    return PLASMA_FAILURE;
  }
}

void plMemoryAllocatorD3D12::DestroyImage(ComPtr<ID3D12Resource>& image_resource, plD3D12Allocation& alloc)
{
  image_resource->Release();
  alloc.Release();
}

plResult plMemoryAllocatorD3D12::CreateBuffer(plD3D12ImageCreationInfo& ImageInfo, const plD3D12AllocationCreateInfo allocationCreateInfo, ComPtr<ID3D12Resource>& out_image_resource, plD3D12Allocation& out_alloc, plD3D12AllocationInfo* pAllocInfo /*= nullptr*/)
{
}