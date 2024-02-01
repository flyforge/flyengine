#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plEditorEngineConnection;
class plEditorEngineSyncObject;
class plAssetDocumentManager;
class plPlatformProfile;
class QImage;

/// \brief Describes whether the asset document on the editor side also needs a rendering context on the engine side
enum class plAssetDocEngineConnection : plUInt8
{
  None,               ///< Use this when the document is fully self-contained and any UI is handled by Qt only. This is very common for 'data only' assets and everything that can't be visualized in 3D.
  Simple,             ///< Use this when the asset should be visualized in 3D. This requires a 'context' to be set up on the engine side that implements custom rendering. This is the most common type for anything that can be visualized in 3D, though can also be used for 2D data.
  FullObjectMirroring ///< In this mode the entire object hierarchy on the editor side is automatically synchronized over to an engine context. This is only needed for complex documents, such as scenes and prefabs.
};

/// \brief Frequently needed asset document states, to prevent code duplication
struct plCommonAssetUiState
{
  enum Enum : plUInt32
  {
    Pause = PL_BIT(0),
    Restart = PL_BIT(1),
    Loop = PL_BIT(2),
    SimulationSpeed = PL_BIT(3),
    Grid = PL_BIT(4),
    Visualizers = PL_BIT(5),
  };

  Enum m_State;
  double m_fValue = 0;
};

class PL_EDITORFRAMEWORK_DLL plAssetDocument : public plDocument
{
  PL_ADD_DYNAMIC_REFLECTION(plAssetDocument, plDocument);

public:
  /// \brief The thumbnail info containing the hash of the file is appended to assets.
  /// The serialized size of this class can't change since it is found by seeking to the end of the file.
  class PL_EDITORFRAMEWORK_DLL ThumbnailInfo
  {
  public:
    plResult Deserialize(plStreamReader& inout_reader);
    plResult Serialize(plStreamWriter& inout_writer) const;

    /// \brief Checks whether the stored file contains the same hash.
    bool IsThumbnailUpToDate(plUInt64 uiExpectedHash, plUInt16 uiVersion) const { return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion); }

    /// \brief Sets the asset file hash
    void SetFileHashAndVersion(plUInt64 uiHash, plUInt16 v)
    {
      m_uiHash = uiHash;
      m_uiVersion = v;
    }

    /// \brief Returns the serialized size of the thumbnail info.
    /// Used to seek to the end of the file and find the thumbnail info struct.
    constexpr plUInt32 GetSerializedSize() const { return 19; }

