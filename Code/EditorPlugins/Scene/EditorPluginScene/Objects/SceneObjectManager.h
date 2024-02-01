#pragma once

#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plDocument;
class plScene2Document;

class plSceneDocumentSettingsBase : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneDocumentSettingsBase, plReflectedClass);
};

class plPrefabDocumentSettings : public plSceneDocumentSettingsBase
{
  PL_ADD_DYNAMIC_REFLECTION(plPrefabDocumentSettings, plSceneDocumentSettingsBase);

public:
  plDynamicArray<plExposedSceneProperty> m_ExposedProperties;
};

class plLayerDocumentSettings : public plSceneDocumentSettingsBase
{
  PL_ADD_DYNAMIC_REFLECTION(plLayerDocumentSettings, plSceneDocumentSettingsBase);
};

class plSceneDocumentRoot : public plDocumentRoot
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneDocumentRoot, plDocumentRoot);

public:
  plSceneDocumentSettingsBase* m_pSettings;
};

class plSceneObjectManager : public plDocumentObjectManager
{
public:
  plSceneObjectManager();
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const override;

private:
  virtual plStatus InternalCanAdd(
    const plRTTI* pRtti, const plDocumentObject* pParent, plStringView sParentProperty, const plVariant& index) const override;
  virtual plStatus InternalCanSelect(const plDocumentObject* pObject) const override;
  virtual plStatus InternalCanMove(
    const plDocumentObject* pObject, const plDocumentObject* pNewParent, plStringView sParentProperty, const plVariant& index) const override;
};
