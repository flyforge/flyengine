#pragma once

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plWorld;
class plGizmoComponent;
class plGizmo;

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plGizmoHandle : public plEditorEngineSyncObject
{
  PL_ADD_DYNAMIC_REFLECTION(plGizmoHandle, plEditorEngineSyncObject);

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


enum plEngineGizmoHandleType
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

    ConstantSize = PL_BIT(0),
    OnTop = PL_BIT(1),
    Visualizer = PL_BIT(2),
    ShowInOrtho = PL_BIT(3),
    Pickable = PL_BIT(4),
    FaceCamera = PL_BIT(5),
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

PL_DECLARE_FLAGS_OPERATORS(plGizmoFlags);

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plEngineGizmoHandle : public plGizmoHandle
{
  PL_ADD_DYNAMIC_REFLECTION(plEngineGizmoHandle, plGizmoHandle);

public:
  plEngineGizmoHandle();
  ~plEngineGizmoHandle();

  void ConfigureHandle(plGizmo* pParentGizmo, plEngineGizmoHandleType type, const plColor& col, plBitflags<plGizmoFlags> flags, const char* szCustomMesh = nullptr);

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
