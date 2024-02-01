#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>
#include <Foundation/Types/Uuid.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class plEditorEngineSyncObjectMsg;
class plEditorEngineSyncObject;
class plEditorEngineDocumentMsg;
class plEngineProcessViewContext;
class plEngineProcessCommunicationChannel;
class plProcessMessage;
class plExportDocumentMsgToEngine;
class plCreateThumbnailMsgToEngine;
struct plResourceEvent;

struct plEngineProcessDocumentContextFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 0,
    CreateWorld = PL_BIT(0),
    Default = None
  };

  struct Bits
  {
    StorageType CreateWorld : 1;
  };
};
PL_DECLARE_FLAGS_OPERATORS(plEngineProcessDocumentContextFlags);

/// \brief A document context is the counter part to an editor document on the engine side.
///
/// For every document in the editor that requires engine output (rendering, picking, etc.), there is a plEngineProcessDocumentContext
/// created in the engine process.
class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plEngineProcessDocumentContext : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plEngineProcessDocumentContext, plReflectedClass);

public:
  plEngineProcessDocumentContext(plBitflags<plEngineProcessDocumentContextFlags> flags);
  virtual ~plEngineProcessDocumentContext();

  virtual void Initialize(const plUuid& documentGuid, const plVariant& metaData, plEngineProcessCommunicationChannel* pIPC, plStringView sDocumentType);
  void Deinitialize();

  /// \brief Returns the document type for which this context was created. Useful in case a context may be used for multiple document types.
  plStringView GetDocumentType() const { return m_sDocumentType; }

  void SendProcessMessage(plProcessMessage* pMsg = nullptr);
  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg);

  static plEngineProcessDocumentContext* GetDocumentContext(plUuid guid);
  static void AddDocumentContext(plUuid guid, const plVariant& metaData, plEngineProcessDocumentContext* pView, plEngineProcessCommunicationChannel* pIPC, plStringView sDocumentType);
  static bool PendingOperationsInProgress();
  static void UpdateDocumentContexts();
  static void DestroyDocumentContext(plUuid guid);

  // \brief Returns the bounding box of the objects in the world.
  plBoundingBoxSphere GetWorldBounds(plWorld* pWorld);

  void ProcessEditorEngineSyncObjectMsg(const plEditorEngineSyncObjectMsg& msg);

  const plUuid& GetDocumentGuid() const { return m_DocumentGuid; }

  virtual void Reset();
  void ClearExistingObjects();

  plIPCObjectMirrorEngine m_Mirror;
  plWorldRttiConverterContext m_Context; // TODO: Move actual context into the EngineProcessDocumentContext
  virtual plWorldRttiConverterContext& GetContext() { return m_Context; }
  virtual const plWorldRttiConverterContext& GetContext() const { return m_Context; }

  plWorld* GetWorld() const { return m_pWorld; }

  /// \brief Tries to resolve a 'reference' (given in pData) to an plGameObject.
  virtual plGameObjectHandle ResolveStringToGameObjectHandle(const void* pString, plComponentHandle hThis, plStringView sProperty) const;

