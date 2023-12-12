#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Random.h>

// only works when also linking against CoreUtils
//#define USE_PLASMAIMAGE

#ifdef USE_PLASMAIMAGE
#  include <Texture/Image/Image.h>
#endif


PLASMA_CREATE_SIMPLE_TEST(Math, Random)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "UIntInRange")
  {
    plRandom r;
    r.Initialize(0xAABBCCDDEEFF0011ULL);

    for (plUInt32 i = 2; i < 10000; ++i)
    {
      const plUInt32 val = r.UIntInRange(i);
      PLASMA_TEST_BOOL(val < i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IntInRange")
  {
    plRandom r;
    r.Initialize(0xBBCCDDEEFF0011AAULL);

    PLASMA_TEST_INT(r.IntInRange(5, 1), 5);
    PLASMA_TEST_INT(r.IntInRange(-5, 1), -5);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const plInt32 val = r.IntInRange(i, i);
      PLASMA_TEST_BOOL(val >= i);
      PLASMA_TEST_BOOL(val < i + i);
    }

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const plInt32 val = r.IntInRange(-i, 2 * i);
      PLASMA_TEST_BOOL(val >= -i);
      PLASMA_TEST_BOOL(val < -i + 2 * i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IntMinMax")
  {
    plRandom r;
    r.Initialize(0xCCDDEEFF0011AABBULL);

    PLASMA_TEST_INT(r.IntMinMax(5, 5), 5);
    PLASMA_TEST_INT(r.IntMinMax(-5, -5), -5);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const plInt32 val = r.IntMinMax(i, 2 * i);
      PLASMA_TEST_BOOL(val >= i);
      PLASMA_TEST_BOOL(val <= i + i);
    }

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const plInt32 val = r.IntMinMax(-i, i);
      PLASMA_TEST_BOOL(val >= -i);
      PLASMA_TEST_BOOL(val <= i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Bool")
  {
    plRandom r;
    r.Initialize(0x11AABBCCDDEEFFULL);

    plUInt32 falseCount = 0;
    plUInt32 trueCount = 0;
    plDynamicArray<bool> values;
    values.SetCount(1000);

    for (int i = 0; i < 1000; ++i)
    {
      values[i] = r.Bool();
      if (values[i])
      {
        ++trueCount;
      }
      else
      {
        ++falseCount;
      }
    }

    // This could be more elaborate, one could also test the variance
    // and assert that approximately an uniform distribution is yielded
    PLASMA_TEST_BOOL(trueCount > 0 && falseCount > 0);

    plRandom r2;
    r2.Initialize(0x11AABBCCDDEEFFULL);

    for (int i = 0; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(values[i] == r2.Bool());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DoubleZeroToOneExclusive")
  {
    plRandom r;
    r.Initialize(0xDDEEFF0011AABBCCULL);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneExclusive();
      PLASMA_TEST_BOOL(val >= 0.0);
      PLASMA_TEST_BOOL(val < 1.0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DoubleZeroToOneInclusive")
  {
    plRandom r;
    r.Initialize(0xEEFF0011AABBCCDDULL);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneInclusive();
      PLASMA_TEST_BOOL(val >= 0.0);
      PLASMA_TEST_BOOL(val <= 1.0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DoubleInRange")
  {
    plRandom r;
    r.Initialize(0xFF0011AABBCCDDEEULL);

    PLASMA_TEST_DOUBLE(r.DoubleInRange(5, 0), 5, 0.0);
    PLASMA_TEST_DOUBLE(r.DoubleInRange(-5, 0), -5, 0.0);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleInRange(i, i);
      PLASMA_TEST_BOOL(val >= i);
      PLASMA_TEST_BOOL(val < i + i);
    }

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleInRange(-i, 2 * i);
      PLASMA_TEST_BOOL(val >= -i);
      PLASMA_TEST_BOOL(val < -i + 2 * i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DoubleMinMax")
  {
    plRandom r;
    r.Initialize(0x0011AABBCCDDEEFFULL);

    PLASMA_TEST_DOUBLE(r.DoubleMinMax(5, 5), 5, 0.0);
    PLASMA_TEST_DOUBLE(r.DoubleMinMax(-5, -5), -5, 0.0);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(i, 2 * i);
      PLASMA_TEST_BOOL(val >= i);
      PLASMA_TEST_BOOL(val <= i + i);
    }

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(-i, i);
      PLASMA_TEST_BOOL(val >= -i);
      PLASMA_TEST_BOOL(val <= i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FloatZeroToOneExclusive")
  {
    plRandom r;
    r.Initialize(0xDDEEFF0011AABBCCULL);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatZeroToOneExclusive();
      PLASMA_TEST_BOOL(val >= 0.f);
      PLASMA_TEST_BOOL(val < 1.f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FloatZeroToOneInclusive")
  {
    plRandom r;
    r.Initialize(0xEEFF0011AABBCCDDULL);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatZeroToOneInclusive();
      PLASMA_TEST_BOOL(val >= 0.f);
      PLASMA_TEST_BOOL(val <= 1.f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FloatInRange")
  {
    plRandom r;
    r.Initialize(0xFF0011AABBCCDDEEULL);

    PLASMA_TEST_FLOAT(r.FloatInRange(5, 0), 5, 0.f);
    PLASMA_TEST_FLOAT(r.FloatInRange(-5, 0), -5, 0.f);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatInRange(static_cast<float>(i), static_cast<float>(i));
      PLASMA_TEST_BOOL(val >= i);
      PLASMA_TEST_BOOL(val < i + i);
    }

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatInRange(static_cast<float>(-i), 2 * static_cast<float>(i));
      PLASMA_TEST_BOOL(val >= -i);
      PLASMA_TEST_BOOL(val < -i + 2 * i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FloatMinMax")
  {
    plRandom r;
    r.Initialize(0x0011AABBCCDDEEFFULL);

    PLASMA_TEST_FLOAT(r.FloatMinMax(5, 5), 5, 0.f);
    PLASMA_TEST_FLOAT(r.FloatMinMax(-5, -5), -5, 0.f);

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(i), static_cast<float>(2 * i));
      PLASMA_TEST_BOOL(val >= i);
      PLASMA_TEST_BOOL(val <= i + i);
    }

    for (plInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(-i), static_cast<float>(i));
      PLASMA_TEST_BOOL(val >= -i);
      PLASMA_TEST_BOOL(val <= i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Save / Load")
  {
    plRandom r, r2;
    r.Initialize(0x0011AABBCCDDE11FULL);

    for (int i = 0; i < 1000; ++i)
      r.UInt();

    plDefaultMemoryStreamStorage storage;
    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    r.Save(writer);

    plDynamicArray<plUInt32> temp;
    temp.SetCountUninitialized(1000);

    for (int i = 0; i < 1000; ++i)
      temp[i] = r.UInt();

    r2.Load(reader);

    for (int i = 0; i < 1000; ++i)
    {
      PLASMA_TEST_INT(temp[i], r2.UInt());
    }
  }
}

static void SaveToImage(plDynamicArray<plUInt32>& Values, plUInt32 uiMaxValue, const char* szFile)
{
#ifdef USE_PLASMAIMAGE
  PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory("", plFileSystem::AllowWrites, "Clear") == PLASMA_SUCCESS);

  plImage img;
  img.SetWidth(Values.GetCount());
  img.SetHeight(100);
  img.SetImageFormat(plImageFormat::B8G8R8A8_UNORM);
  img.AllocateImageData();

  for (plUInt32 y = 0; y < img.GetHeight(); ++y)
  {
    for (plUInt32 x = 0; x < img.GetWidth(); ++x)
    {
      plUInt32* pPixel = img.GetPixelPointer<plUInt32>(0, 0, 0, x, y);
      *pPixel = 0xFF000000;
    }
  }

  for (plUInt32 i = 0; i < Values.GetCount(); ++i)
  {
    double val = ((double)Values[i] / (double)uiMaxValue) * 100.0;
    plUInt32 y = 99 - plMath::Clamp<plUInt32>((plUInt32)val, 0, 99);

    plUInt32* pPixel = img.GetPixelPointer<plUInt32>(0, 0, 0, i, y);
    *pPixel = 0xFFFFFFFF;
  }

  img.SaveTo(szFile);

  plFileSystem::RemoveDataDirectoryGroup("Clear");
#endif
}

PLASMA_CREATE_SIMPLE_TEST(Math, RandomGauss)
{
  const float fVariance = 1.0f;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "UnsignedValue")
  {
    plRandomGauss r;
    r.Initialize(0xABCDEF0012345678ULL, 100, fVariance);

    plDynamicArray<plUInt32> Values;
    Values.SetCount(100);

    plUInt32 uiMaxValue = 0;

    const plUInt32 factor = 10; // with a factor of 100 the bell curve becomes more pronounced, with less samples it has more exceptions
    for (plUInt32 i = 0; i < 10000 * factor; ++i)
    {
      auto val = r.UnsignedValue();

      PLASMA_TEST_BOOL(val < 100);

      if (val < Values.GetCount())
      {
        Values[val]++;

        uiMaxValue = plMath::Max(uiMaxValue, Values[val]);
      }
    }

    SaveToImage(Values, uiMaxValue, "D:/GaussUnsigned.tga");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SignedValue")
  {
    plRandomGauss r;
    r.Initialize(0xABCDEF0012345678ULL, 100, fVariance);

    plDynamicArray<plUInt32> Values;
    Values.SetCount(2 * 100);

    plUInt32 uiMaxValue = 0;

    const plUInt32 factor = 10; // with a factor of 100 the bell curve becomes more pronounced, with less samples it has more exceptions
    for (plUInt32 i = 0; i < 10000 * factor; ++i)
    {
      auto val = r.SignedValue();

      PLASMA_TEST_BOOL(val > -100 && val < 100);

      val += 100;

      if (val < (plInt32)Values.GetCount())
      {
        Values[val]++;

        uiMaxValue = plMath::Max(uiMaxValue, Values[val]);
      }
    }

    SaveToImage(Values, uiMaxValue, "D:/GaussSigned.tga");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Save / Load")
  {
    plRandomGauss r, r2;
    r.Initialize(0x0011AABBCCDDE11FULL, 1000, 1.7f);

    for (int i = 0; i < 1000; ++i)
      r.UnsignedValue();

    plDefaultMemoryStreamStorage storage;
    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    r.Save(writer);

    plDynamicArray<plUInt32> temp;
    temp.SetCountUninitialized(1000);

    for (int i = 0; i < 1000; ++i)
      temp[i] = r.UnsignedValue();

    r2.Load(reader);

    for (int i = 0; i < 1000; ++i)
    {
      PLASMA_TEST_INT(temp[i], r2.UnsignedValue());
    }
  }
}
