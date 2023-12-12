#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <ProcGenPlugin/ProcGenPluginDLL.h>

class plExpressionByteCode;
using plColorGradientResourceHandle = plTypedResourceHandle<class plColorGradientResource>;
using plPrefabResourceHandle = plTypedResourceHandle<class plPrefabResource>;
using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;

struct plProcGenBinaryOperator
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,

    Default = Multiply
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PROCGENPLUGIN_DLL, plProcGenBinaryOperator);

struct plProcGenBlendMode
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,
    Set,

    Default = Multiply
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PROCGENPLUGIN_DLL, plProcGenBlendMode);

struct plProcVertexColorChannelMapping
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    R,
    G,
    B,
    A,
    Black,
    White,

    Default = R
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PROCGENPLUGIN_DLL, plProcVertexColorChannelMapping);

struct plProcVertexColorMapping
{
  plEnum<plProcVertexColorChannelMapping> m_R = plProcVertexColorChannelMapping::R;
  plEnum<plProcVertexColorChannelMapping> m_G = plProcVertexColorChannelMapping::G;
  plEnum<plProcVertexColorChannelMapping> m_B = plProcVertexColorChannelMapping::B;
  plEnum<plProcVertexColorChannelMapping> m_A = plProcVertexColorChannelMapping::A;

  plResult Serialize(plStreamWriter& stream) const;
  plResult Deserialize(plStreamReader& stream);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PROCGENPLUGIN_DLL, plProcVertexColorMapping);

struct plProcPlacementMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Raycast,
    Fixed,

    Default = Raycast
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PROCGENPLUGIN_DLL, plProcPlacementMode);

struct plProcVolumeImageMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    ReferenceColor,
    ChannelR,
    ChannelG,
    ChannelB,
    ChannelA,

    Default = ReferenceColor
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PROCGENPLUGIN_DLL, plProcVolumeImageMode);

//////////////////////////////////////////////////////////////////////////

namespace plProcGenInternal
{
  class PlacementTile;
  class FindPlacementTilesTask;
  class PreparePlacementTask;
  class PlacementTask;
  class VertexColorTask;
  struct PlacementData;

  struct InvalidatedArea
  {
    plBoundingBox m_Box;
    plWorld* m_pWorld = nullptr;
  };

  struct Pattern
  {
    struct Point
    {
      plVec2 m_Coordinates;
      float m_fThreshold;
    };

    plArrayPtr<Point> m_Points;
    float m_fSize;
  };

  struct PLASMA_PROCGENPLUGIN_DLL GraphSharedDataBase : public plRefCounted
  {
    virtual ~GraphSharedDataBase();
  };

  struct Output : public plRefCounted
  {
    virtual ~Output();

    plHashedString m_sName;

    plHybridArray<plUInt8, 4> m_VolumeTagSetIndices;
    plSharedPtr<const GraphSharedDataBase> m_pGraphSharedData;

    plUniquePtr<plExpressionByteCode> m_pByteCode;
  };

  struct PlacementOutput : public Output
  {
    float GetTileSize() const { return m_pPattern->m_fSize * m_fFootprint; }

    bool IsValid() const
    {
      return !m_ObjectsToPlace.IsEmpty() && m_pPattern != nullptr && m_fFootprint > 0.0f && m_fCullDistance > 0.0f && m_pByteCode != nullptr;
    }

    plHybridArray<plPrefabResourceHandle, 4> m_ObjectsToPlace;

    const Pattern* m_pPattern = nullptr;
    float m_fFootprint = 1.0f;

    plVec3 m_vMinOffset = plVec3::ZeroVector();
    plVec3 m_vMaxOffset = plVec3::ZeroVector();

    plAngle m_YawRotationSnap = plAngle::Radian(0.0f);
    float m_fAlignToNormal = 1.0f;

    plVec3 m_vMinScale = plVec3(1.0f);
    plVec3 m_vMaxScale = plVec3(1.0f);

    float m_fCullDistance = 30.0f;

    plUInt32 m_uiCollisionLayer = 0;

    plColorGradientResourceHandle m_hColorGradient;

    plSurfaceResourceHandle m_hSurface;

    plEnum<plProcPlacementMode> m_Mode;
  };

  struct VertexColorOutput : public Output
  {
  };

  struct PLASMA_PROCGENPLUGIN_DLL ExpressionInputs
  {
    static plHashedString s_sPosition;
    static plHashedString s_sPositionX;
    static plHashedString s_sPositionY;
    static plHashedString s_sPositionZ;
    static plHashedString s_sNormal;
    static plHashedString s_sNormalX;
    static plHashedString s_sNormalY;
    static plHashedString s_sNormalZ;
    static plHashedString s_sColor;
    static plHashedString s_sColorR;
    static plHashedString s_sColorG;
    static plHashedString s_sColorB;
    static plHashedString s_sColorA;
    static plHashedString s_sPointIndex;
  };

  struct PLASMA_PROCGENPLUGIN_DLL ExpressionOutputs
  {
    static plHashedString s_sOutDensity;
    static plHashedString s_sOutScale;
    static plHashedString s_sOutColorIndex;
    static plHashedString s_sOutObjectIndex;

    static plHashedString s_sOutColor;
    static plHashedString s_sOutColorR;
    static plHashedString s_sOutColorG;
    static plHashedString s_sOutColorB;
    static plHashedString s_sOutColorA;
  };

  struct PlacementPoint
  {
    PLASMA_DECLARE_POD_TYPE();

    plVec3 m_vPosition;
    float m_fScale;
    plVec3 m_vNormal;
    plUInt8 m_uiColorIndex;
    plUInt8 m_uiObjectIndex;
    plUInt16 m_uiPointIndex;
  };

  struct PlacementTransform
  {
    PLASMA_DECLARE_POD_TYPE();

    plSimdTransform m_Transform;
    plColorLinear16f m_ObjectColor;
    plUInt16 m_uiPointIndex;
    plUInt8 m_uiObjectIndex;
    bool m_bHasValidColor;
    plUInt32 m_uiPadding;
  };

  struct PlacementTileDesc
  {
    plComponentHandle m_hComponent;
    plUInt32 m_uiOutputIndex;
    plInt32 m_iPosX;
    plInt32 m_iPosY;
    float m_fMinZ;
    float m_fMaxZ;
    float m_fTileSize;
    float m_fDistanceToCamera;

    bool operator==(const PlacementTileDesc& other) const
    {
      return m_hComponent == other.m_hComponent && m_uiOutputIndex == other.m_uiOutputIndex && m_iPosX == other.m_iPosX && m_iPosY == other.m_iPosY;
    }

    plBoundingBox GetBoundingBox() const
    {
      plVec2 vCenter = plVec2(m_iPosX * m_fTileSize, m_iPosY * m_fTileSize);
      plVec3 vMin = (vCenter - plVec2(m_fTileSize * 0.5f)).GetAsVec3(m_fMinZ);
      plVec3 vMax = (vCenter + plVec2(m_fTileSize * 0.5f)).GetAsVec3(m_fMaxZ);

      return plBoundingBox(vMin, vMax);
    }

    plHybridArray<plSimdMat4f, 8, plAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;
  };
} // namespace plProcGenInternal