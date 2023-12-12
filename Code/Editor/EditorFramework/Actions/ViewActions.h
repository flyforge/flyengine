#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class PLASMA_EDITORFRAMEWORK_DLL plViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  enum Flags
  {
    PerspectiveMode = PLASMA_BIT(0),
    RenderMode = PLASMA_BIT(1),
    ActivateRemoteProcess = PLASMA_BIT(2),
  };

  static void MapActions(const char* szMapping, const char* szPath, plUInt32 flags);

  static plActionDescriptorHandle s_hRenderMode;
  static plActionDescriptorHandle s_hPerspective;
  static plActionDescriptorHandle s_hActivateRemoteProcess;
  static plActionDescriptorHandle s_hLinkDeviceCamera;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plRenderModeAction : public plEnumerationMenuAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRenderModeAction, plEnumerationMenuAction);

public:
  plRenderModeAction(const plActionContext& context, const char* szName, const char* szIconPath);
  virtual plInt64 GetValue() const override;
  virtual void Execute(const plVariant& value) override;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plPerspectiveAction : public plEnumerationMenuAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPerspectiveAction, plEnumerationMenuAction);

public:
  plPerspectiveAction(const plActionContext& context, const char* szName, const char* szIconPath);
  virtual plInt64 GetValue() const override;
  virtual void Execute(const plVariant& value) override;
};

class PLASMA_EDITORFRAMEWORK_DLL plViewAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewAction, plButtonAction);

public:
  enum class ButtonType
  {
    ActivateRemoteProcess,
    LinkDeviceCamera,
  };

  plViewAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plViewAction();

  virtual void Execute(const plVariant& value) override;

private:
  ButtonType m_ButtonType;
};
