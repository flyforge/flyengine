#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Implementation/Declarations.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plObjectAccessorBase;
class plObjectCommandAccessor;
class plEditorInputContext;
class plAbstractObjectNode;

struct PLASMA_TOOLSFOUNDATION_DLL plObjectAccessorChangeEvent
{
  plDocument* m_pDocument;
  plObjectAccessorBase* m_pOldObjectAccessor;
  plObjectAccessorBase* m_pNewObjectAccessor;
};

class PLASMA_TOOLSFOUNDATION_DLL plDocumentObjectMetaData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentObjectMetaData, plReflectedClass);

public:
  enum ModifiedFlags : unsigned int
  {
    HiddenFlag = PLASMA_BIT(0),
    PrefabFlag = PLASMA_BIT(1),

    AllFlags = 0xFFFFFFFF
  };

  plDocumentObjectMetaData() { m_bHidden = false; }

  bool m_bHidden;            /// Whether the object should be rendered in the editor view (no effect on the runtime)
  plUuid m_CreateFromPrefab; /// The asset GUID of the prefab from which this object was created. Invalid GUID, if this is not a prefab instance.
  plUuid m_PrefabSeedGuid;   /// The seed GUID used to remap the object GUIDs from the prefab asset into this instance.
  plString m_sBasePrefab;    /// The prefab from which this instance was created as complete DDL text (this describes the entire object!). Necessary for
                             /// three-way-merging the prefab instances.
};

enum class plManipulatorSearchStrategy
{
  None,
  SelectedObject,
  ChildrenOfSelectedObject
};

class PLASMA_TOOLSFOUNDATION_DLL plDocument : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocument, plReflectedClass);

public:
  plDocument(plStringView sPath, plDocumentObjectManager* pDocumentObjectManagerImpl);
  virtual ~plDocument();

  /// \name Document State Functions
  ///@{

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }
  const plUuid GetGuid() const { return m_pDocumentInfo ? m_pDocumentInfo->m_DocumentID : plUuid(); }

  const plDocumentObjectManager* GetObjectManager() const { return m_pObjectManager.Borrow(); }
  plDocumentObjectManager* GetObjectManager() { return m_pObjectManager.Borrow(); }
  plSelectionManager* GetSelectionManager() const { return m_pSelectionManager.Borrow(); }
  plCommandHistory* GetCommandHistory() const { return m_pCommandHistory.Borrow(); }
  virtual plObjectAccessorBase* GetObjectAccessor() const;

  ///@}
  /// \name Main / Sub-Document Functions
  ///@{

  /// \brief Returns whether this document is a main document, i.e. self contained.
  bool IsMainDocument() const { return m_pHostDocument == this; }
  /// \brief Returns whether this document is a sub-document, i.e. is part of another document.
  bool IsSubDocument() const { return m_pHostDocument != this; }
  /// \brief In case this is a sub-document, returns the main document this belongs to. Otherwise 'this' is returned.
  const plDocument* GetMainDocument() const { return m_pHostDocument; }
  /// @brief At any given time, only the active sub-document can be edited. This returns the active sub-document which can also be this document itself. Changes to the active sub-document are generally triggered by plDocumentObjectStructureEvent::Type::AfterReset.
  const plDocument* GetActiveSubDocument() const { return m_pActiveSubDocument; }
  plDocument* GetMainDocument() { return m_pHostDocument; }
  plDocument* GetActiveSubDocument() { return m_pActiveSubDocument; }

protected:
  plDocument* m_pHostDocument = nullptr;
  plDocument* m_pActiveSubDocument = nullptr;

  ///@}
  /// \name Document Management Functions
  ///@{

