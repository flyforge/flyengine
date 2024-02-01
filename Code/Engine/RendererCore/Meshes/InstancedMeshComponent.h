#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

struct plPerInstanceData;
struct plRenderWorldRenderEvent;
class plInstancedMeshComponent;
struct plMsgExtractGeometry;
class plStreamWriter;
class plStreamReader;

struct PL_RENDERERCORE_DLL plMeshInstanceData
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

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plMeshInstanceData);

//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plInstancedMeshRenderData : public plMeshRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plInstancedMeshRenderData, plMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;

  plInstanceData* m_pExplicitInstanceData = nullptr;
  plUInt32 m_uiExplicitInstanceCount = 0;
};

//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plInstancedMeshComponentManager : public plComponentManager<class plInstancedMeshComponent, plBlockStorageType::Compact>
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

/// \brief Renders multiple instances of the same mesh.
///
/// This is used as an optimization to render many instances of the same (usually small mesh).
/// For example, if you need to render 1000 pieces of grass in a small area,
/// instead of creating 1000 game objects each with a mesh component,
/// it is more efficient to create one game object with an instanced mesh component and give it the locations of the 1000 pieces.
/// Due to the small area, there is no benefit in culling the instances separately.
///
/// However, editing instanced mesh components isn't very convenient, so usually this component would be created and configured
/// in code, rather than by hand in the editor. For example a procedural plant placement system could use this.
class PL_RENDERERCORE_DLL plInstancedMeshComponent : public plMeshComponentBase
{
  PL_DECLARE_COMPONENT_TYPE(plInstancedMeshComponent, plMeshComponentBase, plInstancedMeshComponentManager);

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
