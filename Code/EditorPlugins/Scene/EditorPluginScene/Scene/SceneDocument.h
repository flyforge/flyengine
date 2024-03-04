#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>

class plExposedSceneProperty;
class plSceneDocumentSettingsBase;
class plPushObjectStateMsgToEditor;

struct GameMode
{
  enum Enum
  {
    Off,
    Simulate,
    Play,
  };
};

class PL_EDITORPLUGINSCENE_DLL plSceneDocument : public plGameObjectDocument
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneDocument, plGameObjectDocument);

public:
  enum class DocumentType
  {
    Scene,
    Prefab,
    Layer
  };

public:
  plSceneDocument(plStringView sDocumentPath, DocumentType documentType);
  ~plSceneDocument();

  enum class ShowOrHide
  {
    Show,
    Hide
  };

  void GroupSelection();

  /// \brief Opens the Duplicate Special dialog
  void DuplicateSpecial();

  /// \brief Opens the 'Delta Transform' dialog.
  void DeltaTransform();


  /// \brief Moves all selected objects to the editor camera position
  void SnapObjectToCamera();


  /// \brief Attaches all selected objects to the selected object
  void AttachToObject();

  /// \brief Detaches all selected objects from their current parent
  void DetachFromParent();

  /// \brief Puts the GUID of the single selected object into the clipboard
  void CopyReference();

  /// \brief Creates a new empty object, either top-level (selection empty) or as a child of the selected item
  plStatus CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition);

  void DuplicateSelection();
  void ToggleHideSelectedObjects();
  void ShowOrHideSelectedObjects(ShowOrHide action);
  void ShowOrHideAllObjects(ShowOrHide action);
  void HideUnselectedObjects();

  /// \brief Whether this document represents a prefab or a scene
  bool IsPrefab() const { return m_DocumentType == DocumentType::Prefab; }

  /// \brief Determines whether the given object is an editor prefab
  bool IsObjectEditorPrefab(const plUuid& object, plUuid* out_pPrefabAssetGuid = nullptr) const;

  /// \brief Determines whether the given object is an engine prefab
  bool IsObjectEnginePrefab(const plUuid& object, plUuid* out_pPrefabAssetGuid = nullptr) const;

  /// \brief Nested prefabs are not allowed
  virtual bool ArePrefabsAllowed() const override { return !IsPrefab(); }


  virtual void GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_sMimeType) const override;
  virtual bool Paste(
    const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType) override;
  bool DuplicateSelectedObjects(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bSetSelected);
  bool CopySelectedObjects(plAbstractObjectGraph& ref_graph, plMap<plUuid, plUuid>* out_pParents) const;
  bool PasteAt(const plArrayPtr<PasteInfo>& info, const plVec3& vPos);
  bool PasteAtOrignalPosition(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph);

  virtual void UpdatePrefabs() override;

  /// \brief Removes the link to the prefab template, making the editor prefab a simple object
  virtual void UnlinkPrefabs(const plDeque<const plDocumentObject*>& selection) override;

  virtual plUuid ReplaceByPrefab(
    const plDocumentObject* pRootObject, plStringView sPrefabFile, const plUuid& prefabAsset, const plUuid& prefabSeed, bool bEnginePrefab) override;

  /// \brief Reverts all selected editor prefabs to their original template state
  virtual plUuid RevertPrefab(const plDocumentObject* pObject) override;

  /// \brief Converts all objects in the selection that are engine prefabs to their respective editor prefab representation
  virtual void ConvertToEditorPrefab(const plDeque<const plDocumentObject*>& selection);
  /// \brief Converts all objects in the selection that are editor prefabs to their respective engine prefab representation
  virtual void ConvertToEnginePrefab(const plDeque<const plDocumentObject*>& selection);

  virtual plStatus CreatePrefabDocumentFromSelection(plStringView sFile, const plRTTI* pRootType, plDelegate<void(plAbstractObjectNode*)> adjustGraphNodeCB = {}, plDelegate<void(plDocumentObject*)> adjustNewNodesCB = {}, plDelegate<void(plAbstractObjectGraph& graph, plDynamicArray<plAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {}) override;

  GameMode::Enum GetGameMode() const { return m_GameMode; }

  virtual bool CanEngineProcessBeRestarted() const override;

  void StartSimulateWorld();
  void TriggerGameModePlay(bool bUsePickedPositionAsStart);

  /// Stops the world simulation, if it is running. Returns true, when the simulation needed to be stopped.
  bool StopGameMode();

  plTransformStatus ExportScene(bool bCreateThumbnail);
  void ExportSceneGeometry(
    const char* szFile, bool bOnlySelection, int iExtractionMode /* plWorldGeoExtractionUtil::ExtractionMode */, const plMat3& mTransform);

  virtual void HandleEngineMessage(const plEditorEngineDocumentMsg* pMsg) override;
  void HandleGameModeMsg(const plGameModeMsgToEditor* pMsg);
  void HandleObjectStateFromEngineMsg(const plPushObjectStateMsgToEditor* pMsg);

  void SendObjectMsg(const plDocumentObject* pObj, plObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const plDocumentObject* pObj, plObjectTagMsgToEngine* pMsg);

  /// \name Scene Settings
  ///@{

  virtual const plDocumentObject* GetSettingsObject() const;
  const plSceneDocumentSettingsBase* GetSettingsBase() const;
  template <typename T>
  const T* GetSettings() const
  {
    return plDynamicCast<const T*>(GetSettingsBase());
  }

  plStatus CreateExposedProperty(
    const plDocumentObject* pObject, const plAbstractProperty* pProperty, plVariant index, plExposedSceneProperty& out_key) const;
  plStatus AddExposedParameter(const char* szName, const plDocumentObject* pObject, const plAbstractProperty* pProperty, plVariant index);
  plInt32 FindExposedParameter(const plDocumentObject* pObject, const plAbstractProperty* pProperty, plVariant index);
  plStatus RemoveExposedParameter(plInt32 iIndex);
  ///@}

  /// \name Editor Camera
  ///@{

  /// \brief Stores the current editor camera position in a user preference. Slot can be 0 to 9.
  ///
  /// Since the preference is stored on disk, this position can be restored in another session.
  void StoreFavoriteCamera(plUInt8 uiSlot);

  /// \brief Applies the previously stored camera position from slot 0 to 9 to the current camera position.
  ///
  /// The camera will quickly interpolate to the stored position.
  void RestoreFavoriteCamera(plUInt8 uiSlot);

  /// \brief Searches for an plCameraComponent with the 'EditorShortcut' property set to \a uiSlot and moves the editor camera to that position.
  plResult JumpToLevelCamera(plUInt8 uiSlot, bool bImmediate);

  /// \brief Creates an object with an plCameraComponent at the current editor camera position and sets the 'EditorShortcut' property to \a uiSlot.
  plResult CreateLevelCamera(plUInt8 uiSlot);

  virtual plManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return plManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

  ///@}

protected:
  void SetGameMode(GameMode::Enum mode);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void UpdatePrefabObject(plDocumentObject* pObject, const plUuid& PrefabAsset, const plUuid& PrefabSeed, plStringView sBasePrefab) override;
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;

  template <typename Func>
  void ApplyRecursive(const plDocumentObject* pObject, Func f)
  {
    f(pObject);

    for (auto pChild : pObject->GetChildren())
    {
      ApplyRecursive<Func>(pChild, f);
    }
  }

protected:
  void EnsureSettingsObjectExist();
  void DocumentObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plDocumentObjectMetaData>::EventData& e);
  void EngineConnectionEventHandler(const plEditorEngineProcessConnection::Event& e);
  void ToolsProjectEventHandler(const plToolsProjectEvent& e);

  plStatus RequestExportScene(const char* szTargetFile, const plAssetFileHeader& header);

  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  void SyncObjectHiddenState();
  void SyncObjectHiddenState(plDocumentObject* pObject);

  /// \brief Finds all objects that are actively being 'debugged' (or visualized) by the editor and thus should get the debug visualization flag in
  /// the runtime.
  void UpdateObjectDebugTargets();

  DocumentType m_DocumentType = DocumentType::Scene;

  GameMode::Enum m_GameMode;

  GameModeData m_GameModeData[3];

  // Local mirror for settings
  plDocumentObjectMirror m_ObjectMirror;
  plRttiConverterContext m_Context;

  //////////////////////////////////////////////////////////////////////////
  /// Communication with other document types
  virtual void OnInterDocumentMessage(plReflectedClass* pMessage, plDocument* pSender) override;
  void GatherObjectsOfType(plDocumentObject* pRoot, plGatherObjectsOfTypeMsgInterDoc* pMsg) const;
};
