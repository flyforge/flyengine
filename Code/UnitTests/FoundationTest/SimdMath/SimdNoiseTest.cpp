#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Texture/Image/Image.h>

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdNoise)
{
  plStringBuilder sReadDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());
  plStringBuilder sWriteDir = plTestFramework::GetInstance()->GetAbsOutputPath();

  PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sReadDir, "SimdNoise") == PLASMA_SUCCESS);
  PLASMA_TEST_BOOL_MSG(plFileSystem::AddDataDirectory(sWriteDir, "SimdNoise", "output", plFileSystem::AllowWrites) == PLASMA_SUCCESS,
    "Failed to mount data dir '%s'", sWriteDir.GetData());

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Perlin")
  {
    const plUInt32 uiSize = 128;

    plImageHeader imageHeader;
    imageHeader.SetWidth(uiSize);
    imageHeader.SetHeight(uiSize);
    imageHeader.SetImageFormat(plImageFormat::R8G8B8A8_UNORM);

    plImage image;
    image.ResetAndAlloc(imageHeader);

    plSimdPerlinNoise perlin(12345);
    plSimdVec4f xOffset(0, 1, 2, 3);
    plSimdFloat scale(100);

    for (plUInt32 uiNumOctaves = 1; uiNumOctaves <= 6; ++uiNumOctaves)
    {
      plColorLinearUB* data = image.GetPixelPointer<plColorLinearUB>();
      for (plUInt32 y = 0; y < uiSize; ++y)
      {
        for (plUInt32 x = 0; x < uiSize / 4; ++x)
        {
          plSimdVec4f sX = (plSimdVec4f(x * 4.0f) + xOffset) / scale;
          plSimdVec4f sY = plSimdVec4f(y * 1.0f) / scale;

          plSimdVec4f noise = perlin.NoiseZeroToOne(sX, sY, plSimdVec4f::ZeroVector(), uiNumOctaves);
          float p[4];
          p[0] = noise.x();
          p[1] = noise.y();
          p[2] = noise.z();
          p[3] = noise.w();

          plUInt32 uiPixelIndex = y * uiSize + x * 4;
          for (plUInt32 i = 0; i < 4; ++i)
          {
            data[uiPixelIndex + i] = plColor(p[i], p[i], p[i]);
          }
        }
      }

      plStringBuilder sOutFile;
      sOutFile.Format(":output/SimdNoise/result-perlin_{}.tga", uiNumOctaves);

      PLASMA_TEST_BOOL(image.SaveTo(sOutFile).Succeeded());

      plStringBuilder sInFile;
      sInFile.Format("SimdNoise/perlin_{}.tga", uiNumOctaves);
      PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(sInFile), "Noise image file is missing: '%s'", sInFile.GetData());

      PLASMA_TEST_FILES(sOutFile, sInFile, "");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Random")
  {
    plUInt32 histogram[256] = {};

    for (plUInt32 i = 0; i < 10000; ++i)
    {
      plSimdVec4u seed = plSimdVec4u(i);
      plSimdVec4f randomValues = plSimdRandom::FloatMinMax(plSimdVec4i(0, 1, 2, 3), plSimdVec4f::ZeroVector(), plSimdVec4f(256.0f), seed);
      plSimdVec4i randomValuesAsInt = plSimdVec4i::Truncate(randomValues);

      ++histogram[randomValuesAsInt.x()];
      ++histogram[randomValuesAsInt.y()];
      ++histogram[randomValuesAsInt.z()];
      ++histogram[randomValuesAsInt.w()];

      randomValues = plSimdRandom::FloatMinMax(plSimdVec4i(32, 33, 34, 35), plSimdVec4f::ZeroVector(), plSimdVec4f(256.0f), seed);
      randomValuesAsInt = plSimdVec4i::Truncate(randomValues);

      ++histogram[randomValuesAsInt.x()];
      ++histogram[randomValuesAsInt.y()];
      ++histogram[randomValuesAsInt.z()];
      ++histogram[randomValuesAsInt.w()];
    }

    const char* szOutFile = ":output/SimdNoise/result-random.csv";
    {
      plFileWriter fileWriter;
      PLASMA_TEST_BOOL(fileWriter.Open(szOutFile).Succeeded());

      plStringBuilder sLine;
      for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(histogram); ++i)
      {
        sLine.Format("{},\n", histogram[i]);
        fileWriter.WriteBytes(sLine.GetData(), sLine.GetElementCount()).IgnoreResult();
      }
    }

    const char* szInFile = "SimdNoise/random.csv";
    PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(szInFile), "Random histogram file is missing: '%s'", szInFile);

    PLASMA_TEST_TEXT_FILES(szOutFile, szInFile, "");
  }

  plFileSystem::RemoveDataDirectoryGroup("SimdNoise");
}