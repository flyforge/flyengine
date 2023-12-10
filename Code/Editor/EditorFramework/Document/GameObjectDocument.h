#pragma once
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class plAssetFileHeader;
class plGameObjectEditTool;
class plGameObjectDocument;

struct PLASMA_EDITORFRAMEWORK_DLL TransformationChanges
{
  enum Enum
  {
    Translation = PLASMA_BIT(0),
    Rotation = PLASMA_BIT(1),
    Scale = PLASMA_BIT(2),
    UniformScale = PLASMA_BIT(3),
    All = 0xFF
  };
};

struct PLASMA_EDITORFRAMEWORK_DLL plGameObjectEvent
{
  enum class Type
  {
    RenderSelectionOverlayChanged,
    RenderVisualizersChanged,
    RenderShapeIconsChanged,
    AddAmbientLightChanged,
    SimulationSpeedChanged,
    PickTransparentChanged,

    ActiveEditToolChanged,

    TriggerShowSelectionInScenegraph,
    TriggerFocusOnSelection_Hovered,
    TriggerFocusOnSelection_All,

    TriggerSnapSelectionPivotToGrid,
    TriggerSnapEachSelectedObjectToGrid,

    GameModeChanged,

    GizmoTransformMayBeInvalid, ///< Sent when a change was made that may affect the current gizmo / manipulator state (ie. objects have been moved)
  };

  Type m_Type;
};

struct plGameObjectDocumentEvent
{
  enum class Type
  {
    GameMode_Stopped,
    GameMode_StartingSimulate,
    GameMode_StartingPlay,
    GameMode_StartingExternal, ///< ie. scene is exported for plPlayer
  };

  Type m_Type;
  plGameObjectDocument* m_pDocument = nullptr;
};

class PLASMA_EDITORFRAMEWORK_DLL plGameObjectMetaData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectMetaData, plReflectedClass);

public:
  enum ModifiedFlags
  {
    CachedName = PLASMA_BIT(2),
    AllFlags = 0xFFFFFFFF
  };

  plGameObjectMetaData() = default;

  plString m_CachedNodeName;
  QIcon m_Icon;
};

struct PLASMA_EDITORFRAMEWORK_DLL plSelectedGameObject
{
  const plDocumentObject* m_pObject;
  plVec3 m_vLocalScaling;
  float m_fLocalUniformScaling;
  plTransform m_GlobalTransform;
};

class PLASMA_EDITORFRAMEWORK_DLL plGameObjectDocument : public plAssetDocument
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectDocument, plAssetDocument);

public:
  plGameObjectDocument(plStringView sDocumentPath, plDocumentObjectManager* pObjectManager,
    plAssetDocEngineConnection engineConnectionType = plAssetDocEngineConnection::FullObjectMirroring);
  ~plGameObjectDocument();


  virtual plEditorInputContext* GetEditorInputContextOverride() override;

protected:
  void SubscribeGameObjectEventHandlers();
  void UnsubscribeGameObjectEventHandlers();

  void GameObjectDocumentEventHandler(const plGameObjectDocumentEvent& e);

  /// \name Gizmo
  ///@{
public:
  /// \brief Makes an edit tool of the given type active. Allocates a new one, if necessary. Only works when SetEditToolConfigDelegate() is set.
  void SetActiveEditTool(const plRTTI* pEditToolType);

  /// \brief Returns the currently active edit tool (nullptr for none).
  plGameObjectEditTool* GetActiveEditTool() const { return m_pActiveEditTool; }

  /// \brief Checks whether an edit tool of the given type, or nullptr for none, is active.
  bool IsActiveEditTool(const plRTTI* pEditToolType) const;

  /// \brief Needs to be called by some higher level code (usually the DocumentWindow) to react to newly created edit tools to configure them (call
  /// plGameObjectEditTool::ConfigureTool()).
  void SetEditToolConfigDelegate(plDelegate<void(plGameObjectEditTool*)> configDelegate);

  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

  void SetGizmoMoveParentOnly(bool bMoveParent);
  bool GetGizmoMoveParentOnly() const;

  /// \brief Finds all objects that are selected at the top level, ie. none of their parents is selected.
  ///
  /// Additionally stores the current transformation. Useful to store this at the start of an operation
  /// to then do modifications on this base transformation every frame.
  void ComputeTopLevelSelectedGameObjects(plDeque<plSelectedGameObject>& out_selection);

  virtual void HandleEngineMessage(const plEditorEngineDocumentMsg* pMsg) override;

private:
  void DeallocateEditTools();

  plDelegate<void(plGameObjectEditTool*)> m_EditToolConfigDelegate;
  plGameObjectEditTool* m_pActiveEditTool = nullptr;
  plMap<const plRTTI*, plGameObjectEditTool*> m_CreatedEditTools;


  ///@}
  /// \name Actions
  ///@{

public:
  void TriggerShowSelectionInScenegraph() const;
  void TriggerFocusOnSelection(bool bAllViews) const;
  void TriggerSnapPivotToGrid() const;
  void TriggerSnapEachObjectToGrid() const;
  /// \brief Moves the editor camera to the same position as the selected object
  void SnapCameraToObject();
  /// \brief Moves the camera to the current picking position
  void MoveCameraHere();

  /// \brief Creates an empty game object at the current picking position
  plStatus CreateGameObjectHere();

  void ScheduleSendObjectSelection();

  /// \brief Sends the current object selection, but only if it was modified or specifically tagged for resending with ScheduleSendObjectSelection().
  void SendObjectSelection();

  ///@}
  /// \name Settings
  ///@{
