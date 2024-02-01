#pragma once

#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <RenderDocPlugin/RenderDocPluginDLL.h>

struct RENDERDOC_API_1_4_1;

/// \brief RenderDoc implementation of the plFrameCaptureInterface interface
///
/// Adds support for capturing frames through RenderDoc.
/// When the plugin gets loaded, an plRenderDoc instance is created and initialized.
/// It tries to find a RenderDoc DLL dynamically, so for initialization to succeed,
/// the DLL has to be available in some search directory (e.g. binary folder or PATH).
/// If an outdated RenderDoc DLL is found, initialization will fail and the plugin will be deactivated.
///
/// For interface documentation see \ref plFrameCaptureInterface
class PL_RENDERDOCPLUGIN_DLL plRenderDoc : public plFrameCaptureInterface
{
  PL_DECLARE_SINGLETON_OF_INTERFACE(plRenderDoc, plFrameCaptureInterface);

public:
  plRenderDoc();
  virtual ~plRenderDoc();

  virtual bool IsInitialized() const override;
  virtual void SetAbsCaptureFilePathTemplate(plStringView sFilePathTemplate) override;
  virtual plStringView GetAbsCaptureFilePathTemplate() const override;
  virtual void StartFrameCapture(plWindowHandle hWnd) override;
  virtual bool IsFrameCapturing() const override;
  virtual void EndFrameCaptureAndWriteOutput(plWindowHandle hWnd) override;
  virtual void EndFrameCaptureAndDiscardResult(plWindowHandle hWnd) override;
  virtual plResult GetLastAbsCaptureFileName(plStringBuilder& out_sFileName) const override;

private:
  RENDERDOC_API_1_4_1* m_pRenderDocAPI = nullptr;
  plMinWindows::HMODULE m_pHandleToFree = nullptr;
};
