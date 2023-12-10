#pragma once

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Size.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Creates a swapchain and keeps it up to date with the window.
///
/// If the window is resized or plGameApplication::cvar_AppVSync changes and onSwapChainChanged is valid, the swapchain is destroyed and recreated. It is up the the application to respond to the OnSwapChainChanged callback and update any references to the swap-chain, e.g. uses in plView or uses as render targets in plGALRenderTargetSetup.
/// If onSwapChainChanged is not set, the swapchain will not be re-created and it is up to the application to manage the swapchain and react to window changes.
class PLASMA_GAMEENGINE_DLL plWindowOutputTargetGAL : public plWindowOutputTargetBase
{
public:
  using OnSwapChainChanged = plDelegate<void(plGALSwapChainHandle hSwapChain, plSizeU32 size)>;
  plWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged = {});
  ~plWindowOutputTargetGAL();

  void CreateSwapchain(const plGALWindowSwapChainCreationDescription& desc);

  virtual void Present(bool bEnableVSync) override;
  virtual plResult CaptureImage(plImage& out_Image) override;

  OnSwapChainChanged m_OnSwapChainChanged;
  plSizeU32 m_Size = plSizeU32(0, 0);
  plGALWindowSwapChainCreationDescription m_currentDesc;
  plGALSwapChainHandle m_hSwapChain;
};
