#include <TexConv/TexConvPCH.h>

#include "NoiseGen.h"

plNoiseGen::plNoiseGen()
  : plApplication("NoiseGen")
{
}

#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 plVec2U32(UI0, UI1)
#define UI3 plVec3U32(UI0, UI1, 2798796415U)
#define UIF (1.0f / float(0xffffffffU))

plVec3 hash33(plVec3 p, plUInt32 seed)
{
  plVec3U32 q = plVec3U32(static_cast<uint32_t>(static_cast<int32_t>(p.x)), static_cast<uint32_t>(static_cast<int32_t>(p.y)), static_cast<uint32_t>(static_cast<int32_t>(p.z))).CompMul(UI3);
  q = ((q.x ^ q.y ^ q.z) + seed) * UI3;
  return plVec3(-1.0f) + (plVec3(static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z)) * (UIF * 2.0f));
}

// frac behaves differently than plMath::Fraction
float frac(float x)
{
  return x - floorf(x);
}

plVec3 frac(const plVec3& x)
{
  return plVec3(frac(x.x), frac(x.y), frac(x.z));
}

// mod2 behaves differently than plMath::mod
float mod2(float x, float y)
{
  return x - y * floorf(x / y);
}

plVec3 mod2(const plVec3& x, float y)
{
  return plVec3(mod2(x.x, y), mod2(x.y, y), mod2(x.z, y));
}

plVec3 floor(const plVec3& v)
{
  return plVec3(floorf(v.x), floorf(v.y), floorf(v.z));
}

// Gradient noise by iq (modified to be tileable)
float gradientNoise(plVec3 x, float freq, plUInt32 seed)
{
  return 0;
//  // grid
//  plVec3 p(floor(x));
//  plVec3 w(frac(x));
//
//  // quintic interpolant
//  plVec3 u = w.CompMul(w).CompMul(w).CompMul(w.CompMul(w * 6.0f - plVec3(15.0f)) + plVec3(10.0f));
//
//  // gradients
//  plVec3 ga = hash33((p + plVec3(0.0f, 0.0f, 0.0f)).Mod(freq), seed);
//  plVec3 gb = hash33((p + plVec3(1.0f, 0.0f, 0.0f)).Mod(freq), seed);
//  plVec3 gc = hash33((p + plVec3(0.0f, 1.0f, 0.0f)).Mod(freq), seed);
//  plVec3 gd = hash33((p + plVec3(1.0f, 1.0f, 0.0f)).Mod(freq), seed);
//  plVec3 ge = hash33((p + plVec3(0.0f, 0.0f, 1.0f)).Mod(freq), seed);
//  plVec3 gf = hash33((p + plVec3(1.0f, 0.0f, 1.0f)).Mod(freq), seed);
//  plVec3 gg = hash33((p + plVec3(0.0f, 1.0f, 1.0f)).Mod(freq), seed);
//  plVec3 gh = hash33((p + plVec3(1.0f, 1.0f, 1.0f)).Mod(freq), seed);
//
//  // projections
//  float va = ga.Dot(w - plVec3(0., 0., 0.));
//  float vb = gb.Dot(w - plVec3(1., 0., 0.));
//  float vc = gc.Dot(w - plVec3(0., 1., 0.));
//  float vd = gd.Dot(w - plVec3(1., 1., 0.));
//  float ve = ge.Dot(w - plVec3(0., 0., 1.));
//  float vf = gf.Dot(w - plVec3(1., 0., 1.));
//  float vg = gg.Dot(w - plVec3(0., 1., 1.));
//  float vh = gh.Dot(w - plVec3(1., 1., 1.));
//
//  // interpolation
//  return va +
//         u.x * (vb - va) +
//         u.y * (vc - va) +
//         u.z * (ve - va) +
//         u.x * u.y * (va - vb - vc + vd) +
//         u.y * u.z * (va - vc - ve + vg) +
//         u.z * u.x * (va - vb - ve + vf) +
//         u.x * u.y * u.z * (-va + vb + vc - vd + ve - vf - vg + vh);
}

// Fbm for Perlin noise based on iq's blog
float perlinfbm(plVec3 p, float freq, int octaves, plUInt32 seed)
{
  float G = exp2(-.85);
  float amp = 1.;
  float noise = 0.;
  for (int i = 0; i < octaves; ++i)
  {
    noise += amp * gradientNoise(p * freq, freq, seed);
    freq *= 2.;
    amp *= G;
  }

  return noise;
}


// Tileable 3D worley noise
float worleyNoise(plVec3 uv, float freq, plUInt32 seed)
{
  plVec3 id(floor(uv));
  plVec3 p(frac(uv));

  float minDist = 10000.0f;
  for (float x = -1.f; x <= 1.0f; x += 1.0f)
  {
    for (float y = -1.0f; y <= 1.0f; y += 1.0f)
    {
      for (float z = -1.0f; z <= 1.0f; z += 1.0f)
      {
        plVec3 offset = plVec3(x, y, z);
        plVec3 h = hash33(mod2(id + offset, freq), seed) * 0.5f + plVec3(0.5f);
        h += offset;
        plVec3 d = p - h;
        minDist = plMath::Min(minDist, d.Dot(d));
      }
    }
  }

  // inverted worley noise
  return 1.0f - minDist;
  // return plMath::Mod(id.y + 1.0f, freq) * 0.1f;
}

