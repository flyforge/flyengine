#pragma once

#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class plScene2Document;

class plSceneLayerBase : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneLayerBase, plReflectedClass);

public:
  plSceneLayerBase();
  ~plSceneLayerBase();

public:
  mutable plScene2Document* m_pDocument = nullptr;
};

class plSceneLayer : public plSceneLayerBase
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneLayer, plSceneLayerBase);

public:
  plSceneLayer();
  ~plSceneLayer();

public:
  plUuid m_Layer;
};

class plSceneDocumentSettings : public plSceneDocumentSettingsBase
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneDocumentSettings, plSceneDocumentSettingsBase);

public:
  plSceneDocumentSettings();
  ~plSceneDocumentSettings();

public:
  plDynamicArray<plSceneLayerBase*> m_Layers;
  mutable plScene2Document* m_pDocument = nullptr;
};

struct plScene2LayerEvent
{
  enum class Type
  {
    LayerAdded,
    LayerRemoved,
    LayerLoaded,
    LayerUnloaded,
    LayerVisible,
    LayerInvisible,
    ActiveLayerChanged,
  };

  Type m_Type;
  plUuid m_layerGuid;
};

class PL_EDITORPLUGINSCENE_DLL plScene2Document : public plSceneDocument
{
  PL_ADD_DYNAMIC_REFLECTION(plScene2Document, plSceneDocument);

public:
  plScene2Document(plStringView sDocumentPath);
  ~plScene2Document();

  /// \name Scene Data Accessors
  ///@{

  const plDocumentObjectManager* GetSceneObjectManager() const { return m_pSceneObjectManager.Borrow(); }
  plDocumentObjectManager* GetSceneObjectManager() { return m_pSceneObjectManager.Borrow(); }
  plSelectionManager* GetSceneSelectionManager() const { return m_pSceneSelectionManager.Borrow(); }
  plCommandHistory* GetSceneCommandHistory() const { return m_pSceneCommandHistory.Borrow(); }
  plObjectAccessorBase* GetSceneObjectAccessor() const { return m_pSceneObjectAccessor.Borrow(); }
  const plObjectMetaData<plUuid, plDocumentObjectMetaData>* GetSceneDocumentObjectMetaData() const { return m_pSceneDocumentObjectMetaData.Borrow(); }
  plObjectMetaData<plUuid, plDocumentObjectMetaData>* GetSceneDocumentObjectMetaData() { return m_pSceneDocumentObjectMetaData.Borrow(); }
  const plObjectMetaData<plUuid, plGameObjectMetaData>* GetSceneGameObjectMetaData() const { return m_pSceneGameObjectMetaData.Borrow(); }
  plObjectMetaData<plUuid, plGameObjectMetaData>* GetSceneGameObjectMetaData() { return m_pSceneGameObjectMetaData.Borrow(); }

  ///@}
  /// \name Layer Functions
  ///@{

  plSelectionManager* GetLayerSelectionManager() const { return m_pLayerSelection.Borrow(); }

  plStatus CreateLayer(const char* szName, plUuid& out_layerGuid);
  plStatus DeleteLayer(const plUuid& layerGuid);

  const plUuid& GetActiveLayer() const;
  plStatus SetActiveLayer(const plUuid& layerGuid);

  bool IsLayerLoaded(const plUuid& layerGuid) const;
  plStatus SetLayerLoaded(const plUuid& layerGuid, bool bLoaded);
  void GetAllLayers(plDynamicArray<plUuid>& out_layerGuids);
  void GetLoadedLayers(plDynamicArray<plSceneDocument*>& out_layers) const;

  bool IsLayerVisible(const plUuid& layerGuid) const;
  plStatus SetLayerVisible(const plUuid& layerGuid, bool bVisible);

  const plDocumentObject* GetLayerObject(const plUuid& layerGuid) const;
  plSceneDocument* GetLayerDocument(const plUuid& layerGuid) const;

  bool IsAnyLayerModified() const;

  ///@}
  /// \name Base Class Functions
  ///@{

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;
  virtual const plDocumentObject* GetSettingsObject() const override;
  virtual void HandleEngineMessage(const plEditorEngineDocumentMsg* pMsg) override;
  virtual plTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override;
  virtual void SendGameWorldToEngine() override;

  ///@}

public:
  mutable plEvent<const plScene2LayerEvent&> m_LayerEvents;

private:
  void LayerSelectionEventHandler(const plSelectionManagerEvent& e);
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);
  void DocumentManagerEventHandler(const plDocumentManager::Event& e);
  void HandleObjectStateFromEngineMsg2(const plPushObjectStateMsgToEditor* pMsg);

  void UpdateLayers();
  void SendLayerVisibility();
  void LayerAdded(const plUuid& layerGuid, const plUuid& layerObjectGuid);
  void LayerRemoved(const plUuid& layerGuid);

private:
  friend class plSceneLayer;
  plCopyOnBroadcastEvent<const plDocumentObjectStructureEvent&>::Unsubscriber m_StructureEventSubscriber;
  plCopyOnBroadcastEvent<const plSelectionManagerEvent&>::Unsubscriber m_LayerSelectionEventSubscriber;
  plEvent<const plCommandHistoryEvent&, plMutex>::Unsubscriber m_CommandHistoryEventSubscriber;
  plCopyOnBroadcastEvent<const plDocumentManager::Event&>::Unsubscriber m_DocumentManagerEventSubscriber;

  // This is used for a flattened list of the plSceneDocumentSettings hierarchy
  struct LayerInfo
  {
    plSceneDocument* m_pLayer = nullptr;
    plUuid m_objectGuid;
    bool m_bVisible = true;
  };

  // Scene document cache
  plUniquePtr<plDocumentObjectManager> m_pSceneObjectManager;
  mutable plUniquePtr<plCommandHistory> m_pSceneCommandHistory;
  mutable plUniquePtr<plSelectionManager> m_pSceneSelectionManager;
  mutable plUniquePtr<plObjectCommandAccessor> m_pSceneObjectAccessor;
  plUniquePtr<plObjectMetaData<plUuid, plDocumentObjectMetaData>> m_pSceneDocumentObjectMetaData;
  plUniquePtr<plObjectMetaData<plUuid, plGameObjectMetaData>> m_pSceneGameObjectMetaData;

  // Layer state
  mutable plUniquePtr<plSelectionManager> m_pLayerSelection;
  plUuid m_ActiveLayerGuid;
  plHashTable<plUuid, LayerInfo> m_Layers;
};
