#include <Core/CorePCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/World.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plWindWorldModuleInterface, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plWindStrength, 1)
  PL_ENUM_CONSTANTS(plWindStrength::Calm, plWindStrength::LightBreple, plWindStrength::GentleBreple, plWindStrength::ModerateBreple, plWindStrength::StrongBreple, plWindStrength::Storm)
  PL_ENUM_CONSTANTS(plWindStrength::WeakShockwave, plWindStrength::MediumShockwave, plWindStrength::StrongShockwave, plWindStrength::ExtremeShockwave)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

float plWindStrength::GetInMetersPerSecond(Enum strength)
{
  // inspired by the Beaufort scale
  // https://en.wikipedia.org/wiki/Beaufort_scale

  switch (strength)
  {
    case Calm:
      return 0.5f;

    case LightBreple:
      return 2.0f;

    case GentleBreple:
      return 5.0f;

    case ModerateBreple:
      return 9.0f;

    case StrongBreple:
      return 14.0f;

    case Storm:
      return 20.0f;

    case WeakShockwave:
      return 40.0f;

    case MediumShockwave:
      return 70.0f;

    case StrongShockwave:
      return 100.0f;

    case ExtremeShockwave:
      return 150.0f;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

plWindWorldModuleInterface::plWindWorldModuleInterface(plWorld* pWorld)
  : plWorldModule(pWorld)
{
}

plVec3 plWindWorldModuleInterface::ComputeWindFlutter(const plVec3& vWind, const plVec3& vObjectDir, float fFlutterSpeed, plUInt32 uiFlutterRandomOffset) const
{
  if (vWind.IsZero(0.001f))
    return plVec3::MakeZero();

  plVec3 windDir = vWind;
  const float fWindStrength = windDir.GetLengthAndNormalize();

  if (fWindStrength <= 0.01f)
    return plVec3::MakeZero();

  plVec3 mainDir = vObjectDir;
  mainDir.NormalizeIfNotZero(plVec3::MakeAxisZ()).IgnoreResult();

  plVec3 flutterDir = windDir.CrossRH(mainDir);
  flutterDir.NormalizeIfNotZero(plVec3::MakeAxisZ()).IgnoreResult();

  const float fFlutterOffset = (uiFlutterRandomOffset & 1023u) / 256.0f;

  const float fFlutter = plMath::Sin(plAngle::MakeFromRadian(fFlutterOffset + fFlutterSpeed * fWindStrength * GetWorld()->GetClock().GetAccumulatedTime().AsFloatInSeconds())) * fWindStrength;

  return flutterDir * fFlutter;
}

PL_STATICLINK_FILE(Core, Core_Interfaces_WindWorldModule);
