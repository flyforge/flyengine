#include <RendererDX12/Device/D3D12Device.h>
#include <RendererDX12/RendererDX12PCH.h>
#include <RendererDX12/RendererDX12Helpers.h>
#include <RendererDX12/Device/D3D12Pass.h>

#include <Core/System/Window.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>

namespace
{
  /// Checks the DXGI Adapter Level. 1 = Direct3D 12 Ultimate, 2 = Direct3D 12 regular, 3 = Direct3D 12.0, 0 = Doesn't support DX12.
  int CheckDXGIAdapterLevel(IDXGIAdapter1* adaptertocheck, IDXGIFactory3* factory)
  {
    for (int adaptindex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adaptindex, &adaptertocheck); ++adaptindex)
    {
      DXGI_ADAPTER_DESC1 desc;
      adaptertocheck->GetDesc1(&desc);

      // Don't select the Basic Render Driver adapter.
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      {
        continue;
      }

      // Check if the adapter supports Direct3D 12 Ultimate , and use that for the rest
      // of the application.
      if (SUCCEEDED(D3D12CreateDevice(adaptertocheck, D3D_FEATURE_LEVEL_12_2, _uuidof(ID3D12Device), nullptr)))
      {
        return 1;
      }

      //️ Check if the adapter supports Direct3D 12 regular, and use that for the rest
      // of the application.
      if (SUCCEEDED(D3D12CreateDevice(adaptertocheck, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
      {
        return 2;
      }

      // Check if the adapter supports Direct3D 12.0, and use that for the rest
      // of the application.
      if (SUCCEEDED(D3D12CreateDevice(adaptertocheck, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
      {
        return 3;
      }

      // Else we won't use this iteration's adapter, so release it
      adaptertocheck->Release();
    }

    /// Doesn't support DX12.
    return 0;
  }
}

plInternal::NewInstance<plGALDevice> CreateDX12Device(plAllocatorBase* pAllocator, const plGALDeviceCreationDescription& desc)
{
  return PLASMA_NEW(pAllocator, plGALDeviceDX12, desc);
}

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererDX12, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  plGALDeviceFactory::RegisterCreatorFunc("DX12", &CreateDX12Device, "DX12_PC", "plShaderCompilerDXC_PCDX12");
}

ON_CORESYSTEMS_SHUTDOWN
{
  plGALDeviceFactory::UnregisterCreatorFunc("DX12");
}

PLASMA_END_SUBSYSTEM_DECLARATION;

plResult plGALDeviceDX12::InitPlatform(D3D_FEATURE_LEVEL platformfeaturelevel, IDXGIAdapter1 * wantedadapterused)
{
  PLASMA_LOG_BLOCK("plGALDeviceDX12::InitPlatform");

  /// Check if we need the debug layer.
  bool createDebugDevice;

  if(m_Description.m_bDebugDevice)
    createDebugDevice = true;
  else
    createDebugDevice = false;

  /// Enable Debug Layer, this will decrease performance by a bit.
  if (createDebugDevice == true)
  {
    plLog::SeriousWarning("D3D12: Debug Layer is Enabled. this will degrade performance. ensure that this isn't enabled on release.");

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_pDebug))))
    {
      m_pDebug->EnableDebugLayer();

      /// Set GPU Validation. Keeps stuff from going very bad.
      m_pDebug->SetEnableGPUBasedValidation(TRUE);
      m_pDebug->SetEnableSynchronizedCommandQueueValidation(TRUE);
    }
    else
    {
      plLog::Error("RendererDX12: DebugInterface failed to create.");
    }
  }

  /// Create the factory.
  if (createDebugDevice == true)
  {
    m_uiDxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
  }
  else
  {
    m_uiDxgiFlags = 0;
  }

  HRESULT result = CreateDXGIFactory2(m_uiDxgiFlags,IID_PPV_ARGS(&m_pDXGIFactory));
  if(FAILED(result))
  {
    plLog::Error("RendererDX12: Failed to create the DXGI Factory. something is very wrong. HRESULT CODE: {0}",result);
    return PLASMA_FAILURE;
  }

