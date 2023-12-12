#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class plManipulatorAttribute;
class plDocumentObject;
struct plDocumentObjectPropertyEvent;
struct plQtDocumentWindowEvent;
class plObjectAccessorBase;
class plGridSettingsMsgToEngine;

class PLASMA_EDITORFRAMEWORK_DLL plManipulatorAdapter
{
public:
  plManipulatorAdapter();
  virtual ~plManipulatorAdapter();

  void SetManipulator(const plManipulatorAttribute* pAttribute, const plDocumentObject* pObject);

  virtual void QueryGridSettings(plGridSettingsMsgToEngine& outGridSettings) {}

private:
  void DocumentObjectPropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const plQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plDocumentObjectMetaData>::EventData& e);

protected:
  virtual plTransform GetOffsetTransform() const;
  virtual plTransform GetObjectTransform() const;
  plObjectAccessorBase* GetObjectAccessor() const;
  const plAbstractProperty* GetProperty(const char* szProperty) const;

  virtual void Finalize() = 0;
  virtual void Update() = 0;
  virtual void UpdateGizmoTransform() = 0;

  void BeginTemporaryInteraction();
  void EndTemporaryInteraction();
  void CancelTemporayInteraction();
  void ChangeProperties(const char* szProperty1, plVariant value1, const char* szProperty2 = nullptr, plVariant value2 = plVariant(),
    const char* szProperty3 = nullptr, plVariant value3 = plVariant(), const char* szProperty4 = nullptr, plVariant value4 = plVariant(),
    const char* szProperty5 = nullptr, plVariant value5 = plVariant(), const char* szProperty6 = nullptr, plVariant value6 = plVariant());

  bool m_bManipulatorIsVisible;
  const plManipulatorAttribute* m_pManipulatorAttr;
  const plDocumentObject* m_pObject;

  void ClampProperty(const char* szProperty, plVariant& value) const;
};
