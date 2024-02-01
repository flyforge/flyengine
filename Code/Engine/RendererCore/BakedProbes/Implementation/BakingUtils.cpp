#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingUtils.h>

plVec3 plBakingUtils::FibonacciSphere(plUInt32 uiSampleIndex, plUInt32 uiNumSamples)
{
  float offset = 2.0f / uiNumSamples;
  float increment = plMath::Pi<float>() * (3.0f - plMath::Sqrt(5.0f));

  float y = ((uiSampleIndex * offset) - 1) + (offset / 2);
  float r = plMath::Sqrt(1 - y * y);

  plAngle phi = plAngle::MakeFromRadian(((uiSampleIndex + 1) % uiNumSamples) * increment);

  float x = plMath::Cos(phi) * r;
  float z = plMath::Sin(phi) * r;

  return plVec3(x, y, z);
}

static plUInt32 s_BitsPerDir[plAmbientCubeBasis::NumDirs] = {5, 5, 5, 5, 6, 6};

plCompressedSkyVisibility plBakingUtils::CompressSkyVisibility(const plAmbientCube<float>& skyVisibility)
{
  plCompressedSkyVisibility result = 0;
  plUInt32 uiOffset = 0;
  for (plUInt32 i = 0; i < plAmbientCubeBasis::NumDirs; ++i)
  {
    float maxValue = static_cast<float>((1u << s_BitsPerDir[i]) - 1u);
    plUInt32 compressedDir = static_cast<plUInt8>(plMath::Saturate(skyVisibility.m_Values[i]) * maxValue + 0.5f);
    result |= (compressedDir << uiOffset);
    uiOffset += s_BitsPerDir[i];
  }

  return result;
}

void plBakingUtils::DecompressSkyVisibility(plCompressedSkyVisibility compressedSkyVisibility, plAmbientCube<float>& out_skyVisibility)
{
  plUInt32 uiOffset = 0;
  for (plUInt32 i = 0; i < plAmbientCubeBasis::NumDirs; ++i)
  {
    plUInt32 maxValue = (1u << s_BitsPerDir[i]) - 1u;
    out_skyVisibility.m_Values[i] = static_cast<float>((compressedSkyVisibility >> uiOffset) & maxValue) * (1.0f / maxValue);
    uiOffset += s_BitsPerDir[i];
  }
}


