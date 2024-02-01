#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

class plPhysicsWorldModuleInterface;
class plVolumeCollection;

namespace plProcGenInternal
{
  class PlacementTask final : public plTask
  {
  public:
    PlacementTask(PlacementData* pData, const char* szName);
    ~PlacementTask();

    void Clear();

    plArrayPtr<const PlacementPoint> GetInputPoints() const { return m_InputPoints; }
    plArrayPtr<const PlacementTransform> GetOutputTransforms() const { return m_OutputTransforms; }

  private:
    virtual void Execute() override;

    void FindPlacementPoints();
    void ExecuteVM();

    plProcessingStream MakeInputStream(const plHashedString& sName, plUInt32 uiOffset, plProcessingStream::DataType dataType = plProcessingStream::DataType::Float)
    {
      return plProcessingStream(sName, m_InputPoints.GetByteArrayPtr().GetSubArray(uiOffset), dataType, sizeof(PlacementPoint));
    }

    plProcessingStream MakeOutputStream(const plHashedString& sName, plUInt32 uiOffset, plProcessingStream::DataType dataType = plProcessingStream::DataType::Float)
    {
      return plProcessingStream(sName, m_InputPoints.GetByteArrayPtr().GetSubArray(uiOffset), dataType, sizeof(PlacementPoint));
    }

    PlacementData* m_pData = nullptr;

    plDynamicArray<PlacementPoint, plAlignedAllocatorWrapper> m_InputPoints;
    plDynamicArray<PlacementTransform, plAlignedAllocatorWrapper> m_OutputTransforms;
    plDynamicArray<float> m_Density;
    plDynamicArray<plUInt32> m_ValidPoints;

    plExpressionVM m_VM;
  };
} // namespace plProcGenInternal
