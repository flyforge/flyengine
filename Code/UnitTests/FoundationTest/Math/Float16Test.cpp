#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Strings/String.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Float16)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "From float and back")
  {
    // default constructor
    PLASMA_TEST_BOOL(static_cast<float>(plFloat16()) == 0.0f);

    // Border cases - exact matching needed.
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(1.0f)), 1.0f, 0);
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(-1.0f)), -1.0f, 0);
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(0.0f)), 0.0f, 0);
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(-0.0f)), -0.0f, 0);
    PLASMA_TEST_BOOL(static_cast<float>(plFloat16(plMath::Infinity<float>())) == plMath::Infinity<float>());
    PLASMA_TEST_BOOL(static_cast<float>(plFloat16(-plMath::Infinity<float>())) == -plMath::Infinity<float>());
    PLASMA_TEST_BOOL(plMath::IsNaN(static_cast<float>(plFloat16(plMath::NaN<float>()))));

    // Some random values.
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(42.0f)), 42.0f, plMath::LargeEpsilon<float>());
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(1.e3f)), 1.e3f, plMath::LargeEpsilon<float>());
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(-1230.0f)), -1230.0f, plMath::LargeEpsilon<float>());
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(plMath::Pi<float>())), plMath::Pi<float>(), plMath::HugeEpsilon<float>());

    // Denormalized float.
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(1.e-40f)), 0.0f, 0);
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(1.e-44f)), 0.0f, 0);

    // Clamping of too large/small values
    // Half only supports 2^-14 to 2^14 (in 10^x this is roughly 4.51) (see Wikipedia)
    PLASMA_TEST_FLOAT(static_cast<float>(plFloat16(1.e-10f)), 0.0f, 0);
    PLASMA_TEST_BOOL(static_cast<float>(plFloat16(1.e5f)) == plMath::Infinity<float>());
    PLASMA_TEST_BOOL(static_cast<float>(plFloat16(-1.e5f)) == -plMath::Infinity<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator ==")
  {
    PLASMA_TEST_BOOL(plFloat16(1.0f) == plFloat16(1.0f));
    PLASMA_TEST_BOOL(plFloat16(10000000.0f) == plFloat16(10000000.0f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator !=")
  {
    PLASMA_TEST_BOOL(plFloat16(1.0f) != plFloat16(-1.0f));
    PLASMA_TEST_BOOL(plFloat16(10000000.0f) != plFloat16(10000.0f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRawData / SetRawData")
  {
    plFloat16 f;
    f.SetRawData(23);

    PLASMA_TEST_INT(f.GetRawData(), 23);
  }
}
