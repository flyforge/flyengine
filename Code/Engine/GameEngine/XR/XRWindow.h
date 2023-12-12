#pragma once

#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class plXRInterface;

/// \brief XR Window base implementation. Optionally wraps a companion window.
class PLASMA_GAMEENGINE_DLL plWindowXR : public plWindowBase
{
public:
  plWindowXR(plXRInterface* pVrInterface, plUniquePtr<plWindowBase> pCompanionWindow);
  virtual ~plWindowXR();

  virtual plSizeU32 GetClientAreaSize() const override;
  virtual plSizeU32 GetRenderAreaSize() const override;

  virtual plWindowHandle GetNativeWindowHandle() const override;

  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode) const override;

  virtual void ProcessWindowMessages() override;

  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }

  /// \brief Returns the companion window if present.
  const plWindowBase* GetCompanionWindow() const;

private:
  plXRInterface* m_pVrInterface = nullptr;
  plUniquePtr<plWindowBase> m_pCompanionWindow;
  plAtomicInteger32 m_iReferenceCount = 0;
};

/// \brief XR Window output target base implementation. Optionally wraps a companion window output target.
class PLASMA_GAMEENGINE_DLL plWindowOutputTargetXR : public plWindowOutputTargetBase
{
public:
  plWindowOutputTargetXR(plXRInterface* pVrInterface, plUniquePtr<plWindowOutputTargetGAL> pCompanionWindowOutputTarget);
  ~plWindowOutputTargetXR();

  virtual void Present(bool bEnableVSync) override;
  void RenderCompanionView(bool bThrottleCompanionView = true);
  virtual plResult CaptureImage(plImage& out_Image) override;

  /// \brief Returns the companion window output target if present.
  const plWindowOutputTargetBase* GetCompanionWindowOutputTarget() const;

private:
  plXRInterface* m_pXrInterface = nullptr;
  plTime m_LastPresent;
  plUniquePtr<plWindowOutputTargetGAL> m_pCompanionWindowOutputTarget;
  plConstantBufferStorageHandle m_hCompanionConstantBuffer;
  plShaderResourceHandle m_hCompanionShader;
};

/// \brief XR actor plugin window base implementation. Optionally wraps a companion window and output target.
class PLASMA_GAMEENGINE_DLL plActorPluginWindowXR : public plActorPluginWindow
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActorPluginWindowXR, plActorPluginWindow);

public:
  plActorPluginWindowXR(plXRInterface* pVrInterface, plUniquePtr<plWindowBase> companionWindow, plUniquePtr<plWindowOutputTargetGAL> companionWindowOutput);
  ~plActorPluginWindowXR();
  void Initialize();

  virtual plWindowBase* GetWindow() const override;
  virtual plWindowOutputTargetBase* GetOutputTarget() const override;

protected:
  virtual void Update() override;

private:
  plXRInterface* m_pVrInterface = nullptr;
  plUniquePtr<plWindowXR> m_pWindow;
  plUniquePtr<plWindowOutputTargetXR> m_pWindowOutputTarget;
};