public:
  /// \brief Returns the absolute path to the document.
  plStringView GetDocumentPath() const { return m_sDocumentPath; }

  /// \brief Saves the document, if it is modified.
  /// If bForce is true, the document will be written, even if it is not considered modified.
  plStatus SaveDocument(bool bForce = false);
  using AfterSaveCallback = plDelegate<void(plDocument*, plStatus)>;
  plTaskGroupID SaveDocumentAsync(AfterSaveCallback callback, bool bForce = false);
  void DocumentRenamed(plStringView sNewDocumentPath);

  static plStatus ReadDocument(plStringView sDocumentPath, plUniquePtr<plAbstractObjectGraph>& ref_pHeader, plUniquePtr<plAbstractObjectGraph>& ref_pObjects,
    plUniquePtr<plAbstractObjectGraph>& ref_pTypes);
  static plStatus ReadAndRegisterTypes(const plAbstractObjectGraph& types);

  plStatus LoadDocument() { return InternalLoadDocument(); }

  /// \brief Brings the corresponding window to the front.
  void EnsureVisible();

  plDocumentManager* GetDocumentManager() const { return m_pDocumentManager; }

  bool HasWindowBeenRequested() const { return m_bWindowRequested; }

  const plDocumentTypeDescriptor* GetDocumentTypeDescriptor() const { return m_pTypeDescriptor; }

  /// \brief Returns the document's type name. Same as GetDocumentTypeDescriptor()->m_sDocumentTypeName.
  plStringView GetDocumentTypeName() const
  {
    if (m_pTypeDescriptor == nullptr)
    {
      // if this is a document without a type descriptor, use the RTTI type name as a fallback
      return GetDynamicRTTI()->GetTypeName();
    }

    return m_pTypeDescriptor->m_sDocumentTypeName;
  }

  const plDocumentInfo* GetDocumentInfo() const { return m_pDocumentInfo; }

  /// \brief Asks the document whether a restart of the engine process is allowed at this time.
  ///
  /// Documents that are currently interacting with the engine process (active play-the-game mode) should return false.
  /// All others should return true.
  /// As long as any document returns false, automatic engine process reload is suppressed.
  virtual bool CanEngineProcessBeRestarted() const { return true; }

  ///@}
  /// \name Clipboard Functions
  ///@{

  struct PasteInfo
  {
    PLASMA_DECLARE_POD_TYPE();

    plDocumentObject* m_pObject = nullptr;
    plDocumentObject* m_pParent = nullptr;
    plInt32 m_Index = -1;
  };

  /// \brief Whether this document supports pasting the given mime format into it
  virtual void GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_mimeTypes) const {}
  /// \brief Creates the abstract graph of data to be copied and returns the mime type for the clipboard to identify the data
  virtual bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_sMimeType) const { return false; };
  virtual bool Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType)
  {
    return false;
  };

  ///@}
  /// \name Inter Document Communication
  ///@{

  /// \brief This will deliver the message to all open documents. The documents may respond, e.g. by modifying the content of the message.
  void BroadcastInterDocumentMessage(plReflectedClass* pMessage, plDocument* pSender);

  /// \brief Called on all documents when BroadcastInterDocumentMessage() is called.
  ///
  /// Use the RTTI information to identify whether the message is of interest.
  virtual void OnInterDocumentMessage(plReflectedClass* pMessage, plDocument* pSender) {}

  ///@}
  /// \name Editing Functionality
  ///@{

  /// \brief Allows to return a single input context that currently overrides all others (in priority).
  ///
  /// Used to implement custom tools that need to have priority over selection and camera movement.
  virtual plEditorInputContext* GetEditorInputContextOverride() { return nullptr; }

  ///@}
  /// \name Misc Functions
  ///@{

  virtual void DeleteSelectedObjects() const;

  const plSet<plString>& GetUnknownObjectTypes() const { return m_UnknownObjectTypes; }
  plUInt32 GetUnknownObjectTypeInstances() const { return m_uiUnknownObjectTypeInstances; }

  /// \brief If disabled, this document will not be put into the recent files list.
  void SetAddToResetFilesList(bool b) { m_bAddToRecentFilesList = b; }

  /// \brief Whether this document shall be put into the recent files list.
  bool GetAddToRecentFilesList() const { return m_bAddToRecentFilesList; }

  /// \brief Broadcasts a status message event. The window that displays the document may show this in some form, e.g. in the status bar.
  void ShowDocumentStatus(const plFormatString& msg) const;

  /// \brief Tries to compute the position and rotation for an object in the document. Returns PLASMA_SUCCESS if it was possible.
  virtual plResult ComputeObjectTransformation(const plDocumentObject* pObject, plTransform& out_result) const;

  /// \brief Needed by plManipulatorManager to know where to look for the manipulator attributes.
  ///
  /// Override this function for document types that use manipulators.
  /// The plManipulatorManager will assert that the document type doesn't return 'None' once it is in use.
  virtual plManipulatorSearchStrategy GetManipulatorSearchStrategy() const { return plManipulatorSearchStrategy::None; }

  ///@}
  /// \name Prefab Functions
  ///@{

  /// \brief Whether the document allows to create prefabs in it. This may note be allowed for prefab documents themselves, to prevent nested prefabs.
  virtual bool ArePrefabsAllowed() const { return true; }

  /// \brief Updates ALL prefabs in the document with the latest changes. Merges the current prefab templates with the instances in the document.
  virtual void UpdatePrefabs();

  /// \brief Resets the given objects to their template prefab state, if they have local modifications.
  void RevertPrefabs(const plDeque<const plDocumentObject*>& selection);

  /// \brief Removes the link between a prefab instance and its template, turning the instance into a regular object.
  virtual void UnlinkPrefabs(const plDeque<const plDocumentObject*>& selection);

  virtual plStatus CreatePrefabDocumentFromSelection(plStringView sFile, const plRTTI* pRootType, plDelegate<void(plAbstractObjectNode*)> adjustGraphNodeCB = {}, plDelegate<void(plDocumentObject*)> adjustNewNodesCB = {}, plDelegate<void(plAbstractObjectGraph& graph, plDynamicArray<plAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});
  virtual plStatus CreatePrefabDocument(plStringView sFile, plArrayPtr<const plDocumentObject*> rootObjects, const plUuid& invPrefabSeed, plUuid& out_newDocumentGuid, plDelegate<void(plAbstractObjectNode*)> adjustGraphNodeCB = {}, bool bKeepOpen = false, plDelegate<void(plAbstractObjectGraph& graph, plDynamicArray<plAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});

  // Returns new guid of replaced object.
  virtual plUuid ReplaceByPrefab(const plDocumentObject* pRootObject, plStringView sPrefabFile, const plUuid& prefabAsset, const plUuid& prefabSeed, bool bEnginePrefab);
  // Returns new guid of reverted object.
  virtual plUuid RevertPrefab(const plDocumentObject* pObject);

  ///@}

