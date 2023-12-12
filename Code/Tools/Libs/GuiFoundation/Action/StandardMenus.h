#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

struct plStandardMenuTypes
{
  using StorageType = plUInt32;

  enum Enum
  {
    File = PLASMA_BIT(0),
    Edit = PLASMA_BIT(1),
    Panels = PLASMA_BIT(2),
    Project = PLASMA_BIT(3),
    Scene = PLASMA_BIT(4),
    View = PLASMA_BIT(5),
    Help = PLASMA_BIT(6),

    Default = 0
  };

  struct Bits
  {
    StorageType File : 1;
    StorageType Edit : 1;
    StorageType Panels : 1;
    StorageType Project : 1;
    StorageType Scene : 1;
    StorageType View : 1;
    StorageType Help : 1;
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plStandardMenuTypes);

///
class PLASMA_GUIFOUNDATION_DLL plStandardMenus
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const plBitflags<plStandardMenuTypes>& Menus);

  static plActionDescriptorHandle s_hMenuFile;
  static plActionDescriptorHandle s_hMenuEdit;
  static plActionDescriptorHandle s_hMenuPanels;
  static plActionDescriptorHandle s_hMenuProject;
  static plActionDescriptorHandle s_hMenuScene;
  static plActionDescriptorHandle s_hMenuView;
  static plActionDescriptorHandle s_hMenuHelp;
  static plActionDescriptorHandle s_hCheckForUpdates;
  static plActionDescriptorHandle s_hReportProblem;
};

///
class PLASMA_GUIFOUNDATION_DLL plApplicationPanelsMenuAction : public plDynamicMenuAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plApplicationPanelsMenuAction, plDynamicMenuAction);

public:
  plApplicationPanelsMenuAction(const plActionContext& context, const char* szName, const char* szIconPath)
    : plDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_Entries) override;
  virtual void Execute(const plVariant& value) override;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_GUIFOUNDATION_DLL plHelpActions : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plHelpActions, plButtonAction);

public:
  enum class ButtonType
  {
    CheckForUpdates,
    ReportProblem,
  };

  plHelpActions(const plActionContext& context, const char* szName, ButtonType button);
  ~plHelpActions();

  virtual void Execute(const plVariant& value) override;

private:
  ButtonType m_ButtonType;
};
