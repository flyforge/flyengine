#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <RendererCore/Meshes/MeshComponent.h>

class PL_PROCGENPLUGIN_DLL plProcVertexColorRenderData : public plMeshRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plProcVertexColorRenderData, plMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;

  plGALBufferHandle m_hVertexColorBuffer;
  plUInt32 m_uiBufferAccessData = 0;
};

//////////////////////////////////////////////////////////////////////////

struct plRenderWorldExtractionEvent;
struct plRenderWorldRenderEvent;
class plProcVertexColorComponent;

class PL_PROCGENPLUGIN_DLL plProcVertexColorComponentManager : public plComponentManager<plProcVertexColorComponent, plBlockStorageType::Compact>
{
  PL_DISALLOW_COPY_AND_ASSIGN(plProcVertexColorComponentManager);

public:
  plProcVertexColorComponentManager(plWorld* pWorld);
  ~plProcVertexColorComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class plProcVertexColorComponent;

  void UpdateVertexColors(const plWorldModule::UpdateContext& context);
  void UpdateComponentVertexColors(plProcVertexColorComponent* pComponent);
  void OnExtractionEvent(const plRenderWorldExtractionEvent& e);
  void OnRenderEvent(const plRenderWorldRenderEvent& e);

  void EnqueueUpdate(plProcVertexColorComponent* pComponent);
  void RemoveComponent(plProcVertexColorComponent* pComponent);

  void OnResourceEvent(const plResourceEvent& resourceEvent);

  void OnAreaInvalidated(const plProcGenInternal::InvalidatedArea& area);

  plDynamicArray<plComponentHandle> m_ComponentsToUpdate;

  plDynamicArray<plSharedPtr<plProcGenInternal::VertexColorTask>> m_UpdateTasks;
  plTaskGroupID m_UpdateTaskGroupID;
  plUInt32 m_uiNextTaskIndex = 0;

  plGALBufferHandle m_hVertexColorBuffer;
  plDynamicArray<plUInt32> m_VertexColorData;
  plUInt32 m_uiCurrentBufferOffset = 0;

  plGAL::ModifiedRange m_ModifiedDataRange;

  struct DataCopy
  {
    plArrayPtr<plUInt32> m_Data;
    plUInt32 m_uiStart = 0;
  };
  DataCopy m_DataCopy[2];
};

//////////////////////////////////////////////////////////////////////////

struct plProcVertexColorOutputDesc
{
  plHashedString m_sName;
  plProcVertexColorMapping m_Mapping;

  void SetName(const char* szName);
  const char* GetName() const { return m_sName; }

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_PROCGENPLUGIN_DLL, plProcVertexColorOutputDesc);

//////////////////////////////////////////////////////////////////////////

struct plMsgTransformChanged;

class PL_PROCGENPLUGIN_DLL plProcVertexColorComponent : public plMeshComponent
{
  PL_DECLARE_COMPONENT_TYPE(plProcVertexColorComponent, plMeshComponent, plProcVertexColorComponentManager);

public:
  plProcVertexColorComponent();
  ~plProcVertexColorComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void SetResource(const plProcGenGraphResourceHandle& hResource);
  const plProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  const plProcVertexColorOutputDesc& GetOutputDesc(plUInt32 uiIndex) const;
  void SetOutputDesc(plUInt32 uiIndex, const plProcVertexColorOutputDesc& outputDesc);

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnTransformChanged(plMsgTransformChanged& ref_msg);

protected:
  virtual plMeshRenderData* CreateRenderData() const override;

private:
  plUInt32 OutputDescs_GetCount() const;
  void OutputDescs_Insert(plUInt32 uiIndex, const plProcVertexColorOutputDesc& outputDesc);
  void OutputDescs_Remove(plUInt32 uiIndex);

  bool HasValidOutputs() const;

  plProcGenGraphResourceHandle m_hResource;
  plHybridArray<plProcVertexColorOutputDesc, 2> m_OutputDescs;

  plHybridArray<plSharedPtr<const plProcGenInternal::VertexColorOutput>, 2> m_Outputs;

  plGALBufferHandle m_hVertexColorBuffer;
  plUInt32 m_uiBufferAccessData = 0;
};
