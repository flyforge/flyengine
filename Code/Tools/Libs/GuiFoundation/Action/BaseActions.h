#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QIcon>

///
class PL_GUIFOUNDATION_DLL plNamedAction : public plAction
{
  PL_ADD_DYNAMIC_REFLECTION(plNamedAction, plAction);

public:
  plNamedAction(const plActionContext& context, const char* szName, const char* szIconPath)
    : plAction(context)
    , m_sName(szName)
    , m_sIconPath(szIconPath)
  {
  }

  const char* GetName() const { return m_sName; }

  plStringView GetAdditionalDisplayString() { return m_sAdditionalDisplayString; }
  void SetAdditionalDisplayString(plStringView sString, bool bTriggerUpdate = true)
  {
    m_sAdditionalDisplayString = sString;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  const char* GetIconPath() const { return m_sIconPath; }
  void SetIconPath(const char* szIconPath) { m_sIconPath = szIconPath; }

protected:
  plString m_sName;
  plString m_sAdditionalDisplayString; // to add some context to the current action
  plString m_sIconPath;
};

///
class PL_GUIFOUNDATION_DLL plCategoryAction : public plAction
{
  PL_ADD_DYNAMIC_REFLECTION(plCategoryAction, plAction);

public:
  plCategoryAction(const plActionContext& context)
    : plAction(context)
  {
  }

  virtual void Execute(const plVariant& value) override{};
};

///
class PL_GUIFOUNDATION_DLL plMenuAction : public plNamedAction
{
  PL_ADD_DYNAMIC_REFLECTION(plMenuAction, plNamedAction);

public:
  plMenuAction(const plActionContext& context, const char* szName, const char* szIconPath)
    : plNamedAction(context, szName, szIconPath)
  {
  }

  virtual void Execute(const plVariant& value) override{};
};

///
class PL_GUIFOUNDATION_DLL plDynamicMenuAction : public plMenuAction
{
  PL_ADD_DYNAMIC_REFLECTION(plDynamicMenuAction, plMenuAction);

public:
  struct Item
  {
    enum class CheckMark
    {
      NotCheckable,
      Unchecked,
      Checked
    };

    struct ItemFlags
    {
      using StorageType = plUInt8;

      enum Enum
      {
        Default = 0,
        Separator = PL_BIT(0),
      };
      struct Bits
      {
        StorageType Separator : 1;
      };
    };

    Item() { m_CheckState = CheckMark::NotCheckable; }

    plString m_sDisplay;
    QIcon m_Icon;
    CheckMark m_CheckState;
    plBitflags<ItemFlags> m_ItemFlags;
    plVariant m_UserValue;
  };

  plDynamicMenuAction(const plActionContext& context, const char* szName, const char* szIconPath)
    : plMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(plHybridArray<Item, 16>& out_entries) = 0;
};

///
class PL_GUIFOUNDATION_DLL plDynamicActionAndMenuAction : public plDynamicMenuAction
{
  PL_ADD_DYNAMIC_REFLECTION(plDynamicActionAndMenuAction, plDynamicMenuAction);

public:
  plDynamicActionAndMenuAction(const plActionContext& context, const char* szName, const char* szIconPath);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

protected:
  bool m_bEnabled;
  bool m_bVisible;
};

///
class PL_GUIFOUNDATION_DLL plEnumerationMenuAction : public plDynamicMenuAction
{
  PL_ADD_DYNAMIC_REFLECTION(plEnumerationMenuAction, plDynamicMenuAction);

public:
  plEnumerationMenuAction(const plActionContext& context, const char* szName, const char* szIconPath);
  void InitEnumerationType(const plRTTI* pEnumerationType);
  virtual void GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_entries) override;
  virtual plInt64 GetValue() const = 0;

protected:
  const plRTTI* m_pEnumerationType;
};

///
class PL_GUIFOUNDATION_DLL plButtonAction : public plNamedAction
{
  PL_ADD_DYNAMIC_REFLECTION(plButtonAction, plNamedAction);

public:
  plButtonAction(const plActionContext& context, const char* szName, bool bCheckable, const char* szIconPath);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsCheckable() const { return m_bCheckable; }
  void SetCheckable(bool bCheckable, bool bTriggerUpdate = true)
  {
    m_bCheckable = bCheckable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsChecked() const { return m_bChecked; }
  void SetChecked(bool bChecked, bool bTriggerUpdate = true)
  {
    m_bChecked = bChecked;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

protected:
  bool m_bCheckable;
  bool m_bChecked;
  bool m_bEnabled;
  bool m_bVisible;
};


class PL_GUIFOUNDATION_DLL plSliderAction : public plNamedAction
{
  PL_ADD_DYNAMIC_REFLECTION(plSliderAction, plNamedAction);

public:
  plSliderAction(const plActionContext& context, const char* szName);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  void GetRange(plInt32& out_iMin, plInt32& out_iMax) const
  {
    out_iMin = m_iMinValue;
    out_iMax = m_iMaxValue;
  }

  void SetRange(plInt32 iMin, plInt32 iMax, bool bTriggerUpdate = true);

  plInt32 GetValue() const { return m_iCurValue; }
  void SetValue(plInt32 iVal, bool bTriggerUpdate = true);

protected:
  bool m_bEnabled;
  bool m_bVisible;
  plInt32 m_iMinValue;
  plInt32 m_iMaxValue;
  plInt32 m_iCurValue;
};
