#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class QToolButton;
class QAction;

class PLASMA_EDITORFRAMEWORK_DLL plExposedParameterCommandAccessor : public plObjectProxyAccessor
{
public:
  plExposedParameterCommandAccessor(plObjectAccessorBase* pSource, const plAbstractProperty* pParameterProp, const plAbstractProperty* m_pParameterSourceProp);

  virtual plStatus GetValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant& out_value, plVariant index = plVariant()) override;
  virtual plStatus SetValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;
  virtual plStatus RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus GetCount(const plDocumentObject* pObject, const plAbstractProperty* pProp, plInt32& out_iCount) override;
  virtual plStatus GetKeys(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_keys) override;
  virtual plStatus GetValues(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_values) override;

public:
  const plExposedParameters* GetExposedParams(const plDocumentObject* pObject);
  const plExposedParameter* GetExposedParam(const plDocumentObject* pObject, const char* szParamName);
  const plRTTI* GetExposedParamsType(const plDocumentObject* pObject);
  const plRTTI* GetCommonExposedParamsType(const plHybridArray<plPropertySelection, 8>& items);
  bool IsExposedProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp);

public:
  const plAbstractProperty* m_pParameterProp = nullptr;
  const plAbstractProperty* m_pParameterSourceProp = nullptr;
};

class PLASMA_EDITORFRAMEWORK_DLL plQtExposedParameterPropertyWidget : public plQtVariantPropertyWidget
{
  Q_OBJECT;

protected:
  virtual void InternalSetValue(const plVariant& value);
};

class PLASMA_EDITORFRAMEWORK_DLL plQtExposedParametersPropertyWidget : public plQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT

public:
  plQtExposedParametersPropertyWidget();
  virtual ~plQtExposedParametersPropertyWidget();
  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;

protected:
  virtual void OnInit() override;
  virtual plQtPropertyWidget* CreateWidget(plUInt32 index) override;
  virtual void UpdateElement(plUInt32 index) override;
  virtual void UpdatePropertyMetaState() override;

private:
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);
  void FlushQueuedChanges();
  bool RemoveUnusedKeys(bool bTestOnly);
  bool FixKeyTypes(bool bTestOnly);
  void UpdateActionState();

private:
  plUniquePtr<plExposedParameterCommandAccessor> m_pProxy;
  plObjectAccessorBase* m_pSourceObjectAccessor = nullptr;
  plString m_sExposedParamProperty;
  mutable plDynamicArray<plExposedParameter> m_Parameters;
  bool m_bNeedsUpdate = false;
  bool m_bNeedsMetaDataUpdate = false;

  QToolButton* m_pFixMeButton = nullptr;
  QAction* m_pRemoveUnusedAction = nullptr;
  QAction* m_pFixTypesAction = nullptr;
};
