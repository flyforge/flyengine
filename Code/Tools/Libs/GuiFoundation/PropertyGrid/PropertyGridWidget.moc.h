#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/TypeWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

class QSpacerItem;
class QVBoxLayout;
class QScrollArea;

class plQtGroupBoxBase;
class plDocument;
class plDocumentObjectManager;
class plCommandHistory;
class plObjectAccessorBase;
struct plDocumentObjectPropertyEvent;
struct plPropertyMetaStateEvent;
struct plObjectAccessorChangeEvent;
struct plPropertyDefaultEvent;
struct plContainerElementMetaStateEvent;

class PLASMA_GUIFOUNDATION_DLL plQtPropertyGridWidget : public QWidget
{
  Q_OBJECT
public:
  plQtPropertyGridWidget(QWidget* pParent, plDocument* pDocument = nullptr, bool bBindToSelectionManager = true);
  ~plQtPropertyGridWidget();

  void SetDocument(plDocument* pDocument, bool bBindToSelectionManager = true);

  void ClearSelection();
  void SetSelectionIncludeExcludeProperties(const char* szIncludeProperties = nullptr, const char* szExcludeProperties = nullptr);
  void SetSelection(const plDeque<const plDocumentObject*>& selection);
  const plDocument* GetDocument() const;
  const plDocumentObjectManager* GetObjectManager() const;
  plCommandHistory* GetCommandHistory() const;
  plObjectAccessorBase* GetObjectAccessor() const;

  static plRttiMappedObjectFactory<plQtPropertyWidget>& GetFactory();
  static plQtPropertyWidget* CreateMemberPropertyWidget(const plAbstractProperty* pProp);
  static plQtPropertyWidget* CreatePropertyWidget(const plAbstractProperty* pProp);

  void SetCollapseState(plQtGroupBoxBase* pBox);

Q_SIGNALS:
  void ExtendContextMenu(QMenu& menu, const plHybridArray<plPropertySelection, 8>& items, const plAbstractProperty* pProp);

public Q_SLOTS:
  void OnCollapseStateChanged(bool bCollapsed);

private:
  static plRttiMappedObjectFactory<plQtPropertyWidget> s_Factory;
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, PropertyGrid);

private:
  void ObjectAccessorChangeEventHandler(const plObjectAccessorChangeEvent& e);
  void SelectionEventHandler(const plSelectionManagerEvent& e);
  void FactoryEventHandler(const plRttiMappedObjectFactory<plQtPropertyWidget>::Event& e);
  void TypeEventHandler(const plPhantomRttiManagerEvent& e);
  plUInt32 GetGroupBoxHash(plQtGroupBoxBase* pBox) const;

private:
  plDocument* m_pDocument;
  bool m_bBindToSelectionManager = false;
  plDeque<const plDocumentObject*> m_Selection;
  plMap<plUInt32, bool> m_CollapseState;
  plString m_sSelectionIncludeProperties;
  plString m_sSelectionExcludeProperties;

  QVBoxLayout* m_pLayout;
  QScrollArea* m_pScroll;
  QWidget* m_pContent;
  QVBoxLayout* m_pContentLayout;

  plQtTypeWidget* m_pTypeWidget;
  QSpacerItem* m_pSpacer;
};

