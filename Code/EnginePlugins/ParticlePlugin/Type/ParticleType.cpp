#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/ParticleType.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeFactory, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleType, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleType* plParticleTypeFactory::CreateType(plParticleSystemInstance* pOwner) const
{
  const plRTTI* pRtti = GetTypeType();

  plParticleType* pType = pRtti->GetAllocator()->Allocate<plParticleType>();
  pType->Reset(pOwner);

  CopyTypeProperties(pType, true);
  pType->CreateRequiredStreams();

  return pType;
}

plParticleType::plParticleType()
{
  m_uiLastExtractedFrame = 0;

  // run these as the last, after all the initializers and behaviors
  m_fPriority = +1000.0f;
}

plUInt32 plParticleType::ComputeSortingKey(plParticleTypeRenderMode::Enum mode, plUInt32 uiTextureHash)
{
  plUInt32 key = 0;

  switch (mode)
  {
    case plParticleTypeRenderMode::Additive:
      key = plParticleTypeSortingKey::Additive;
      break;

    case plParticleTypeRenderMode::Blended:
      key = plParticleTypeSortingKey::Blended;
      break;

    case plParticleTypeRenderMode::BlendedForeground:
      key = plParticleTypeSortingKey::BlendedForeground;
      break;

    case plParticleTypeRenderMode::BlendedBackground:
      key = plParticleTypeSortingKey::BlendedBackground;
      break;

    case plParticleTypeRenderMode::Opaque:
      key = plParticleTypeSortingKey::Opaque;
      break;

    case plParticleTypeRenderMode::BlendAdd:
      key = plParticleTypeSortingKey::BlendAdd;
      break;

    case plParticleTypeRenderMode::Distortion:
      key = plParticleTypeSortingKey::Distortion;
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  key <<= 32 - 3; // require 3 bits for the values above
  key |= uiTextureHash & 0x1FFFFFFFu;

  return key;
}

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_ParticleType);