public:
  plUniquePtr<plObjectMetaData<plUuid, plDocumentObjectMetaData>> m_DocumentObjectMetaData;

  mutable plEvent<const plDocumentEvent&> m_EventsOne;
  static plEvent<const plDocumentEvent&> s_EventsAny;

  mutable plEvent<const plObjectAccessorChangeEvent&> m_ObjectAccessorChangeEvents;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  virtual plTaskGroupID InternalSaveDocument(AfterSaveCallback callback);
  virtual plStatus InternalLoadDocument();
  virtual plDocumentInfo* CreateDocumentInfo() = 0;

  /// \brief A hook to execute additional code after SUCCESSFULLY saving a document. E.g. manual asset transform can be done here.
  virtual void InternalAfterSaveDocument() {}

  virtual void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const;
  virtual void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) {}
  virtual void InitializeAfterLoadingAndSaving() {}

  virtual void BeforeClosing();

  void SetUnknownObjectTypes(const plSet<plString>& Types, plUInt32 uiInstances);

  /// \name Prefab Functions
  ///@{

  virtual void UpdatePrefabsRecursive(plDocumentObject* pObject);
  virtual void UpdatePrefabObject(plDocumentObject* pObject, const plUuid& PrefabAsset, const plUuid& PrefabSeed, plStringView sBasePrefab);

  ///@}

  plUniquePtr<plDocumentObjectManager> m_pObjectManager;
  mutable plUniquePtr<plCommandHistory> m_pCommandHistory;
  mutable plUniquePtr<plSelectionManager> m_pSelectionManager;
  mutable plUniquePtr<plObjectCommandAccessor> m_pObjectAccessor; ///< Default object accessor used by every doc.

  plDocumentInfo* m_pDocumentInfo = nullptr;
  const plDocumentTypeDescriptor* m_pTypeDescriptor = nullptr;

private:
  friend class plDocumentManager;
  friend class plCommandHistory;
  friend class plSaveDocumentTask;
  friend class plAfterSaveDocumentTask;

  void SetupDocumentInfo(const plDocumentTypeDescriptor* pTypeDescriptor);

  plDocumentManager* m_pDocumentManager = nullptr;

  plString m_sDocumentPath;
  bool m_bModified;
  bool m_bReadOnly;
  bool m_bWindowRequested;
  bool m_bAddToRecentFilesList;

  plSet<plString> m_UnknownObjectTypes;
  plUInt32 m_uiUnknownObjectTypeInstances;

  plTaskGroupID m_ActiveSaveTask;
  plStatus m_LastSaveResult;
};
