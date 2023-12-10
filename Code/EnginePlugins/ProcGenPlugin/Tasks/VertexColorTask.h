#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

struct plMeshBufferResourceDescriptor;
class plVolumeCollection;

namespace plProcGenInternal
{
  class VertexColorTask final : public plTask
  {
  public:
    VertexColorTask();
    ~VertexColorTask();

    void Prepare(const plWorld& world, const plMeshBufferResourceDescriptor& mbDesc, const plTransform& transform,
      plArrayPtr<plSharedPtr<const VertexColorOutput>> outputs, plArrayPtr<plProcVertexColorMapping> outputMappings,
      plArrayPtr<plUInt32> outputVertexColors);

  private:
    virtual void Execute() override;

    plHybridArray<plSharedPtr<const VertexColorOutput>, 2> m_Outputs;
    plHybridArray<plProcVertexColorMapping, 2> m_OutputMappings;

    struct InputVertex
    {
      PLASMA_DECLARE_POD_TYPE();

      plVec3 m_vPosition;
      plVec3 m_vNormal;
      plColor m_Color;
      plUInt32 m_uiIndex;
    };

    plDynamicArray<InputVertex> m_InputVertices;

    plDynamicArray<plColor> m_TempData;
    plArrayPtr<plUInt32> m_OutputVertexColors;

    plDeque<plVolumeCollection> m_VolumeCollections;
    plExpression::GlobalData m_GlobalData;

    plExpressionVM m_VM;
  };
} // namespace plProcGenInternal