#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <ProcGenPlugin/Declarations.h>

class plPhysicsWorldModuleInterface;
class plVolumeCollection;

namespace plProcGenInternal
{
  struct PlacementData
  {
    PlacementData();
    ~PlacementData();

    void Clear();

    const plPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;
    const plWorld* m_pWorld = nullptr;

    plSharedPtr<const PlacementOutput> m_pOutput;
    plUInt32 m_uiTileSeed = 0;
    plBoundingBox m_TileBoundingBox;

    plDynamicArray<plSimdMat4f, plAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;

    plDeque<plVolumeCollection> m_VolumeCollections;
    plExpression::GlobalData m_GlobalData;
  };
} // namespace plProcGenInternal
