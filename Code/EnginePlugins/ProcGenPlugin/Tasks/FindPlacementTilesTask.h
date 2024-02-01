#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

enum
{
  EmptyTileIndex = plInvalidIndex,
  NewTileIndex = EmptyTileIndex - 1
};

class plProcPlacementComponent;

namespace plProcGenInternal
{
  class FindPlacementTilesTask final : public plTask
  {
  public:
    FindPlacementTilesTask(plProcPlacementComponent* pComponent, plUInt32 uiOutputIndex);
    ~FindPlacementTilesTask();

    void AddCameraPosition(const plVec3& vCameraPosition) { m_CameraPositions.PushBack(vCameraPosition); }

    plArrayPtr<const PlacementTileDesc> GetNewTiles() const { return m_NewTiles; }
    plArrayPtr<const plUInt64> GetOldTiles() const { return m_OldTileKeys; }

  private:
    virtual void Execute() override;

    plProcPlacementComponent* m_pComponent = nullptr;
    plUInt32 m_uiOutputIndex = 0;

    plHybridArray<plVec3, 2> m_CameraPositions;

    plDynamicArray<PlacementTileDesc, plAlignedAllocatorWrapper> m_NewTiles;
    plDynamicArray<plUInt64> m_OldTileKeys;

    struct TileByAge
    {
      PL_DECLARE_POD_TYPE();

      plUInt64 m_uiTileKey;
      plUInt64 m_uiLastSeenFrame;
    };

    plDynamicArray<TileByAge> m_TilesByAge;
  };

  PL_ALWAYS_INLINE plUInt64 GetTileKey(plInt32 x, plInt32 y)
  {
    plUInt64 sx = (plUInt32)x;
    plUInt64 sy = (plUInt32)y;

    return (sx << 32) | sy;
  }
} // namespace plProcGenInternal
