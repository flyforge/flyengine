#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <GameEngine/Utils/ImageDataResource.h>
#include <GameEngine/Volumes/VolumeSampler.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <Texture/Image/ImageUtils.h>

namespace
{
  PLASMA_FORCE_INLINE float ApplyValue(plProcGenBlendMode::Enum blendMode, float fInitialValue, float fNewValue)
  {
    switch (blendMode)
    {
      case plProcGenBlendMode::Add:
        return fInitialValue + fNewValue;
      case plProcGenBlendMode::Subtract:
        return fInitialValue - fNewValue;
      case plProcGenBlendMode::Multiply:
        return fInitialValue * fNewValue;
      case plProcGenBlendMode::Divide:
        return fInitialValue / fNewValue;
      case plProcGenBlendMode::Max:
        return plMath::Max(fInitialValue, fNewValue);
      case plProcGenBlendMode::Min:
        return plMath::Min(fInitialValue, fNewValue);
      case plProcGenBlendMode::Set:
        return fNewValue;
      default:
        return fInitialValue;
    }
  }
} // namespace

PLASMA_CHECK_AT_COMPILETIME(sizeof(plVolumeCollection::Sphere) == 64);
PLASMA_CHECK_AT_COMPILETIME(sizeof(plVolumeCollection::Box) == 80);

void plVolumeCollection::Shape::SetGlobalToLocalTransform(const plSimdMat4f& t)
{
  plSimdVec4f r0, r1, r2, r3;
  t.GetRows(r0, r1, r2, r3);

  m_GlobalToLocalTransform0 = plSimdConversion::ToVec4(r0);
  m_GlobalToLocalTransform1 = plSimdConversion::ToVec4(r1);
  m_GlobalToLocalTransform2 = plSimdConversion::ToVec4(r2);
}