  private:
    plUInt64 m_uiHash = 0;
    plUInt16 m_uiVersion = 0;
    plUInt16 m_uiReserved = 0;
  };

  plAssetDocument(plStringView sDocumentPath, plDocumentObjectManager* pObjectManager, plAssetDocEngineConnection engineConnectionType);
  ~plAssetDocument();

  /// \name Asset Functions
  ///@{

  plAssetDocumentManager* GetAssetDocumentManager() const;
  const plAssetDocumentInfo* GetAssetDocumentInfo() const;

  plBitflags<plAssetDocumentFlags> GetAssetFlags() const;

  const plAssetDocumentTypeDescriptor* GetAssetDocumentTypeDescriptor() const
  {
    return static_cast<const plAssetDocumentTypeDescriptor*>(GetDocumentTypeDescriptor());
  }

  /// \brief Transforms an asset.
  ///   Typically not called manually but by the curator which takes care of dependencies first.
  ///
  /// If plTransformFlags::ForceTransform is set, it will try to transform the asset, ignoring whether the transform is up to date.
  /// If plTransformFlags::TriggeredManually is set, transform produced changes will be saved back to the document.
  /// If plTransformFlags::BackgroundProcessing is set and transforming the asset would require re-saving it, nothing is done.
  plTransformStatus TransformAsset(plBitflags<plTransformFlags> transformFlags, const plPlatformProfile* pAssetProfile = nullptr);

  /// \brief Updates the thumbnail of the asset.
  ///   Should never be called manually. Called only by the curator which takes care of dependencies first.
  plTransformStatus CreateThumbnail();

  /// \brief Returns the RTTI type version of this asset document type. E.g. when the algorithm to transform an asset changes,
  /// Increase the RTTI version. This will ensure that assets get re-transformed, even though their settings and dependencies might not have changed.
  plUInt16 GetAssetTypeVersion() const;

  ///@}
  /// \name IPC Functions
  ///@{

  enum class EngineStatus
  {
    Unsupported,  ///< This document does not have engine IPC.
    Disconnected, ///< Engine process crashed or not started yet.
    Initializing, ///< Document is being initialized on the engine process side.
    Loaded,       ///< Any message sent after this state is reached will work on a fully loaded document.
  };

  /// \brief Returns the current state of the engine process side of this document.
  EngineStatus GetEngineStatus() const { return m_EngineStatus; }

  /// \brief Passed into plEngineProcessDocumentContext::Initialize on the engine process side. Allows the document to provide additional data to the engine process during context creation.
  virtual plVariant GetCreateEngineMetaData() const { return plVariant(); }

  /// \brief Sends a message to the corresponding plEngineProcessDocumentContext on the engine process.
  bool SendMessageToEngine(plEditorEngineDocumentMsg* pMessage) const;

  /// \brief Handles all messages received from the corresponding plEngineProcessDocumentContext on the engine process.
  virtual void HandleEngineMessage(const plEditorEngineDocumentMsg* pMsg);

  /// \brief Returns the plEditorEngineConnection for this document.
  plEditorEngineConnection* GetEditorEngineConnection() const { return m_pEngineConnection; }

  /// \brief Registers a sync object for this document. It will be mirrored to the plEngineProcessDocumentContext on the engine process.
  void AddSyncObject(plEditorEngineSyncObject* pSync) const;

  /// \brief Removes a previously registered sync object. It will be removed on the engine process side.
  void RemoveSyncObject(plEditorEngineSyncObject* pSync) const;

  /// \brief Returns the sync object registered under the given guid.
  plEditorEngineSyncObject* FindSyncObject(const plUuid& guid) const;

  /// \brief Returns the first sync object registered with the given type.
  plEditorEngineSyncObject* FindSyncObject(const plRTTI* pType) const;

  /// \brief Sends messages to sync all sync objects to the engine process side.
  void SyncObjectsToEngine() const;

  /// /brief Sends a message that the document has been opened or closed. Resends all document data.
  ///
  /// Calling this will always clear the existing document on the engine side and reset the state to the editor state.
  void SendDocumentOpenMessage(bool bOpen);


  ///@}

  plEvent<const plEditorEngineDocumentMsg*> m_ProcessMessageEvent;

