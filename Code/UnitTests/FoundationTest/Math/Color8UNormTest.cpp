#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>


PLASMA_CREATE_SIMPLE_TEST(Math, Color8UNorm)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor empty")
  {
    // Placement new of the default constructor should not have any effect on the previous data.
    plUInt8 testBlock[4] = {0, 64, 128, 255};
    plColorLinearUB* pDefCtor = ::new ((void*)&testBlock[0]) plColorLinearUB;
    PLASMA_TEST_BOOL(pDefCtor->r == 0 && pDefCtor->g == 64 && pDefCtor->b == 128 && pDefCtor->a == 255);

    // Make sure the class didn't accidentally change in size
    PLASMA_TEST_BOOL(sizeof(plColorLinearUB) == sizeof(plUInt8) * 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor components")
  {
    plColorLinearUB init3(100, 123, 255);
    PLASMA_TEST_BOOL(init3.r == 100 && init3.g == 123 && init3.b == 255 && init3.a == 255);

    plColorLinearUB init4(100, 123, 255, 42);
    PLASMA_TEST_BOOL(init4.r == 100 && init4.g == 123 && init4.b == 255 && init4.a == 42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor copy")
  {
    plColorLinearUB init4(100, 123, 255, 42);
    plColorLinearUB copy(init4);
    PLASMA_TEST_BOOL(copy.r == 100 && copy.g == 123 && copy.b == 255 && copy.a == 42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor plColor")
  {
    plColorLinearUB fromColor32f(plColor(0.39f, 0.58f, 0.93f));
    PLASMA_TEST_BOOL(plMath::IsEqual<plUInt8>(fromColor32f.r, static_cast<plUInt8>(plColor(0.39f, 0.58f, 0.93f).r * 255), 2) &&
                 plMath::IsEqual<plUInt8>(fromColor32f.g, static_cast<plUInt8>(plColor(0.39f, 0.58f, 0.93f).g * 255), 2) &&
                 plMath::IsEqual<plUInt8>(fromColor32f.b, static_cast<plUInt8>(plColor(0.39f, 0.58f, 0.93f).b * 255), 2) &&
                 plMath::IsEqual<plUInt8>(fromColor32f.a, static_cast<plUInt8>(plColor(0.39f, 0.58f, 0.93f).a * 255), 2));
  }

  // conversion
  {
    plColorLinearUB cornflowerBlue(plColor(0.39f, 0.58f, 0.93f));

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Conversion plColor")
    {
      plColor color32f = cornflowerBlue;
      PLASMA_TEST_BOOL(plMath::IsEqual<float>(color32f.r, plColor(0.39f, 0.58f, 0.93f).r, 2.0f / 255.0f) &&
                   plMath::IsEqual<float>(color32f.g, plColor(0.39f, 0.58f, 0.93f).g, 2.0f / 255.0f) &&
                   plMath::IsEqual<float>(color32f.b, plColor(0.39f, 0.58f, 0.93f).b, 2.0f / 255.0f) &&
                   plMath::IsEqual<float>(color32f.a, plColor(0.39f, 0.58f, 0.93f).a, 2.0f / 255.0f));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Conversion plUInt*")
    {
      const plUInt8* pUIntsConst = cornflowerBlue.GetData();
      PLASMA_TEST_BOOL(pUIntsConst[0] == cornflowerBlue.r && pUIntsConst[1] == cornflowerBlue.g && pUIntsConst[2] == cornflowerBlue.b &&
                   pUIntsConst[3] == cornflowerBlue.a);

      plUInt8* pUInts = cornflowerBlue.GetData();
      PLASMA_TEST_BOOL(pUInts[0] == cornflowerBlue.r && pUInts[1] == cornflowerBlue.g && pUInts[2] == cornflowerBlue.b && pUInts[3] == cornflowerBlue.a);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plColorGammaUB: Constructor")
  {
    plColorGammaUB c(50, 150, 200, 100);
    PLASMA_TEST_INT(c.r, 50);
    PLASMA_TEST_INT(c.g, 150);
    PLASMA_TEST_INT(c.b, 200);
    PLASMA_TEST_INT(c.a, 100);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plColorGammaUB: Constructor (plColor)")
  {
    plColorGammaUB c2 = plColor::RebeccaPurple;

    plColor c3 = c2;

    PLASMA_TEST_BOOL(c3.IsEqualRGBA(plColor::RebeccaPurple, 0.001f));
  }
}
