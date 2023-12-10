#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Declarations.h>

class plPhysicsWorldModuleInterface;

namespace plProcGenInternal
{
  class PlacementTile
  {
  public:
    PlacementTile();
    PlacementTile(PlacementTile&& other);
    ~PlacementTile();

    void Initialize(const PlacementTileDesc& desc, plSharedPtr<const PlacementOutput>& pOutput);
    void Deinitialize(plWorld& world);

    bool IsValid() const;

    const PlacementTileDesc& GetDesc() const;
    const PlacementOutput* GetOutput() const;
    plArrayPtr<const plGameObjectHandle> GetPlacedObjects() const;
    plBoundingBox GetBoundingBox() const;
    plColor GetDebugColor() const;

    void PreparePlacementData(const plWorld* pWorld, const plPhysicsWorldModuleInterface* pPhysicsModule, PlacementData& placementData);

    plUInt32 PlaceObjects(plWorld& world, plArrayPtr<const PlacementTransform> objectTransforms);

  private:
    PlacementTileDesc m_Desc;
    plSharedPtr<const PlacementOutput> m_pOutput;

    struct State
    {
      enum Enum
      {
        Invalid,
        Initialized,
        Scheduled,
        Finished
      };
    };

    State::Enum m_State;
    plDynamicArray<plGameObjectHandle> m_PlacedObjects;
  };
} // namespace plProcGenInternal