protected:
  void EngineConnectionEventHandler(const plEditorEngineProcessConnection::Event& e);

  /// \name Hash Functions
  ///@{

  /// \brief Computes the hash from all document objects
  plUInt64 GetDocumentHash() const;

  /// \brief Computes the hash for one document object and combines it with the given hash
  void GetChildHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const;

  /// \brief Computes the hash for transform relevant meta data of the given document object and combines it with the given hash.
  virtual void InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const {}

  ///@}
  /// \name Reimplemented Base Functions
  ///@{

  /// \brief Overrides the base function to call UpdateAssetDocumentInfo() to update the settings hash
  virtual plTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override;

  /// \brief Implements auto transform on save
  virtual void InternalAfterSaveDocument() override;

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;

  ///@}
  /// \name Asset Functions
  ///@{

  /// \brief Override this to add custom data (e.g. additional file dependencies) to the info struct.
  ///
  /// \note ALWAYS call the base function! It automatically fills out references that it can determine.
  ///       In most cases that is already sufficient.
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const;

  /// \brief Override this and write the transformed file for the given szOutputTag into the given stream.
  ///
  /// The stream already contains the plAssetFileHeader. This is the function to prefer when the asset can be written
  /// directly from the editor process. AssetHeader is already written to the stream, but provided as reference.
  ///
  /// \param stream Data stream to write the asset to.
  /// \param szOutputTag Either empty for the default output or matches one of the tags defined in plAssetDocumentInfo::m_Outputs.
  /// \param szPlatform Platform for which is the output is to be created. Default is 'PC'.
  /// \param AssetHeader Header already written to the stream, provided for reference.
  /// \param transformFlags flags that affect the transform process, see plTransformFlags.
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) = 0;

  /// \brief Only override this function, if the transformed file for the given szOutputTag must be written from another process.
  ///
  /// szTargetFile is where the transformed asset should be written to. The overriding function must ensure to first
  /// write \a AssetHeader to the file, to make it a valid asset file or provide a custom plAssetDocumentManager::IsOutputUpToDate function.
  /// See plTransformFlags for definition of transform flags.
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags);

  plStatus RemoteExport(const plAssetFileHeader& header, const char* szOutputTarget) const;

  ///@}
  /// \name Thumbnail Functions
  ///@{

  /// \brief Override this function to generate a thumbnail. Only called if GetAssetFlags returns plAssetDocumentFlags::SupportsThumbnail.
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo);

  /// \brief Returns the full path to the jpg file in which the thumbnail for this asset is supposed to be
  plString GetThumbnailFilePath(plStringView sSubAssetName = plStringView()) const;

  /// \brief Should be called after manually changing the thumbnail, such that the system will reload it
  void InvalidateAssetThumbnail(plStringView sSubAssetName = plStringView()) const;

  /// \brief Requests the engine side to render a thumbnail, will call SaveThumbnail on success.
  plStatus RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo, plArrayPtr<plStringView> viewExclusionTags /*= plStringView("SkyLight")*/) const;
  plStatus RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo) const
  {
    plStringView defVal("SkyLight");
    return RemoteCreateThumbnail(thumbnailInfo, {&defVal, 1});
  }

  /// \brief Saves the given image as the new thumbnail for the asset
  plStatus SaveThumbnail(const plImage& img, const ThumbnailInfo& thumbnailInfo) const;

  /// \brief Saves the given image as the new thumbnail for the asset
  plStatus SaveThumbnail(const QImage& img, const ThumbnailInfo& thumbnailInfo) const;

  /// \brief Appends an asset header containing the thumbnail hash to the file. Each thumbnail is appended by it to check up-to-date state.
  void AppendThumbnailInfo(plStringView sThumbnailFile, const ThumbnailInfo& thumbnailInfo) const;

  ///@}
  /// \name Common Asset States
  ///@{

public:
  /// \brief Override this to handle a change to a common asset state differently.
  ///
  /// By default an on-off flag for every state is tracked, but nothing else.
  /// Also this automatically broadcasts the m_CommonAssetUiChangeEvent event.
  virtual void SetCommonAssetUiState(plCommonAssetUiState::Enum state, double value);

  /// \brief Override this to return custom values for a common asset state.
  virtual double GetCommonAssetUiState(plCommonAssetUiState::Enum state) const;

  /// \brief Used to broadcast state change events for common asset states.
  plEvent<const plCommonAssetUiState&> m_CommonAssetUiChangeEvent;

protected:
  plUInt32 m_uiCommonAssetStateFlags = 0;

  ///@}

protected:
  /// \brief Adds all prefab dependencies to the plAssetDocumentInfo object. Called automatically by UpdateAssetDocumentInfo()
  void AddPrefabDependencies(const plDocumentObject* pObject, plAssetDocumentInfo* pInfo) const;

  /// \brief Crawls through all asset properties of pObject and adds all string properties that have a plAssetBrowserAttribute as a dependency to
  /// pInfo. Automatically called by UpdateAssetDocumentInfo()
  void AddReferences(const plDocumentObject* pObject, plAssetDocumentInfo* pInfo, bool bInsidePrefab) const;

protected:
  plUniquePtr<plIPCObjectMirrorEditor> m_pMirror;

  virtual plDocumentInfo* CreateDocumentInfo() override;

  plTransformStatus DoTransformAsset(const plPlatformProfile* pAssetProfile, plBitflags<plTransformFlags> transformFlags);

  EngineStatus m_EngineStatus;
  plAssetDocEngineConnection m_EngineConnectionType = plAssetDocEngineConnection::None;

  plEditorEngineConnection* m_pEngineConnection;

  mutable plHashTable<plUuid, plEditorEngineSyncObject*> m_AllSyncObjects;
  mutable plDeque<plEditorEngineSyncObject*> m_SyncObjects;

  mutable plHybridArray<plUuid, 32> m_DeletedObjects;
};