plSimdMat4f plVolumeCollection::Shape::GetGlobalToLocalTransform() const
{
  plSimdMat4f m;
  m.SetRows(plSimdConversion::ToVec4(m_GlobalToLocalTransform0), plSimdConversion::ToVec4(m_GlobalToLocalTransform1),
    plSimdConversion::ToVec4(m_GlobalToLocalTransform2), plSimdVec4f(0, 0, 0, 1));

  return m;
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plVolumeCollection, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

float plVolumeCollection::EvaluateAtGlobalPosition(const plSimdVec4f& vPosition, float fInitialValue, plProcVolumeImageMode::Enum imgMode, const plColor& refColor) const
{
  float fValue = fInitialValue;

  for (auto pShape : m_SortedShapes)
  {
    if (pShape->m_Type == ShapeType::Sphere)
    {
      auto& sphere = *static_cast<const Sphere*>(pShape);
      const plSimdVec4f localPos = sphere.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const float distSquared = localPos.GetLengthSquared<3>();
      if (distSquared <= 1.0f)
      {
        const float fNewValue = ApplyValue(sphere.m_BlendMode, fValue, sphere.m_fValue);
        const float fAlpha = plMath::Saturate(plMath::Sqrt(distSquared) * sphere.m_fFadeOutScale + sphere.m_fFadeOutBias);
        fValue = plMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Box)
    {
      auto& box = *static_cast<const Box*>(pShape);
      const plSimdVec4f absLocalPos = box.GetGlobalToLocalTransform().TransformPosition(vPosition).Abs();
      if ((absLocalPos <= plSimdVec4f(1.0f)).AllSet<3>())
      {
        const float fNewValue = ApplyValue(box.m_BlendMode, fValue, box.m_fValue);
        plSimdVec4f vAlpha = absLocalPos.CompMul(plSimdConversion::ToVec3(box.m_vFadeOutScale)) + plSimdConversion::ToVec3(box.m_vFadeOutBias);
        vAlpha = vAlpha.CompMin(plSimdVec4f(1.0f)).CompMax(plSimdVec4f::MakeZero());
        const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
        fValue = plMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Image)
    {
      auto& image = *static_cast<const Image*>(pShape);

      const plSimdVec4f localPos = image.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const plSimdVec4f absLocalPos = localPos.Abs();

      if ((absLocalPos <= plSimdVec4f(1.0f)).AllSet<3>() && image.m_pPixelData != nullptr)
      {
        plVec2 uv;
        uv.x = static_cast<float>(localPos.x()) * 0.5f + 0.5f;
        uv.y = static_cast<float>(localPos.y()) * 0.5f + 0.5f;

        const plColor col = plImageUtils::NearestSample(image.m_pPixelData, image.m_uiImageWidth, image.m_uiImageHeight, plImageAddressMode::Clamp, uv);

        float fValueToUse = image.m_fValue;
        PLASMA_IGNORE_UNUSED(fValueToUse);

        switch (imgMode)
        {
          case plProcVolumeImageMode::ReferenceColor:
            fValueToUse = image.m_fValue;
            break;
          case plProcVolumeImageMode::ChannelR:
            fValueToUse = image.m_fValue * col.r;
            break;
          case plProcVolumeImageMode::ChannelG:
            fValueToUse = image.m_fValue * col.g;
            break;
          case plProcVolumeImageMode::ChannelB:
            fValueToUse = image.m_fValue * col.b;
            break;
          case plProcVolumeImageMode::ChannelA:
            fValueToUse = image.m_fValue * col.a;
            break;
        }

        if (imgMode != plProcVolumeImageMode::ReferenceColor || col.IsEqualRGBA(refColor, 0.1f))
        {
          const float fNewValue = ApplyValue(image.m_BlendMode, fValue, fValueToUse);
          plSimdVec4f vAlpha = absLocalPos.CompMul(plSimdConversion::ToVec3(image.m_vFadeOutScale)) + plSimdConversion::ToVec3(image.m_vFadeOutBias);
          vAlpha = vAlpha.CompMin(plSimdVec4f(1.0f)).CompMax(plSimdVec4f::MakeZero());
          const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
          fValue = plMath::Lerp(fValue, fNewValue, fAlpha);
        }
      }
    }
  }

  return fValue;
}

// static
void plVolumeCollection::ExtractVolumesInBox(const plWorld& world, const plBoundingBox& box, plSpatialData::Category spatialCategory,
  const plTagSet& includeTags, plVolumeCollection& out_collection, const plRTTI* pComponentBaseType)
{
  plMsgExtractVolumes msg;
  msg.m_pCollection = &out_collection;

  plSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();
  queryParams.m_IncludeTags = includeTags;

  world.GetSpatialSystem()->FindObjectsInBox(box, queryParams, [&](plGameObject* pObject)
    {
      if (pComponentBaseType != nullptr)
      {
        plHybridArray<const plComponent*, 8> components;
        pObject->TryGetComponentsOfBaseType(pComponentBaseType, components);

        for (auto pComponent : components)
        {
          pComponent->SendMessage(msg);
        }
      }
      else
      {
        pObject->SendMessage(msg);
      }

      return plVisitorExecution::Continue; });

  out_collection.m_Spheres.Sort();
  out_collection.m_Boxes.Sort();
  out_collection.m_Images.Sort();

  const plUInt32 uiNumSpheres = out_collection.m_Spheres.GetCount();
  const plUInt32 uiNumBoxes = out_collection.m_Boxes.GetCount();
  const plUInt32 uiNumImages = out_collection.m_Images.GetCount();

  out_collection.m_SortedShapes.Reserve(uiNumSpheres + uiNumBoxes + uiNumImages);

  plUInt32 uiCurrentSphere = 0;
  plUInt32 uiCurrentBox = 0;
  plUInt32 uiCurrentImage = 0;

  while (uiCurrentSphere < uiNumSpheres || uiCurrentBox < uiNumBoxes || uiCurrentImage < uiNumImages)
  {
    Sphere* pSphere = uiCurrentSphere < uiNumSpheres ? &out_collection.m_Spheres[uiCurrentSphere] : nullptr;
    Box* pBox = uiCurrentBox < uiNumBoxes ? &out_collection.m_Boxes[uiCurrentBox] : nullptr;
    Image* pImage = uiCurrentImage < uiNumImages ? &out_collection.m_Images[uiCurrentImage] : nullptr;

    Shape* pSmallestShape = nullptr;
    plUInt32 uiSmallestKey = 0xFFFFFFFF;

    if (pSphere && pSphere->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pSphere;
      uiSmallestKey = pSmallestShape->m_uiSortingKey;
    }

    if (pBox && pBox->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pBox;
      uiSmallestKey = pSmallestShape->m_uiSortingKey;
    }

    if (pImage && pImage->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pImage;
      uiSmallestKey = pSmallestShape->m_uiSortingKey;
    }

    PLASMA_IGNORE_UNUSED(uiSmallestKey);
    PLASMA_ASSERT_DEBUG(pSmallestShape != nullptr, "Error sorting proc-gen volumes.");

    out_collection.m_SortedShapes.PushBack(pSmallestShape);

    if (pSmallestShape == pSphere)
    {
      ++uiCurrentSphere;
    }
    else if (pSmallestShape == pBox)
    {
      ++uiCurrentBox;
    }
    else if (pSmallestShape == pImage)
    {
      ++uiCurrentImage;
    }
  }
}

void plVolumeCollection::AddSphere(const plSimdTransform& transform, float fRadius, plEnum<plProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff)
{
  plSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale *= fRadius;

  auto& sphere = m_Spheres.ExpandAndGetRef();
  sphere.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  sphere.m_Type = ShapeType::Sphere;
  sphere.m_BlendMode = blendMode;
  sphere.m_fValue = fValue;
  sphere.m_uiSortingKey = plVolumeSampler::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  sphere.m_fFadeOutScale = -1.0f / plMath::Max(fFalloff, 0.0001f);
  sphere.m_fFadeOutBias = -sphere.m_fFadeOutScale;
}

void plVolumeCollection::AddBox(const plSimdTransform& transform, const plVec3& vExtents, plEnum<plProcGenBlendMode> blendMode, float fSortOrder,
  float fValue, const plVec3& vFalloff)
{
  plSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(plSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& box = m_Boxes.ExpandAndGetRef();
  box.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  box.m_Type = ShapeType::Box;
  box.m_BlendMode = blendMode;
  box.m_fValue = fValue;
  box.m_uiSortingKey = plVolumeSampler::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  box.m_vFadeOutScale = plVec3(-1.0f).CompDiv(vFalloff.CompMax(plVec3(0.0001f)));
  box.m_vFadeOutBias = -box.m_vFadeOutScale;
}

void plVolumeCollection::AddImage(const plSimdTransform& transform, const plVec3& vExtents, plEnum<plProcGenBlendMode> blendMode, float fSortOrder, float fValue, const plVec3& vFadeOutStart, const plImageDataResourceHandle& hImage)
{
  plSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(plSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& shape = m_Images.ExpandAndGetRef();
  shape.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  shape.m_Type = ShapeType::Image;
  shape.m_BlendMode = blendMode;
  shape.m_fValue = fValue;
  shape.m_uiSortingKey = plVolumeSampler::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  shape.m_vFadeOutScale = plVec3(-1.0f).CompDiv((plVec3(1.0f) - vFadeOutStart).CompMax(plVec3(0.0001f)));
  shape.m_vFadeOutBias = -shape.m_vFadeOutScale;

  shape.m_Image = hImage;

  if (shape.m_Image.IsValid())
  {
    plResourceLock<plImageDataResource> pImage(shape.m_Image, plResourceAcquireMode::BlockTillLoaded);
    shape.m_pPixelData = pImage->GetDescriptor().m_Image.GetPixelPointer<plColor>();
    shape.m_uiImageWidth = pImage->GetDescriptor().m_Image.GetWidth();
    shape.m_uiImageHeight = pImage->GetDescriptor().m_Image.GetHeight();
  }
}

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgExtractVolumes);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgExtractVolumes, 1, plRTTIDefaultAllocator<plMsgExtractVolumes>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
