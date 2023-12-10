#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

class QGridLayout;
class plDocument;
class plQtManipulatorLabel;
struct plManipulatorManagerEvent;
class plObjectAccessorBase;

class PLASMA_GUIFOUNDATION_DLL plQtTypeWidget : public QWidget
{
  Q_OBJECT
public:
  plQtTypeWidget(QWidget* pParent, plQtPropertyGridWidget* pGrid, plObjectAccessorBase* pObjectAccessor, const plRTTI* pType,
    const char* szIncludeProperties, const char* szExcludeProperties);
  ~plQtTypeWidget();
  void SetSelection(const plHybridArray<plPropertySelection, 8>& items);
  const plHybridArray<plPropertySelection, 8>& GetSelection() const { return m_Items; }
  const plRTTI* GetType() const { return m_pType; }
  void PrepareToDie();

private:
  struct PropertyGroup
  {
    PropertyGroup(const plGroupAttribute* pAttr, float& ref_fOrder)
    {
      if (pAttr)
      {
        m_sGroup = pAttr->GetGroup();
        m_sIconName = pAttr->GetIconName();
        m_fOrder = pAttr->GetOrder();
        if (m_fOrder == -1.0f)
        {
          ref_fOrder += 1.0f;
          m_fOrder = ref_fOrder;
        }
      }
      else
      {
        ref_fOrder += 1.0f;
        m_fOrder = ref_fOrder;
      }
    }

    void MergeGroup(const plGroupAttribute* pAttr)
    {
      if (pAttr)
      {
        m_sGroup = pAttr->GetGroup();
        m_sIconName = pAttr->GetIconName();
        if (pAttr->GetOrder() != -1.0f)
        {
          m_fOrder = pAttr->GetOrder();
        }
      }
    }

    bool operator==(const PropertyGroup& rhs) { return m_sGroup == rhs.m_sGroup; }
    bool operator<(const PropertyGroup& rhs) { return m_fOrder < rhs.m_fOrder; }

    plString m_sGroup;
    plString m_sIconName;
    float m_fOrder = -1.0f;
    plHybridArray<const plAbstractProperty*, 8> m_Properties;
  };

  void BuildUI(const plRTTI* pType, const char* szIncludeProperties, const char* szExcludeProperties);
  void BuildUI(const plRTTI* pType, const plMap<plString, const plManipulatorAttribute*>& manipulatorMap,
    const plDynamicArray<plUniquePtr<PropertyGroup>>& groups, const char* szIncludeProperties, const char* szExcludeProperties);

  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);
  void ManipulatorManagerEventHandler(const plManipulatorManagerEvent& e);

  void UpdateProperty(const plDocumentObject* pObject, const plString& sProperty);
  void FlushQueuedChanges();
  void UpdatePropertyMetaState();

protected:
  virtual void showEvent(QShowEvent* event) override;

private:
  bool m_bUndead = false;
  plQtPropertyGridWidget* m_pGrid = nullptr;
  plObjectAccessorBase* m_pObjectAccessor = nullptr;
  const plRTTI* m_pType = nullptr;
  plHybridArray<plPropertySelection, 8> m_Items;

  struct PropertyWidgetData
  {
    plQtPropertyWidget* m_pWidget;
    plQtManipulatorLabel* m_pLabel;
    plString m_sOriginalLabelText;
  };

  QGridLayout* m_pLayout;
  plMap<plString, PropertyWidgetData> m_PropertyWidgets;
  plHybridArray<plString, 1> m_QueuedChanges;
  QPalette m_Pal;
};
