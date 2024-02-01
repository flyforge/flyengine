#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPropertyAnimResource, 1, plRTTIDefaultAllocator<plPropertyAnimResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plPropertyAnimTarget, 1)
PL_ENUM_CONSTANTS(plPropertyAnimTarget::Number, plPropertyAnimTarget::VectorX, plPropertyAnimTarget::VectorY, plPropertyAnimTarget::VectorZ, plPropertyAnimTarget::VectorW)
PL_ENUM_CONSTANTS(plPropertyAnimTarget::RotationX, plPropertyAnimTarget::RotationY, plPropertyAnimTarget::RotationZ, plPropertyAnimTarget::Color)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plPropertyAnimMode, 1)
PL_ENUM_CONSTANTS(plPropertyAnimMode::Once, plPropertyAnimMode::Loop, plPropertyAnimMode::BackAndForth)
PL_END_STATIC_REFLECTED_ENUM;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plPropertyAnimResource);
// clang-format on

plPropertyAnimResource::plPropertyAnimResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plPropertyAnimResource, plPropertyAnimResourceDescriptor)
{
  m_pDescriptor = PL_DEFAULT_NEW(plPropertyAnimResourceDescriptor);
  *m_pDescriptor = descriptor;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

plResourceLoadDesc plPropertyAnimResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  m_pDescriptor = nullptr;

  return res;
}

plResourceLoadDesc plPropertyAnimResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plPropertyAnimResource::UpdateContent", GetResourceIdOrDescription());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = PL_DEFAULT_NEW(plPropertyAnimResourceDescriptor);
  m_pDescriptor->Load(*Stream);

  res.m_State = plResourceState::Loaded;
  return res;
}

void plPropertyAnimResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU = m_pDescriptor->m_FloatAnimations.GetHeapMemoryUsage() + sizeof(plPropertyAnimResourceDescriptor);
  }
}

void plPropertyAnimResourceDescriptor::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 6;
  const plUInt8 uiIdentifier = 0x0A; // dummy to fill the header to 32 Bit
  const plUInt16 uiNumFloatAnimations = static_cast<plUInt16>(m_FloatAnimations.GetCount());
  const plUInt16 uiNumColorAnimations = static_cast<plUInt16>(m_ColorAnimations.GetCount());

  PL_ASSERT_DEV(m_AnimationDuration.GetSeconds() > 0, "Animation duration must be positive");

  inout_stream << uiVersion;
  inout_stream << uiIdentifier;
  inout_stream << m_AnimationDuration;
  inout_stream << uiNumFloatAnimations;

  plCurve1D tmpCurve;

  for (plUInt32 i = 0; i < uiNumFloatAnimations; ++i)
  {
    inout_stream << m_FloatAnimations[i].m_sObjectSearchSequence;
    inout_stream << m_FloatAnimations[i].m_sComponentType;
    inout_stream << m_FloatAnimations[i].m_sPropertyPath;
    inout_stream << m_FloatAnimations[i].m_Target;

    tmpCurve = m_FloatAnimations[i].m_Curve;
    tmpCurve.SortControlPoints();
    tmpCurve.ApplyTangentModes();
    tmpCurve.ClampTangents();
    tmpCurve.Save(inout_stream);
  }

  plColorGradient tmpGradient;
  inout_stream << uiNumColorAnimations;
  for (plUInt32 i = 0; i < uiNumColorAnimations; ++i)
  {
    inout_stream << m_ColorAnimations[i].m_sObjectSearchSequence;
    inout_stream << m_ColorAnimations[i].m_sComponentType;
    inout_stream << m_ColorAnimations[i].m_sPropertyPath;
    inout_stream << m_ColorAnimations[i].m_Target;

    tmpGradient = m_ColorAnimations[i].m_Gradient;
    tmpGradient.SortControlPoints();
    tmpGradient.Save(inout_stream);
  }

  // Version 6
  m_EventTrack.Save(inout_stream);
}

void plPropertyAnimResourceDescriptor::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  plUInt8 uiIdentifier = 0;
  plUInt16 uiNumAnimations = 0;

  inout_stream >> uiVersion;
  inout_stream >> uiIdentifier;

  PL_ASSERT_DEV(uiIdentifier == 0x0A, "File does not contain a valid plPropertyAnimResourceDescriptor");
  PL_ASSERT_DEV(uiVersion == 4 || uiVersion == 5 || uiVersion == 6, "Invalid file version {0}", uiVersion);

  inout_stream >> m_AnimationDuration;

  if (uiVersion == 4)
  {
    plEnum<plPropertyAnimMode> mode;
    inout_stream >> mode;
  }

  inout_stream >> uiNumAnimations;
  m_FloatAnimations.SetCount(uiNumAnimations);

  for (plUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_FloatAnimations[i];

    inout_stream >> anim.m_sObjectSearchSequence;
    inout_stream >> anim.m_sComponentType;
    inout_stream >> anim.m_sPropertyPath;
    inout_stream >> anim.m_Target;
    anim.m_Curve.Load(inout_stream);
    anim.m_Curve.SortControlPoints();
    anim.m_Curve.CreateLinearApproximation();

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = plRTTI::FindTypeByName(anim.m_sComponentType);
  }

  inout_stream >> uiNumAnimations;
  m_ColorAnimations.SetCount(uiNumAnimations);

  for (plUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_ColorAnimations[i];

    inout_stream >> anim.m_sObjectSearchSequence;
    inout_stream >> anim.m_sComponentType;
    inout_stream >> anim.m_sPropertyPath;
    inout_stream >> anim.m_Target;
    anim.m_Gradient.Load(inout_stream);

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = plRTTI::FindTypeByName(anim.m_sComponentType);
  }

  if (uiVersion >= 6)
  {
    m_EventTrack.Load(inout_stream);
  }
}



PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PropertyAnimResource);