  /// Create the adapter.
  if (wantedadapterused == nullptr)
  {

    switch (CheckDXGIAdapterLevel(wantedadapterused, m_pDXGIFactory))
    {
      case 3:
      {
        plLog::Info("D3D12: Using Feature Level 12_0. this is the most basic feature level we can use.");
        m_uiFeatureLevel = D3D_FEATURE_LEVEL_12_0;
        m_pDX12Ultimate = false;
        break;
      }
      case 2:
      {
        plLog::Info("D3D12: Using Feature Level 12_1.");
        m_uiFeatureLevel = D3D_FEATURE_LEVEL_12_1;
        m_pDX12Ultimate = false;
        break;
      }
      case 1:
      {
        plLog::Info("D3D12: Using Feature Level 12_2. DX12 Ultimate is now enabled, allowing RT and more.");
        m_uiFeatureLevel = D3D_FEATURE_LEVEL_12_2;
        m_pDX12Ultimate = true;
        break;
      }
      case 0:
      {
        #if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG) || PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
          plLog::Error("D3D12: The Given Adapter does NOT Support DX12. Please Launch with DX11 instead.");
        #else
          plLog::OsMessageBox("D3D12: The Given Adapter does NOT Support DX12. Please Launch with DX11 instead(-renderer DX11).");
        #endif

        m_uiFeatureLevel = 0;
        m_pDX12Ultimate = false;
        break;
      }
      default:
      {
        #if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG) || PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
          plLog::Error("D3D12: The Given Adapter does NOT Support DX12. Please Launch with DX11 nstead.");
        #else
          plLog::OsMessageBox("D3D12: CheckDXGIAdapterLevel Returned a invalid feature level. This may mean: The Given Adapter does NOT Support DX12. Please Launch with DX11 or Vulkan instead(-renderer DX11/VULKAN).");
        #endif

        m_uiFeatureLevel = 0;
        m_pDX12Ultimate = false;
        break;
      }
    }
  }
  else
  {
    switch (CheckDXGIAdapterLevel(m_pDXGIAdapter, m_pDXGIFactory))
    {
      case 3:
      {
        plLog::Info("D3D12: Using Feature Level 12_0.");
        m_uiFeatureLevel = D3D_FEATURE_LEVEL_12_0;
        m_pDX12Ultimate = false;
        break;
      }
      case 2:
      {
        plLog::Info("D3D12: Using Feature Level 12_1.");
        m_uiFeatureLevel = D3D_FEATURE_LEVEL_12_1;
        m_pDX12Ultimate = false;
        break;
      }
      case 1:
      {
        plLog::Info("D3D12: Using Feature Level 12_2. DX12 Ultimate is now enabled.");
        m_uiFeatureLevel = D3D_FEATURE_LEVEL_12_2;
        m_pDX12Ultimate = true;
        break;
      }
      case 0:
      {
        plLog::OsMessageBox("D3D12: The Given Adapter does NOT Support DX12. Please Launch with DX11 instead.");
        m_uiFeatureLevel = 0;
        m_pDX12Ultimate = false;
        break;
      }
      default:
      {
        plLog::Error("D3D12: CheckDXGIAdapterLevel Returned a invalid feature level. This may mean: The Given Adapter does NOT Support DX12. Please Launch with DX11 instead.");
        m_uiFeatureLevel = 0;
        m_pDX12Ultimate = false;
        break;
      }
    }
  }

  /// Create Device
  HRESULT result_device = D3D12CreateDevice(m_pDXGIAdapter, (D3D_FEATURE_LEVEL)m_uiFeatureLevel, IID_PPV_ARGS(&m_pDevice3));
  if (FAILED(result_device))
  {
    plLog::Error("Failed to create the D3D12 Device. something is very wrong. HRESULT CODE: {0}", result);
    return PLASMA_FAILURE;
  }

  /// Also get the debug device.
  if (createDebugDevice)
  {
    HRESULT result_debugdevice = m_pDevice3->QueryInterface(&m_pDebugDevice);
    ID3D12InfoQueue* pInfoQueue = nullptr;

    if (FAILED(result_debugdevice) && FAILED(m_pDebug->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
    {
      plLog::Error("Failed to create the D3D12 Debug Objects. something is very wrong. HRESULT CODE: {0}", result);
      return PLASMA_FAILURE;
    }
    else
    {
      ReportLiveGpuObjects();
      pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
      pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    }
  }

  // Create default pass
  m_pDefaultPass = PLASMA_NEW(&m_Allocator, plGALPassDX12, *this);

  // Fill lookup table
  FillFormatLookupTable();
}

plResult plGALDeviceDX12::InitPlatform()
{
  return InitPlatform(D3D_FEATURE_LEVEL_12_0,nullptr);
}

void plGALDeviceDX12::ReportLiveGpuObjects()
{
  OutputDebugStringW(L" +++++ Live DX12 Objects: +++++\n");
  HRESULT result = m_pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY);

  if (FAILED(result))
  {
    plLog::SeriousWarning("RendererDX12: Failed to report Live Device Objects.");
  }

  OutputDebugStringW(L" ----- Live DX12 Objects: -----\n");

  PLASMA_GAL_DX12_RELEASE(m_pDebugDevice);
}