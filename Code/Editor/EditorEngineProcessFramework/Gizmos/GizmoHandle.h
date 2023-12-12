#pragma once

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plWorld;
class plGizmoComponent;
class plGizmo;

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGizmoHandle : public PlasmaEditorEngineSyncObject
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGizmoHandle, PlasmaEditorEngineSyncObject);

public:
  plGizmoHandle();

  plGizmo* GetOwnerGizmo() const { return m_pParentGizmo; }

  void SetVisible(bool bVisible);

  void SetTransformation(const plTransform& m);
  void SetTransformation(const plMat4& m);

  const plTransform& GetTransformation() const { return m_Transformation; }

protected:
  bool m_bVisible = false;
  plTransform m_Transformation;

  void SetParentGizmo(plGizmo* pParentGizmo) { m_pParentGizmo = pParentGizmo; }

private:
  plGizmo* m_pParentGizmo = nullptr;
};


enum PlasmaEngineGizmoHandleType
{
  Arrow,
  Ring,
  Rect,
  LineRect,
  Box,
  Piston,
  HalfPiston,
  Sphere,
  CylinderZ,
  HalfSphereZ,
  BoxCorners,
  BoxEdges,
  BoxFaces,
  LineBox,
  Cone,
  Frustum,
  FromFile,
};

struct plGizmoFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    Default = 0,

    ConstantSize = PLASMA_BIT(0),
    OnTop = PLASMA_BIT(1),
    Visualizer = PLASMA_BIT(2),
    ShowInOrtho = PLASMA_BIT(3),
    Pickable = PLASMA_BIT(4),
    FaceCamera = PLASMA_BIT(5),
  };

  struct Bits
  {
    StorageType ConstantSize : 1;
    StorageType OnTop : 1;
    StorageType Visualizer : 1;
    StorageType ShowInOrtho : 1;
    StorageType Pickable : 1;
    StorageType FaceCamera : 1;
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plGizmoFlags);

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEngineGizmoHandle : public plGizmoHandle
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEngineGizmoHandle, plGizmoHandle);

public:
  PlasmaEngineGizmoHandle();
  ~PlasmaEngineGizmoHandle();

  void ConfigureHandle(plGizmo* pParentGizmo, PlasmaEngineGizmoHandleType type, const plColor& col, plBitflags<plGizmoFlags> flags, const char* szCustomMesh = nullptr);

  virtual bool SetupForEngine(plWorld* pWorld, plUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(plWorld* pWorld) override;

  void SetColor(const plColor& col);

protected:
  bool m_bConstantSize = true;
  bool m_bAlwaysOnTop = false;
  bool m_bVisualizer = false;
  bool m_bShowInOrtho = false;
  bool m_bIsPickable = true;
  bool m_bFaceCamera = false;
  plInt32 m_iHandleType = -1;
  plString m_sGizmoHandleMesh;
  plGameObjectHandle m_hGameObject;
  plGizmoComponent* m_pGizmoComponent = nullptr;
  plColor m_Color = plColor::CornflowerBlue; /* The Original! */
  plWorld* m_pWorld = nullptr;
};
