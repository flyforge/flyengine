#pragma once

#include <Core/ActorSystem/ActorPlugin.h>

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/System/Window.h>

class plActor;
class plWindowOutputTargetBase;
class plWindowBase;

class PLASMA_CORE_DLL plActorPluginWindow : public plActorPlugin
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActorPluginWindow, plActorPlugin);

public:
  virtual plWindowBase* GetWindow() const = 0;
  virtual plWindowOutputTargetBase* GetOutputTarget() const = 0;

protected:
  virtual void Update() override;
};

class PLASMA_CORE_DLL plActorPluginWindowOwner : public plActorPluginWindow
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActorPluginWindowOwner, plActorPluginWindow);

public:
  virtual ~plActorPluginWindowOwner();
  virtual plWindowBase* GetWindow() const override;
  virtual plWindowOutputTargetBase* GetOutputTarget() const override;

  plUniquePtr<plWindowBase> m_pWindow;
  plUniquePtr<plWindowOutputTargetBase> m_pWindowOutputTarget;
};

class PLASMA_CORE_DLL plActorPluginWindowShared : public plActorPluginWindow
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActorPluginWindowShared, plActorPluginWindow);

public:
  virtual plWindowBase* GetWindow() const override;
  virtual plWindowOutputTargetBase* GetOutputTarget() const override;

  plWindowBase* m_pWindow = nullptr;
  plWindowOutputTargetBase* m_pWindowOutputTarget = nullptr;
};
