#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyEventHandler.h>
#include <QWidget>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class plDocumentObject;
class plQtTypeWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QMenu;
class QComboBox;
class plQtGroupBoxBase;
class plQtAddSubElementButton;
class plQtPropertyGridWidget;
class plQtElementGroupButton;
class QMimeData;
struct plCommandHistoryEvent;
class plObjectAccessorBase;

/// \brief Base class for all property widgets
class PLASMA_GUIFOUNDATION_DLL plQtPropertyWidget : public QWidget
{
  Q_OBJECT;

public:
  explicit plQtPropertyWidget();
  virtual ~plQtPropertyWidget();

  void Init(plQtPropertyGridWidget* pGrid, plObjectAccessorBase* pObjectAccessor, const plRTTI* pType, const plAbstractProperty* pProp);
  const plAbstractProperty* GetProperty() const { return m_pProp; }

  /// \brief This is called whenever the selection in the editor changes and thus the widget may need to display a different value.
  ///
  /// If the array holds more than one element, the user selected multiple objects. In this case, the code should check whether
  /// the values differ across the selected objects and if so, the widget should display "multiple values".
  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items);
  const plHybridArray<plPropertySelection, 8>& GetSelection() const { return m_Items; }

  /// \brief If this returns true (default), a QLabel is created and the text that GetLabel() returns is displayed.
  virtual bool HasLabel() const { return true; }

  /// \brief The return value is used to display a label, if HasLabel() returns true.
  virtual const char* GetLabel(plStringBuilder& ref_sTmp) const;

  virtual void ExtendContextMenu(QMenu& ref_menu);

  /// \brief Whether the variable that the widget represents is currently set to the default value or has been modified.
  virtual void SetIsDefault(bool bIsDefault) { m_bIsDefault = bIsDefault; }

  /// \brief If the property is of type plVariant this function returns whether all items have the same type.
  /// If true is returned, out_Type contains the common type. Note that 'invalid' can be a common type.
  bool GetCommonVariantSubType(
    const plHybridArray<plPropertySelection, 8>& items, const plAbstractProperty* pProperty, plVariantType::Enum& out_type);

  plVariant GetCommonValue(const plHybridArray<plPropertySelection, 8>& items, const plAbstractProperty* pProperty);
  void PrepareToDie();

public:
  static const plRTTI* GetCommonBaseType(const plHybridArray<plPropertySelection, 8>& items);
  static QColor SetPaletteBackgroundColor(plColorGammaUB inputColor, QPalette& ref_palette);

public Q_SLOTS:
  void OnCustomContextMenu(const QPoint& pt);

protected:
  void Broadcast(plPropertyEvent::Type type);
  void PropertyChangedHandler(const plPropertyEvent& ed);

  virtual void OnInit() = 0;
  bool IsUndead() const { return m_bUndead; }

protected:
  virtual void DoPrepareToDie() = 0;

  virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

  plQtPropertyGridWidget* m_pGrid = nullptr;
  plObjectAccessorBase* m_pObjectAccessor = nullptr;
  const plRTTI* m_pType = nullptr;
  const plAbstractProperty* m_pProp = nullptr;
  plHybridArray<plPropertySelection, 8> m_Items;
  bool m_bIsDefault; ///< Whether the variable that the widget represents is currently set to the default value or has been modified.

private:
  bool m_bUndead; ///< Widget is being destroyed
};


/// \brief Fallback widget for all property types for which no other widget type is registered
class PLASMA_GUIFOUNDATION_DLL plQtUnsupportedPropertyWidget : public plQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit plQtUnsupportedPropertyWidget(const char* szMessage = nullptr);

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override {}

  QHBoxLayout* m_pLayout;
  QLabel* m_pWidget;
  plString m_sMessage;
};


/// \brief Base class for most 'simple' property type widgets. Implements some of the standard functionality.
class PLASMA_GUIFOUNDATION_DLL plQtStandardPropertyWidget : public plQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit plQtStandardPropertyWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;

protected:
  void BroadcastValueChanged(const plVariant& NewValue);
  virtual void DoPrepareToDie() override {}

  const plVariant& GetOldValue() const { return m_OldValue; }
  virtual void InternalSetValue(const plVariant& value) = 0;

protected:
  plVariant m_OldValue;
};


/// \brief Base class for more 'advanced' property type widgets for Pointer or Class type properties.
/// Implements some of plQtTypeWidget functionality at property widget level.
class PLASMA_GUIFOUNDATION_DLL plQtEmbeddedClassPropertyWidget : public plQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit plQtEmbeddedClassPropertyWidget();
  ~plQtEmbeddedClassPropertyWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;

protected:
  void SetPropertyValue(const plAbstractProperty* pProperty, const plVariant& NewValue);

  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  virtual void OnPropertyChanged(const plString& sProperty) = 0;

private:
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);
  void FlushQueuedChanges();

protected:
  bool m_bTemporaryCommand = false;
  const plRTTI* m_pResolvedType = nullptr;
  plHybridArray<plPropertySelection, 8> m_ResolvedObjects;

  plHybridArray<plString, 1> m_QueuedChanges;
};


/// Used for pointers and embedded classes.
/// Does not inherit from plQtEmbeddedClassPropertyWidget as it just embeds
/// a plQtTypeWidget for the property's value which handles everything already.
class PLASMA_GUIFOUNDATION_DLL plQtPropertyTypeWidget : public plQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit plQtPropertyTypeWidget(bool bAddCollapsibleGroup = false);
  virtual ~plQtPropertyTypeWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }
  virtual void SetIsDefault(bool bIsDefault) override;

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;

