#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginScene/EnginePluginSceneDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;
class plGameStateBase;
class plGameModeMsgToEngine;
class plWorldSettingsMsgToEngine;
class plObjectsForDebugVisMsgToEngine;
class plGridSettingsMsgToEngine;
class plSimulationSettingsMsgToEngine;
struct plResourceManagerEvent;
class plExposedDocumentObjectPropertiesMsgToEngine;
class plViewRedrawMsgToEngine;
class plWorldWriter;
class plDeferredFileWriter;
class plLayerContext;
struct plGameApplicationExecutionEvent;

class PLASMA_ENGINEPLUGINSCENE_DLL plSceneContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSceneContext, plEngineProcessDocumentContext);

public:
  plSceneContext();
  ~plSceneContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

  const plDeque<plGameObjectHandle>& GetSelection() const { return m_Selection; }
  const plDeque<plGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }
  bool GetRenderSelectionOverlay() const { return m_bRenderSelectionOverlay; }
  bool GetRenderShapeIcons() const { return m_bRenderShapeIcons; }
  bool GetRenderSelectionBoxes() const { return m_bRenderSelectionBoxes; }
  float GetGridDensity() const { return plMath::Abs(m_fGridDensity); }
  bool IsGridInGlobalSpace() const { return m_fGridDensity >= 0.0f; }
  plTransform GetGridTransform() const { return m_GridTransform; }

  plGameStateBase* GetGameState() const;
  bool IsPlayTheGameActive() const { return GetGameState() != nullptr; }

  plUInt32 RegisterLayer(plLayerContext* pLayer);
  void UnregisterLayer(plLayerContext* pLayer);
  void AddLayerIndexTag(const plEntityMsgToEngine& msg, plWorldRttiConverterContext& context, const plTag& layerTag);
  const plArrayPtr<const plTag> GetInvisibleLayerTags() const;

  plEngineProcessDocumentContext* GetActiveDocumentContext();
  const plEngineProcessDocumentContext* GetActiveDocumentContext() const;
  plWorldRttiConverterContext& GetActiveContext();
  const plWorldRttiConverterContext& GetActiveContext() const;
  plWorldRttiConverterContext* GetContextForLayer(const plUuid& layerGuid);
  plArrayPtr<plWorldRttiConverterContext*> GetAllContexts();

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual plStatus ExportDocument(const plExportDocumentMsgToEngine* pMsg) override;
  void ExportExposedParameters(const plWorldWriter& ww, plDeferredFileWriter& file) const;

  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;
  virtual void OnThumbnailViewContextCreated() override;
  virtual void OnDestroyThumbnailViewContext() override;
  virtual void UpdateDocumentContext() override;
  virtual plGameObjectHandle ResolveStringToGameObjectHandle(const void* pString, plComponentHandle hThis, plStringView sProperty) const override;

private:
  struct TagGameObject
  {
    plGameObjectHandle m_hObject;
    plTag m_Tag;
  };

  void AddAmbientLight(bool bSetEditorTag);
  void RemoveAmbientLight();

  void HandleViewRedrawMsg(const plViewRedrawMsgToEngine* pMsg);
  void HandleSelectionMsg(const plObjectSelectionMsgToEngine* pMsg);
  void HandleGameModeMsg(const plGameModeMsgToEngine* pMsg);
  void HandleSimulationSettingsMsg(const plSimulationSettingsMsgToEngine* msg);
  void HandleGridSettingsMsg(const plGridSettingsMsgToEngine* msg);
  void HandleWorldSettingsMsg(const plWorldSettingsMsgToEngine* msg);
  void HandleObjectsForDebugVisMsg(const plObjectsForDebugVisMsgToEngine* pMsg);
  void ComputeHierarchyBounds(plGameObject* pObj, plBoundingBoxSphere& bounds);
  void HandleExposedPropertiesMsg(const plExposedDocumentObjectPropertiesMsgToEngine* pMsg);
  void HandleSceneGeometryMsg(const plExportSceneGeometryMsgToEngine* pMsg);
  void HandlePullObjectStateMsg(const plPullObjectStateMsgToEngine* pMsg);
  void AnswerObjectStatePullRequest(const plViewRedrawMsgToEngine* pMsg);
  void HandleActiveLayerChangedMsg(const plActiveLayerChangedMsgToEngine* pMsg);
  void HandleTagMsgToEngineMsg(const plObjectTagMsgToEngine* pMsg);
  void HandleLayerVisibilityChangedMsgToEngineMsg(const plLayerVisibilityChangedMsgToEngine* pMsg);

  void DrawSelectionBounds(const plViewHandle& hView);

  void UpdateInvisibleLayerTags();
  void InsertSelectedChildren(const plGameObject* pObject);
  void QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg);
  void OnSimulationEnabled();
  void OnSimulationDisabled();
  void OnPlayTheGameModeStarted(const plTransform* pStartPosition);

  void OnResourceManagerEvent(const plResourceManagerEvent& e);
  void GameApplicationEventHandler(const plGameApplicationExecutionEvent& e);

  bool m_bUpdateAllLocalBounds = false;
  bool m_bRenderSelectionOverlay;
  bool m_bRenderShapeIcons;
  bool m_bRenderSelectionBoxes;
  float m_fGridDensity;
  plTransform m_GridTransform;

  plDeque<plGameObjectHandle> m_Selection;
  plDeque<plGameObjectHandle> m_SelectionWithChildren;
  plSet<plGameObjectHandle> m_SelectionWithChildrenSet;
  plGameObjectHandle m_hSkyLight;
  plGameObjectHandle m_hDirectionalLight;
  plDynamicArray<plExposedSceneProperty> m_ExposedSceneProperties;

  plPushObjectStateMsgToEditor m_PushObjectStateMsg;

  plUuid m_ActiveLayer;
  plDynamicArray<plLayerContext*> m_Layers;
  plDynamicArray<plWorldRttiConverterContext*> m_Contexts;

  // We use tags in the form of Layer_4 (Layer_Scene for the scene itself) to not pollute the tag registry with hundreds of unique tags. The tags do not need to be unique across documents so we can just use the layer index but that requires the Tags to be recomputed whenever we remove / add layers.
  // By caching the guids we do not need to send another message each time a layer is loaded as we send also guids of unloaded layers.
  plTag m_LayerTag;
  plHybridArray<plUuid, 1> m_InvisibleLayers;
  bool m_bInvisibleLayersDirty = true;
  plHybridArray<plTag, 1> m_InvisibleLayerTags;

  plDynamicArray<TagGameObject> m_ObjectsToTag;
};