// Tileable Worley fbm inspired by Andrew Schneider's Real-Time Volumetric Cloudscapes
// chapter in GPU Pro 7.
float worleyFbm(plVec3 p, float freq, plUInt32 seed)
{
  return worleyNoise(p * freq, freq, seed) * 0.625f +
         worleyNoise(p * freq * 2.0f, freq * 2.0f, seed) * 0.25f +
         worleyNoise(p * freq * 4.0f, freq * 4.0f, seed) * 0.125f;
}

float remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
  return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

plResult plNoiseGen::BeforeCoreSystemsStartup()
{
  plStartup::AddApplicationTag("tool");
  plStartup::AddApplicationTag("noisegen");

  return SUPER::BeforeCoreSystemsStartup();
}

void plNoiseGen::AfterCoreSystemsStartup()
{
  plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  // Add the empty data directory to access files via absolute paths
  plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();
}

void plNoiseGen::BeforeCoreSystemsShutdown()
{
  plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

uint8_t floatToUint8(float value)
{
  return static_cast<uint8_t>(plMath::Clamp(value * 255.0f, 0.0f, 255.0f));
}


plApplication::Execution plNoiseGen::Run()
{
  SetReturnCode(-1);

  // Generate cloud base shape noise texture
  {
    plImageHeader header;
    header.SetWidth(128);
    header.SetHeight(128);
    header.SetDepth(128);
    header.SetImageFormat(plImageFormat::R8G8B8A8_UNORM);

    plImage noiseCube;
    noiseCube.ResetAndAlloc(header);

    const float invSize = 1.0f / (128.0f);

    float frequency = 4.0f;

    plUInt32 seed = 0;

    for (uint32_t slice = 0; slice < 128; slice++)
    {
      float uvZ = static_cast<float>(slice) * invSize + (invSize / 2.0);
      for (uint32_t y = 0; y < 128; y++)
      {
        auto curPixel = noiseCube.GetPixelPointer<plColorLinearUB>(0, 0, 0, 0, y, slice);
        float uvY = 1.0f - (static_cast<float>(y) * invSize + (invSize / 2.0));
        for (uint32_t x = 0; x < 128; x++, curPixel++)
        {
          float uvX = static_cast<float>(x) * invSize + (invSize / 2.0);

          const plVec3 uvw(uvX, uvY, uvZ);

          float perlinFbm = plMath::Lerp(1.0f, perlinfbm(uvw, frequency, 7, seed), 0.5f);
          perlinFbm = plMath::Abs(perlinFbm * 2.0f - 1.0f);

          float baseWorley = worleyFbm(uvw, frequency, seed);
          curPixel->g = floatToUint8(baseWorley);
          curPixel->b = floatToUint8(worleyFbm(uvw, frequency * 2.0f, seed));
          curPixel->a = floatToUint8(worleyFbm(uvw, frequency * 4.0f, seed));
          curPixel->r = floatToUint8(remap(perlinFbm, 0.0f, 1.0f, baseWorley, 1.0f));
        }
      }
    }

    plStringBuilder appDir = plOSFile::GetCurrentWorkingDirectory();
    appDir.AppendPath("CloudNoise.dds");

    if (noiseCube.SaveTo(appDir).Failed())
    {
      plLog::Error("Failed to write result to CloudNoise.dds");
    }
  }

 /* {
    plUInt32 dim = 32;

    plImageHeader header;
    header.SetWidth(dim);
    header.SetHeight(dim);
    header.SetDepth(dim);
    header.SetImageFormat(plImageFormat::R8G8B8A8_UNORM);

    plImage noiseCube;
    noiseCube.ResetAndAlloc(header);

    const float invSize = 1.0f / dim;

    float frequency = 3.0f;

    plUInt32 seed = 12345678;

    for (uint32_t slice = 0; slice < dim; slice++)
    {
      float uvZ = static_cast<float>(slice) * invSize + (invSize / 2.0);
      for (uint32_t y = 0; y < dim; y++)
      {
        auto curPixel = noiseCube.GetPixelPointer<plColorLinearUB>(0, 0, 0, 0, y, slice);
        float uvY = 1.0f - (static_cast<float>(y) * invSize + (invSize / 2.0));
        for (uint32_t x = 0; x < dim; x++, curPixel++)
        {
          float uvX = static_cast<float>(x) * invSize + (invSize / 2.0);

          const plVec3 uvw(uvX, uvY, uvZ);

          curPixel->r = floatToUint8(worleyFbm(uvw, frequency, seed));
          curPixel->g = floatToUint8(worleyFbm(uvw, frequency * 2.0f, seed));
          curPixel->b = floatToUint8(worleyFbm(uvw, frequency * 4.0f, seed));
          curPixel->a = 255;
        }
      }
    }

    plStringBuilder appDir = plOSFile::GetCurrentWorkingDirectory();
    appDir.AppendPath("CloudDetailNoise.dds");

    if (noiseCube.SaveTo(appDir).Failed())
    {
      plLog::Error("Failed to write result to CloudNoise.dds");
    }
  }*/

  SetReturnCode(0);
  return plApplication::Execution::Quit;
}

PLASMA_CONSOLEAPP_ENTRY_POINT(plNoiseGen);