protected:
  QHBoxLayout* m_pLayout;
  plQtGroupBoxBase* m_pGroup;
  QHBoxLayout* m_pGroupLayout;
  plQtTypeWidget* m_pTypeWidget;
};

/// \brief Used for property types that are pointers.
class PLASMA_GUIFOUNDATION_DLL plQtPropertyPointerWidget : public plQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit plQtPropertyPointerWidget();
  virtual ~plQtPropertyPointerWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }


public Q_SLOTS:
  void OnDeleteButtonClicked();

protected:
  virtual void OnInit() override;
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  virtual void DoPrepareToDie() override;
  void UpdateTitle(const plRTTI* pType = nullptr);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  plQtGroupBoxBase* m_pGroup = nullptr;
  plQtAddSubElementButton* m_pAddButton = nullptr;
  plQtElementGroupButton* m_pDeleteButton = nullptr;
  QHBoxLayout* m_pGroupLayout = nullptr;
  plQtTypeWidget* m_pTypeWidget = nullptr;
};


/// \brief Base class for all container properties
class PLASMA_GUIFOUNDATION_DLL plQtPropertyContainerWidget : public plQtPropertyWidget
{
  Q_OBJECT;

public:
  plQtPropertyContainerWidget();
  virtual ~plQtPropertyContainerWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;
  virtual bool HasLabel() const override { return false; }
  virtual void SetIsDefault(bool bIsDefault) override;

public Q_SLOTS:
  void OnElementButtonClicked();
  void OnDragStarted(QMimeData& ref_mimeData);
  void OnContainerContextMenu(const QPoint& pt);
  void OnCustomElementContextMenu(const QPoint& pt);

protected:
  struct Element
  {
    Element() = default;

    Element(plQtGroupBoxBase* pSubGroup, plQtPropertyWidget* pWidget, plQtElementGroupButton* pHelpButton)
      : m_pSubGroup(pSubGroup)
      , m_pWidget(pWidget)
      , m_pHelpButton(pHelpButton)
    {
    }

    plQtGroupBoxBase* m_pSubGroup = nullptr;
    plQtPropertyWidget* m_pWidget = nullptr;
    plQtElementGroupButton* m_pHelpButton = nullptr;
  };

  virtual plQtGroupBoxBase* CreateElement(QWidget* pParent);
  virtual plQtPropertyWidget* CreateWidget(plUInt32 index);
  virtual Element& AddElement(plUInt32 index);
  virtual void RemoveElement(plUInt32 index);
  virtual void UpdateElement(plUInt32 index) = 0;
  void UpdateElements();
  virtual plUInt32 GetRequiredElementCount() const;
  virtual void UpdatePropertyMetaState();

  void Clear();
  virtual void OnInit() override;

  void DeleteItems(plHybridArray<plPropertySelection, 8>& items);
  void MoveItems(plHybridArray<plPropertySelection, 8>& items, plInt32 iMove);
  virtual void DoPrepareToDie() override;
  virtual void dragEnterEvent(QDragEnterEvent* event) override;
  virtual void dragMoveEvent(QDragMoveEvent* event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* event) override;
  virtual void dropEvent(QDropEvent* event) override;
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void showEvent(QShowEvent* event) override;

private:
  bool updateDropIndex(QDropEvent* pEvent);

protected:
  QHBoxLayout* m_pLayout;
  plQtGroupBoxBase* m_pGroup;
  QVBoxLayout* m_pGroupLayout;
  plQtAddSubElementButton* m_pAddButton = nullptr;
  QPalette m_Pal;

  mutable plHybridArray<plVariant, 16> m_Keys;
  plDynamicArray<Element> m_Elements;
  plInt32 m_iDropSource = -1;
  plInt32 m_iDropTarget = -1;
};


class PLASMA_GUIFOUNDATION_DLL plQtPropertyStandardTypeContainerWidget : public plQtPropertyContainerWidget
{
  Q_OBJECT;

public:
  plQtPropertyStandardTypeContainerWidget();
  virtual ~plQtPropertyStandardTypeContainerWidget();

protected:
  virtual plQtGroupBoxBase* CreateElement(QWidget* pParent) override;
  virtual plQtPropertyWidget* CreateWidget(plUInt32 index) override;
  virtual Element& AddElement(plUInt32 index) override;
  virtual void RemoveElement(plUInt32 index) override;
  virtual void UpdateElement(plUInt32 index) override;
};

class PLASMA_GUIFOUNDATION_DLL plQtPropertyTypeContainerWidget : public plQtPropertyContainerWidget
{
  Q_OBJECT;

public:
  plQtPropertyTypeContainerWidget();
  virtual ~plQtPropertyTypeContainerWidget();

protected:
  virtual void OnInit() override;
  virtual void UpdateElement(plUInt32 index) override;

  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);

private:
  bool m_bNeedsUpdate = false;
};

class PLASMA_GUIFOUNDATION_DLL plQtVariantPropertyWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  plQtVariantPropertyWidget();
  virtual ~plQtVariantPropertyWidget();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;
  virtual void DoPrepareToDie() override;
  void UpdateTypeListSelection(plVariantType::Enum type);
  void ChangeVariantType(plVariantType::Enum type);

  virtual plResult GetVariantTypeDisplayName(plVariantType::Enum type, plStringBuilder& out_sName) const;

protected:
  QVBoxLayout* m_pLayout = nullptr;
  QComboBox* m_pTypeList = nullptr;
  plQtPropertyWidget* m_pWidget = nullptr;
  const plRTTI* m_pCurrentSubType = nullptr;
};
