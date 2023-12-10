#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

struct plPerInstanceData;
struct plRenderWorldRenderEvent;
class plInstancedMeshComponent;
struct plMsgExtractGeometry;
class plStreamWriter;
class plStreamReader;

struct PLASMA_RENDERERCORE_DLL plMeshInstanceData
{
  void SetLocalPosition(plVec3 vPosition);
  plVec3 GetLocalPosition() const;

  void SetLocalRotation(plQuat qRotation);
  plQuat GetLocalRotation() const;

  void SetLocalScaling(plVec3 vScaling);
  plVec3 GetLocalScaling() const;

  plResult Serialize(plStreamWriter& ref_writer) const;
  plResult Deserialize(plStreamReader& ref_reader);

  plTransform m_transform;

  plColor m_color;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plMeshInstanceData);

//////////////////////////////////////////////////////////////////////////

class PLASMA_RENDERERCORE_DLL plInstancedMeshRenderData : public plMeshRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plInstancedMeshRenderData, plMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;

  plInstanceData* m_pExplicitInstanceData = nullptr;
  plUInt32 m_uiExplicitInstanceCount = 0;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_RENDERERCORE_DLL plInstancedMeshComponentManager : public plComponentManager<class plInstancedMeshComponent, plBlockStorageType::Compact>
{
public:
  using SUPER = plComponentManager<plInstancedMeshComponent, plBlockStorageType::Compact>;

  plInstancedMeshComponentManager(plWorld* pWorld);

  void EnqueueUpdate(const plInstancedMeshComponent* pComponent) const;

private:
  struct ComponentToUpdate
  {
    plComponentHandle m_hComponent;
    plArrayPtr<plPerInstanceData> m_InstanceData;
  };

  mutable plMutex m_Mutex;
  mutable plDeque<ComponentToUpdate> m_RequireUpdate;

protected:
  void OnRenderEvent(const plRenderWorldRenderEvent& e);

  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

class PLASMA_RENDERERCORE_DLL plInstancedMeshComponent : public plMeshComponentBase
{
  PLASMA_DECLARE_COMPONENT_TYPE(plInstancedMeshComponent, plMeshComponentBase, plInstancedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plMeshComponentBase

protected:
  virtual plMeshRenderData* CreateRenderData() const override;


  //////////////////////////////////////////////////////////////////////////
  // plInstancedMeshComponent

public:
  plInstancedMeshComponent();
  ~plInstancedMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(plMsgExtractGeometry& ref_msg); // [ msg handler ]

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  plUInt32 Instances_GetCount() const;                                 // [ property ]
  plMeshInstanceData Instances_GetValue(plUInt32 uiIndex) const;       // [ property ]
  void Instances_SetValue(plUInt32 uiIndex, plMeshInstanceData value); // [ property ]
  void Instances_Insert(plUInt32 uiIndex, plMeshInstanceData value);   // [ property ]
  void Instances_Remove(plUInt32 uiIndex);                             // [ property ]

  plArrayPtr<plPerInstanceData> GetInstanceData() const;

  // Unpacked, reflected instance data for editing and ease of access
  plDynamicArray<plMeshInstanceData> m_RawInstancedData;

  plInstanceData* m_pExplicitInstanceData = nullptr;

  mutable plUInt64 m_uiEnqueuedFrame = plUInt64(-1);
};
