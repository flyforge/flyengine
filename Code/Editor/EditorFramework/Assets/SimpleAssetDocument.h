#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

template <typename ObjectProperties>
class plSimpleDocumentObjectManager : public plDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const override { ref_types.PushBack(plGetStaticRTTI<ObjectProperties>()); }
};

template <typename PropertyType, typename BaseClass = plAssetDocument>
class plSimpleAssetDocument : public BaseClass
{
public:
  plSimpleAssetDocument(plStringView sDocumentPath, plAssetDocEngineConnection engineConnectionType, bool bEnableDefaultLighting = false)
    : BaseClass(sDocumentPath, PL_DEFAULT_NEW(plSimpleDocumentObjectManager<PropertyType>), engineConnectionType)
    , m_LightSettings(bEnableDefaultLighting)
  {
    if (bEnableDefaultLighting)
    {
      plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
      pPreferences->ApplyDefaultValues(m_LightSettings);
    }
  }

  plSimpleAssetDocument(plDocumentObjectManager* pObjectManager, plStringView sDocumentPath, plAssetDocEngineConnection engineConnectionType, bool bEnableDefaultLighting = false)
    : BaseClass(sDocumentPath, pObjectManager, engineConnectionType)
    , m_LightSettings(bEnableDefaultLighting)
  {
    if (bEnableDefaultLighting)
    {
      plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
      pPreferences->ApplyDefaultValues(m_LightSettings);
    }
  }

  ~plSimpleAssetDocument()
  {
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();
  }

  const PropertyType* GetProperties() const
  {
    return static_cast<const PropertyType*>(m_ObjectMirror.GetNativeObjectPointer(this->GetObjectManager()->GetRootObject()->GetChildren()[0]));
  }

  PropertyType* GetProperties()
  {
    return static_cast<PropertyType*>(m_ObjectMirror.GetNativeObjectPointer(this->GetObjectManager()->GetRootObject()->GetChildren()[0]));
  }

  plDocumentObject* GetPropertyObject() { return this->GetObjectManager()->GetRootObject()->GetChildren()[0]; }

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override
  {
    EnsureSettingsObjectExist();

    m_ObjectMirror.InitSender(this->GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();

    BaseClass::InitializeAfterLoading(bFirstTimeCreation);

    this->AddSyncObject(&m_LightSettings);
  }

  virtual plStatus InternalLoadDocument() override
  {
    this->GetObjectManager()->DestroyAllObjects();

    plStatus ret = BaseClass::InternalLoadDocument();

    return ret;
  }

  // Index based remapping ignores address identity and solely uses the object's parent index to define
  // its guid. Set it to true if the native changes are complete clear and replace operations and
  // not incremental changes to the existing data.
  void ApplyNativePropertyChangesToObjectManager(bool bForceIndexBasedRemapping = false)
  {
    PL_PROFILE_SCOPE("ApplyNativePropertyChangesToObjectManager");
    // Create object manager graph
    plAbstractObjectGraph origGraph;
    plAbstractObjectNode* pOrigRootNode = nullptr;
    {
      plDocumentObjectConverterWriter writer(&origGraph, this->GetObjectManager());
      pOrigRootNode = writer.AddObjectToGraph(GetPropertyObject());
    }

    // Create native object graph
    plAbstractObjectGraph graph;
    plAbstractObjectNode* pRootNode = nullptr;
    {
      // The plApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those
      // of the object manager.
      plApplyNativePropertyChangesContext nativeChangesContext(m_Context, origGraph);
      plRttiConverterWriter rttiConverter(&graph, &nativeChangesContext, true, true);
      nativeChangesContext.RegisterObject(pOrigRootNode->GetGuid(), plGetStaticRTTI<PropertyType>(), GetProperties());
      pRootNode = rttiConverter.AddObjectToGraph(GetProperties(), "Object");
    }

    // Remapping is no longer necessary as plApplyNativePropertyChangesContext takes care of mapping to the original nodes.
    // However, if the native changes are done like clear+rebuild everything, then no original object will be found and
    // every pointer will be deleted and re-created. Forcing the remapping (which works entirely via index and ignores
    // pointer addresses) will yield better results (e.g. no changes on two back-to -back transform calls).
    if (bForceIndexBasedRemapping)
    {
      // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
      graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
    }

    plDeque<plAbstractGraphDiffOperation> diffResult;
    graph.CreateDiffWithBaseGraph(origGraph, diffResult);

    // if index-based remapping is used, we MUST send a change event of some kind
    // since the underlying data structures (memory locations) might have been changed,
    // even if there is no actual change to the content
    // the command history will detect that there was no change and actually send a "TransactionCanceled" event, but that is enough for other code to react to
    if (!diffResult.IsEmpty() || bForceIndexBasedRemapping)
    {
      // As we messed up the native side the object mirror is no longer synced and needs to be destroyed.
      m_ObjectMirror.Clear();

      // Apply diff while object mirror is down.
      this->GetObjectAccessor()->StartTransaction("Apply Native Property Changes to Object");

      plDocumentObjectConverterReader::ApplyDiffToObject(this->GetObjectAccessor(), GetPropertyObject(), diffResult);

      // Re-apply document
      m_ObjectMirror.SendDocument();

      this->GetObjectAccessor()->FinishTransaction();
    }
  }

private:
  void EnsureSettingsObjectExist()
  {
    auto pRoot = this->GetObjectManager()->GetRootObject();
    if (pRoot->GetChildren().IsEmpty())
    {
      plDocumentObject* pObject = this->GetObjectManager()->CreateObject(plGetStaticRTTI<PropertyType>());
      this->GetObjectManager()->AddObject(pObject, pRoot, "Children", 0);
    }
  }

protected:
  virtual plDocumentInfo* CreateDocumentInfo() override { return PL_DEFAULT_NEW(plAssetDocumentInfo); }

  plDocumentObjectMirror m_ObjectMirror;
  plRttiConverterContext m_Context;
  plEngineViewLightSettings m_LightSettings;
};