protected:
  virtual void OnInitialize();
  virtual void OnDeinitialize();

  /// \brief Needs to be implemented to create a view context used for windows and thumbnails rendering.
  virtual plEngineProcessViewContext* CreateViewContext() = 0;
  /// \brief Needs to be implemented to destroy the view context created in CreateViewContext.
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) = 0;

  /// \brief Should return true if this context has any operation in progress like thumbnail rendering
  /// and thus needs to continue rendering even if no new messages from the editor come in.
  virtual bool PendingOperationInProgress() const;

  /// \brief A tick functions that allows each document context to do processing that continues
  /// over multiple frames and can't be handled in HandleMessage directly.
  ///
  /// Make sure to call the base implementation when overwriting as this handles the thumbnail
  /// rendering that takes multiple frames to complete.
  virtual void UpdateDocumentContext();

  /// \brief Exports to current document resource to file. Make sure to write plAssetFileHeader at the start of it.
  virtual plStatus ExportDocument(const plExportDocumentMsgToEngine* pMsg);
  void UpdateSyncObjects();

  /// \brief Creates the thumbnail view context. It uses 'CreateViewContext' in combination with an off-screen render target.
  void CreateThumbnailViewContext(const plCreateThumbnailMsgToEngine* pMsg);

  /// \brief Once a thumbnail is successfully rendered, the thumbnail view context is destroyed again.
  void DestroyThumbnailViewContext();

  /// \brief Overwrite this function to apply the thumbnail render settings to the given context.
  ///
  /// Return false if you need more frames to be rendered to setup everything correctly.
  /// If true is returned for 'ThumbnailConvergenceFramesTarget' frames in a row the thumbnail image is taken.
  /// This is to allow e.g. camera updates after more resources have been streamed in. The frame counter
  /// will start over to count to 'ThumbnailConvergenceFramesTarget' when a new resource is being loaded
  /// to make sure we do not make an image of half-streamed in data.
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext);

  /// \brief Called before a thumbnail context is created.
  virtual void OnThumbnailViewContextRequested() {}
  /// \brief Called after a thumbnail context was created. Allows to insert code before the thumbnail is generated.
  virtual void OnThumbnailViewContextCreated();
  /// \brief Called before a thumbnail context is destroyed. Used for cleanup of what was done in OnThumbnailViewContextCreated()
  virtual void OnDestroyThumbnailViewContext();

  plWorld* m_pWorld = nullptr;

  /// \brief Sets or removes the given tag on the object and optionally all children
  void SetTagOnObject(const plUuid& object, const char* szTag, bool bSet, bool recursive);

  /// \brief Sets the given tag on the object and all children.
  void SetTagRecursive(plGameObject* pObject, const plTag& tag);
  /// \brief Clears the given tag on the object and all children.
  void ClearTagRecursive(plGameObject* pObject, const plTag& tag);

protected:
  const plEngineProcessViewContext* GetViewContext(plUInt32 uiView) const
  {
    return uiView >= m_ViewContexts.GetCount() ? nullptr : m_ViewContexts[uiView];
  }

private:
  friend class plEditorEngineSyncObject;

  void AddSyncObject(plEditorEngineSyncObject* pSync);
  void RemoveSyncObject(plEditorEngineSyncObject* pSync);
  plEditorEngineSyncObject* FindSyncObject(const plUuid& guid);


private:
  void ClearViewContexts();

  // Maps a document guid to the corresponding context that handles that document on the engine side
  static plHashTable<plUuid, plEngineProcessDocumentContext*> s_DocumentContexts;

  /// Removes all sync objects that are tied to this context
  void CleanUpContextSyncObjects();

protected:
  plBitflags<plEngineProcessDocumentContextFlags> m_Flags;
  plUuid m_DocumentGuid;
  plVariant m_MetaData;

  plEngineProcessCommunicationChannel* m_pIPC = nullptr;
  plHybridArray<plEngineProcessViewContext*, 4> m_ViewContexts;

  plMap<plUuid, plEditorEngineSyncObject*> m_SyncObjects;

private:
  enum Constants
  {
    ThumbnailSuperscaleFactor =
      2,                                 ///< Thumbnail render target size is multiplied by this and then the final image is downscaled again. Needs to be power-of-two.
    ThumbnailConvergenceFramesTarget = 4 ///< Due to multi-threaded rendering, this must be at least 4
  };

  plUInt8 m_uiThumbnailConvergenceFrames = 0;
  plUInt16 m_uiThumbnailWidth = 0;
  plUInt16 m_uiThumbnailHeight = 0;
  plEngineProcessViewContext* m_pThumbnailViewContext = nullptr;
  plGALRenderTargets m_ThumbnailRenderTargets;
  plGALTextureHandle m_hThumbnailColorRT;
  plGALTextureHandle m_hThumbnailDepthRT;
  bool m_bWorldSimStateBeforeThumbnail = false;
  plString m_sDocumentType;

  //////////////////////////////////////////////////////////////////////////
  // GameObject reference resolution
private:
  struct GoReferenceTo
  {
    plStringView m_sComponentProperty;
    plUuid m_ReferenceToGameObject;
  };

  struct GoReferencedBy
  {
    plStringView m_sComponentProperty;
    plUuid m_ReferencedByComponent;
  };

  // Components reference GameObjects
  mutable plMap<plUuid, plHybridArray<GoReferenceTo, 4>> m_GoRef_ReferencesTo;

  // GameObjects referenced by Components
  mutable plMap<plUuid, plHybridArray<GoReferencedBy, 4>> m_GoRef_ReferencedBy;

  void WorldRttiConverterContextEventHandler(const plWorldRttiConverterContext::Event& e);
};