public:
  bool GetAddAmbientLight() const { return m_bAddAmbientLight; }
  void SetAddAmbientLight(bool b);

  float GetSimulationSpeed() const { return m_fSimulationSpeed; }
  void SetSimulationSpeed(float f);

  bool GetRenderSelectionOverlay() const { return m_CurrentMode.m_bRenderSelectionOverlay; }
  void SetRenderSelectionOverlay(bool b);

  bool GetRenderVisualizers() const { return m_CurrentMode.m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  bool GetRenderShapeIcons() const { return m_CurrentMode.m_bRenderShapeIcons; }
  void SetRenderShapeIcons(bool b);

  bool GetPickTransparent() const { return m_bPickTransparent; }
  void SetPickTransparent(bool b);

  ///@}
  /// \name Transform
  ///@{

  /// \brief Sets the new global transformation of the given object.
  /// The transformationChanges bitmask (of type TransformationChanges) allows to tell the system that, e.g. only translation has changed and thus
  /// some work can be spared.
  void SetGlobalTransform(const plDocumentObject* pObject, const plTransform& t, plUInt8 uiTransformationChanges) const;

  /// \brief Same as SetGlobalTransform, except that all children will keep their current global transform (thus their local transforms are adjusted)
  void SetGlobalTransformParentOnly(const plDocumentObject* pObject, const plTransform& t, plUInt8 uiTransformationChanges) const;

  /// \brief Returns a cached value for the global transform of the given object, if available. Otherwise it calls ComputeGlobalTransform().
  plTransform GetGlobalTransform(const plDocumentObject* pObject) const;

  /// \brief Retrieves the local transform property values from the object and combines it into one plTransform
  static plTransform QueryLocalTransform(const plDocumentObject* pObject);
  static plSimdTransform QueryLocalTransformSimd(const plDocumentObject* pObject);

  /// \brief Computes the global transform of the parent and combines it with the local transform of the given object.
  /// This function does not return a cached value, but always computes it. It does update the internal cache for later reads though.
  plTransform ComputeGlobalTransform(const plDocumentObject* pObject) const;

  /// \brief Traverses the pObject hierarchy up until it hits an plGameObject, then computes the global transform of that.
  virtual plResult ComputeObjectTransformation(const plDocumentObject* pObject, plTransform& out_result) const override;

  ///@}
  /// \name Node Names
  ///@{

  /// \brief Generates a good name for pObject. Queries the "Name" property, child components and asset properties, if necessary.
  void DetermineNodeName(const plDocumentObject* pObject, const plUuid& prefabGuid, plStringBuilder& out_sResult, QIcon* out_pIcon = nullptr) const;

  /// \brief Similar to DetermineNodeName() but prefers to return the last cached value from scene meta data. This is more efficient, but may give an
  /// outdated result.
  void QueryCachedNodeName(
    const plDocumentObject* pObject, plStringBuilder& out_sResult, plUuid* out_pPrefabGuid = nullptr, QIcon* out_pIcon = nullptr) const;

  /// \brief Creates a full "path" to a scene object for display in UIs. No guarantee for uniqueness.
  void GenerateFullDisplayName(const plDocumentObject* pRoot, plStringBuilder& out_sFullPath) const;

  ///@}

public:
  mutable plEvent<const plGameObjectEvent&> m_GameObjectEvents;
  mutable plUniquePtr<plObjectMetaData<plUuid, plGameObjectMetaData>> m_GameObjectMetaData;

  static plEvent<const plGameObjectDocumentEvent&> s_GameObjectDocumentEvents;

protected:
  void InvalidateGlobalTransformValue(const plDocumentObject* pObject) const;
  /// \brief Sends the current state of the scene to the engine process. This is typically done after scene load or when the world might have deviated
  /// on the engine side (after play the game etc.)
  virtual void SendGameWorldToEngine();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable) override;

public:
  void SelectionManagerEventHandler(const plSelectionManagerEvent& e);
  void ObjectPropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void ObjectStructureEventHandler(const plDocumentObjectStructureEvent& e);
  void ObjectEventHandler(const plDocumentObjectEvent& e);

protected:
  struct PLASMA_EDITORFRAMEWORK_DLL GameModeData
  {
    bool m_bRenderSelectionOverlay;
    bool m_bRenderVisualizers;
    bool m_bRenderShapeIcons;
  };
  GameModeData m_CurrentMode;

private:
  bool m_bAddAmbientLight = false;
  bool m_bGizmoWorldSpace = true; // whether the gizmo is in local/global space mode
  bool m_bGizmoMoveParentOnly = false;
  bool m_bPickTransparent = true;

  float m_fSimulationSpeed = 1.0f;

  using TransformTable = plHashTable<const plDocumentObject*, plSimdTransform, plHashHelper<const plDocumentObject*>, plAlignedAllocatorWrapper>;
  mutable TransformTable m_GlobalTransforms;

  // when new objects are created the engine sometimes needs to catch up creating sub-objects (e.g. for reference prefabs)
  // therefore when the selection is changed in the first frame, it might not be fully correct
  // by sending it a second time, we can fix that easily
  plInt8 m_iResendSelection = 0;

protected:
  plEventSubscriptionID m_SelectionManagerEventHandlerID;
  plEventSubscriptionID m_ObjectPropertyEventHandlerID;
  plEventSubscriptionID m_ObjectStructureEventHandlerID;
  plEventSubscriptionID m_ObjectEventHandlerID;
};
