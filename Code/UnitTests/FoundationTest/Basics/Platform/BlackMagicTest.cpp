#include <FoundationTest/FoundationTestPCH.h>

#define INT_DECLARE(name, n) int name = n;

namespace
{
  PLASMA_EXPAND_ARGS_WITH_INDEX(INT_DECLARE, heinz, klaus);
}

PLASMA_CREATE_SIMPLE_TEST(Basics, BlackMagic)
{
  PLASMA_TEST_INT(heinz, 0);
  PLASMA_TEST_INT(klaus, 1);
}
