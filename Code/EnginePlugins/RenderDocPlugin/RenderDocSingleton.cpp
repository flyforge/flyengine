#include <RenderDocPlugin/RenderDocPluginPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RenderDocPlugin/RenderDocSingleton.h>
#include <RenderDocPlugin/ThirdParty/renderdoc_app.h>

PL_IMPLEMENT_SINGLETON(plRenderDoc);

static plCommandLineOptionBool opt_NoCaptures("RenderDoc", "-NoCaptures", "Disables RenderDoc capture support.", false);

static plRenderDoc g_RenderDocSingleton;

plRenderDoc::plRenderDoc()
  : m_SingletonRegistrar(this)
{
  if (opt_NoCaptures.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    plLog::Info("RenderDoc plugin: Initialization suppressed via command-line.");
    return;
  }

  HMODULE dllHandle = GetModuleHandleW(L"renderdoc.dll");
  if (!dllHandle)
  {
    dllHandle = LoadLibraryW(L"renderdoc.dll");
    m_pHandleToFree = plMinWindows::FromNative(dllHandle);
  }

  if (!dllHandle)
  {
    plLog::Info("RenderDoc plugin: 'renderdoc.dll' could not be found. Frame captures aren't possible.");
    return;
  }

  if (pRENDERDOC_GetAPI RenderDoc_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(dllHandle, "RENDERDOC_GetAPI"))
  {
    void* pApi = nullptr;
    RenderDoc_GetAPI(eRENDERDOC_API_Version_1_4_0, &pApi);
    m_pRenderDocAPI = (RENDERDOC_API_1_4_1*)pApi;
  }

  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->SetCaptureKeys(nullptr, 0);
    m_pRenderDocAPI->SetFocusToggleKeys(nullptr, 0);
    m_pRenderDocAPI->MaskOverlayBits(0, 0);
  }
  else
  {
    plLog::Warning("RenderDoc plugin: Unable to retrieve API pointer from DLL. Potentially outdated version. Frame captures aren't possible.");
  }
}

plRenderDoc::~plRenderDoc()
{
  m_pRenderDocAPI = nullptr;

  if (m_pHandleToFree)
  {
    FreeLibrary(plMinWindows::ToNative(m_pHandleToFree));
    m_pHandleToFree = nullptr;
  }
}

bool plRenderDoc::IsInitialized() const
{
  return m_pRenderDocAPI != nullptr;
}

void plRenderDoc::SetAbsCaptureFilePathTemplate(plStringView sFilePathTemplate)
{
  if (m_pRenderDocAPI)
  {
    plStringBuilder tmp;
    m_pRenderDocAPI->SetCaptureFilePathTemplate(sFilePathTemplate.GetData(tmp));
  }
}

plStringView plRenderDoc::GetAbsCaptureFilePathTemplate() const
{
  if (m_pRenderDocAPI)
  {
    return m_pRenderDocAPI->GetCaptureFilePathTemplate();
  }

  return {};
}

void plRenderDoc::StartFrameCapture(plWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->StartFrameCapture(nullptr, hWnd);
  }
}

bool plRenderDoc::IsFrameCapturing() const
{
  return m_pRenderDocAPI ? m_pRenderDocAPI->IsFrameCapturing() : false;
}

void plRenderDoc::EndFrameCaptureAndWriteOutput(plWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->EndFrameCapture(nullptr, hWnd);
  }
}

void plRenderDoc::EndFrameCaptureAndDiscardResult(plWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->DiscardFrameCapture(nullptr, hWnd);
  }
}

plResult plRenderDoc::GetLastAbsCaptureFileName(plStringBuilder& out_sFileName) const
{
  if (m_pRenderDocAPI && m_pRenderDocAPI->GetNumCaptures() > 0)
  {
    plUInt32 uiNumCaptures = m_pRenderDocAPI->GetNumCaptures();
    plUInt32 uiFilePathLength = 0;
    if (m_pRenderDocAPI->GetCapture(uiNumCaptures - 1, nullptr, &uiFilePathLength, nullptr))
    {
      plHybridArray<char, 128> filePathBuffer;
      filePathBuffer.SetCount(uiFilePathLength);
      m_pRenderDocAPI->GetCapture(uiNumCaptures - 1, filePathBuffer.GetArrayPtr().GetPtr(), nullptr, nullptr);
      out_sFileName = filePathBuffer.GetArrayPtr().GetPtr();

      return PL_SUCCESS;
    }
  }

  return PL_FAILURE;
}

PL_STATICLINK_FILE(RenderDocPlugin, RenderDocPlugin_RenderDocSingleton);
