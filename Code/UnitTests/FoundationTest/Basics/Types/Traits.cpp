#include <FoundationTest/FoundationTestPCH.h>

// This test does not actually run, it tests compile time stuff

namespace
{
  struct AggregatePod
  {
    int m_1;
    float m_2;

    PLASMA_DETECT_TYPE_CLASS(int, float);
  };

  struct AggregatePod2
  {
    int m_1;
    float m_2;
    AggregatePod m_3;

    PLASMA_DETECT_TYPE_CLASS(int, float, AggregatePod);
  };

  struct MemRelocateable
  {
    PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();
  };

  struct AggregateMemRelocateable
  {
    int m_1;
    float m_2;
    AggregatePod m_3;
    MemRelocateable m_4;

    PLASMA_DETECT_TYPE_CLASS(int, float, AggregatePod, MemRelocateable);
  };

  class ClassType
  {
  };

  struct AggregateClass
  {
    int m_1;
    float m_2;
    AggregatePod m_3;
    MemRelocateable m_4;
    ClassType m_5;

    PLASMA_DETECT_TYPE_CLASS(int, float, AggregatePod, MemRelocateable, ClassType);
  };

  PLASMA_CHECK_AT_COMPILETIME(plGetTypeClass<AggregatePod>::value == plTypeIsPod::value);
  PLASMA_CHECK_AT_COMPILETIME(plGetTypeClass<AggregatePod2>::value == plTypeIsPod::value);
  PLASMA_CHECK_AT_COMPILETIME(plGetTypeClass<MemRelocateable>::value == plTypeIsMemRelocatable::value);
  PLASMA_CHECK_AT_COMPILETIME(plGetTypeClass<AggregateMemRelocateable>::value == plTypeIsMemRelocatable::value);
  PLASMA_CHECK_AT_COMPILETIME(plGetTypeClass<ClassType>::value == plTypeIsClass::value);
  PLASMA_CHECK_AT_COMPILETIME(plGetTypeClass<AggregateClass>::value == plTypeIsClass::value);
} // namespace
