#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

using plImageDataResourceHandle = plTypedResourceHandle<class plImageDataResource>;

class PL_PROCGENPLUGIN_DLL plVolumeCollection : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plVolumeCollection, plReflectedClass);

public:
  struct ShapeType
  {
    using StorageType = plUInt8;

    enum Enum
    {
      Sphere,
      Box,
      Image,

      Default = Sphere
    };
  };

  struct Shape
  {
    plVec4 m_GlobalToLocalTransform0;
    plVec4 m_GlobalToLocalTransform1;
    plVec4 m_GlobalToLocalTransform2;
    plEnum<ShapeType> m_Type;
    plEnum<plProcGenBlendMode> m_BlendMode;
    plFloat16 m_fValue;
    plUInt32 m_uiSortingKey;

    PL_ALWAYS_INLINE bool operator<(const Shape& other) const { return m_uiSortingKey < other.m_uiSortingKey; }

    void SetGlobalToLocalTransform(const plSimdMat4f& t);
    plSimdMat4f GetGlobalToLocalTransform() const;
  };

  struct Sphere : public Shape
  {
    float m_fFadeOutScale;
    float m_fFadeOutBias;
  };

  struct Box : public Shape
  {
    plVec3 m_vFadeOutScale;
    plVec3 m_vFadeOutBias;
  };

  struct Image : public Box
  {
    plImageDataResourceHandle m_Image;
    const plColor* m_pPixelData = nullptr;
    plUInt32 m_uiImageWidth = 0;
    plUInt32 m_uiImageHeight = 0;
  };

  bool IsEmpty() { return m_Spheres.IsEmpty() && m_Boxes.IsEmpty(); }

  float EvaluateAtGlobalPosition(const plSimdVec4f& vPosition, float fInitialValue, plProcVolumeImageMode::Enum imgMode, const plColor& refColor) const;

  static void ExtractVolumesInBox(const plWorld& world, const plBoundingBox& box, plSpatialData::Category spatialCategory, const plTagSet& includeTags, plVolumeCollection& out_collection, const plRTTI* pComponentBaseType = nullptr);

  void AddSphere(const plSimdTransform& transform, float fRadius, plEnum<plProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFadeOutStart);

  void AddBox(const plSimdTransform& transform, const plVec3& vExtents, plEnum<plProcGenBlendMode> blendMode, float fSortOrder, float fValue, const plVec3& vFadeOutStart);

  void AddImage(const plSimdTransform& transform, const plVec3& vExtents, plEnum<plProcGenBlendMode> blendMode, float fSortOrder, float fValue, const plVec3& vFadeOutStart, const plImageDataResourceHandle& hImage);

private:
  plDynamicArray<Sphere, plAlignedAllocatorWrapper> m_Spheres;
  plDynamicArray<Box, plAlignedAllocatorWrapper> m_Boxes;
  plDynamicArray<Image, plAlignedAllocatorWrapper> m_Images;

  plDynamicArray<const Shape*> m_SortedShapes;
};

struct PL_PROCGENPLUGIN_DLL plMsgExtractVolumes : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgExtractVolumes, plMessage);

  plVolumeCollection* m_pCollection = nullptr;
};
