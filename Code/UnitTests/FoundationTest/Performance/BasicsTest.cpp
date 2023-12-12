#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

/* Performance Statistics:

  AMD E-350 Processor 1.6 GHz ('Fusion'), 32 Bit, Debug Mode
    Virtual Function Calls:   ~60 ns
    Simple Function Calls:    ~27 ns
    Fastcall Function Calls:  ~27 ns
    Integer Division:         52 ns
    Integer Multiplication:   23 ns
    Float Division:           25 ns
    Float Multiplication:     25 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 64 Bit, Debug Mode
    Virtual Function Calls:   ~80 ns
    Simple Function Calls:    ~55 ns
    Fastcall Function Calls:  ~55 ns
    Integer Division:         ~97 ns
    Integer Multiplication:   ~52 ns
    Float Division:           ~66 ns
    Float Multiplication:     ~58 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 32 Bit, Release Mode
    Virtual Function Calls:   ~9 ns
    Simple Function Calls:    ~5 ns
    Fastcall Function Calls:  ~5 ns
    Integer Division:         35 ns
    Integer Multiplication:   3.78 ns
    Float Division:           10.7 ns
    Float Multiplication:     9.5 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 64 Bit, Release Mode
    Virtual Function Calls:   ~10 ns
    Simple Function Calls:    ~5 ns
    Fastcall Function Calls:  ~5 ns
    Integer Division:         35 ns
    Integer Multiplication:   3.23 ns
    Float Division:           8.13 ns
    Float Multiplication:     4.13 ns

  Intel Core i7 3770 3.4 GHz, 64 Bit, Release Mode
    Virtual Function Calls:   ~3.8 ns
    Simple Function Calls:    ~4.4 ns
    Fastcall Function Calls:  ~4.0 ns
    Integer Division:         8.25 ns
    Integer Multiplication:   1.55 ns
    Float Division:           4.40 ns
    Float Multiplication:     1.87 ns

*/

PLASMA_CREATE_SIMPLE_TEST_GROUP(Performance);

struct plMsgTest : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgTest, plMessage);
};

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgTest);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTest, 1, plRTTIDefaultAllocator<plMsgTest>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


struct GetValueMessage : public plMsgTest
{
  PLASMA_DECLARE_MESSAGE_TYPE(GetValueMessage, plMsgTest);

  plInt32 m_iValue;
};
PLASMA_IMPLEMENT_MESSAGE_TYPE(GetValueMessage);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(GetValueMessage, 1, plRTTIDefaultAllocator<GetValueMessage>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;



class Base : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(Base, plReflectedClass);

public:
  virtual ~Base() = default;

  virtual plInt32 Virtual() = 0;
};

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(Base, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  define PLASMA_FASTCALL __fastcall
#  define PLASMA_NO_INLINE __declspec(noinline)
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  if PLASMA_ENABLED(PLASMA_PLATFORM_ARCH_X86) && PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
#    define PLASMA_FASTCALL __attribute((fastcall)) // Fastcall only relevant on x86-32 and would otherwise generate warnings
#  else
#    define PLASMA_FASTCALL
#  endif
#  define PLASMA_NO_INLINE __attribute__((noinline))
#else
#  warning Unknown Platform.
#  define PLASMA_FASTCALL
#  define PLASMA_NO_INLINE __attribute__((noinline)) /* should work on GCC */
#endif

class Derived1 : public Base
{
  PLASMA_ADD_DYNAMIC_REFLECTION(Derived1, Base);

public:
  PLASMA_NO_INLINE plInt32 PLASMA_FASTCALL FastCall() { return 1; }
  PLASMA_NO_INLINE plInt32 NonVirtual() { return 1; }
  PLASMA_NO_INLINE virtual plInt32 Virtual() override { return 1; }
  PLASMA_NO_INLINE void OnGetValueMessage(GetValueMessage& ref_msg) { ref_msg.m_iValue = 1; }
};

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(Derived1, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(GetValueMessage, OnGetValueMessage),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class Derived2 : public Base
{
  PLASMA_ADD_DYNAMIC_REFLECTION(Derived2, Base);

public:
  PLASMA_NO_INLINE plInt32 PLASMA_FASTCALL FastCall() { return 2; }
  PLASMA_NO_INLINE plInt32 NonVirtual() { return 2; }
  PLASMA_NO_INLINE virtual plInt32 Virtual() override { return 2; }
  PLASMA_NO_INLINE void OnGetValueMessage(GetValueMessage& ref_msg) { ref_msg.m_iValue = 2; }
};

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(Derived2, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(GetValueMessage, OnGetValueMessage),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PLASMA_CREATE_SIMPLE_TEST(Performance, Basics)
{
  const plInt32 iNumObjects = 1000000;
  const float fNumObjects = (float)iNumObjects;

  plDynamicArray<Derived1> Der1;
  Der1.SetCount(iNumObjects / 2);

  plDynamicArray<Derived2> Der2;
  Der2.SetCount(iNumObjects / 2);

  plDynamicArray<Base*> Objects;
  Objects.SetCount(iNumObjects);

  for (plInt32 i = 0; i < iNumObjects; i += 2)
  {
    Objects[i] = &Der1[i / 2];
    Objects[i + 1] = &Der2[i / 2];
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Dispatch Message")
  {
    plInt32 iResult = 0;

    // warm up
    for (plUInt32 i = 0; i < iNumObjects; ++i)
    {
      GetValueMessage msg;
      Objects[i]->GetDynamicRTTI()->DispatchMessage(Objects[i], msg);
      iResult += msg.m_iValue;
    }

    plTime t0 = plTime::Now();

    for (plUInt32 i = 0; i < iNumObjects; ++i)
    {
      GetValueMessage msg;
      Objects[i]->GetDynamicRTTI()->DispatchMessage(Objects[i], msg);
      iResult += msg.m_iValue;
    }

    plTime t1 = plTime::Now();

    PLASMA_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    plTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double)iNumObjects;

    plLog::Info("[test]Dispatch Message: {0}ns", plArgF(tFC, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Virtual")
  {
    plInt32 iResult = 0;

    // warm up
    for (plUInt32 i = 0; i < iNumObjects; ++i)
      iResult += Objects[i]->Virtual();

    plTime t0 = plTime::Now();

    for (plUInt32 i = 0; i < iNumObjects; ++i)
      iResult += Objects[i]->Virtual();

    plTime t1 = plTime::Now();

    PLASMA_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    plTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double)iNumObjects;

    plLog::Info("[test]Virtual Function Calls: {0}ns", plArgF(tFC, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "NonVirtual")
  {
    plInt32 iResult = 0;

    // warm up
    for (plUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->NonVirtual();
      iResult += ((Derived2*)Objects[i])->NonVirtual();
    }

    plTime t0 = plTime::Now();

    for (plUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->NonVirtual();
      iResult += ((Derived2*)Objects[i])->NonVirtual();
    }

    plTime t1 = plTime::Now();

    PLASMA_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    plTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double)iNumObjects;

    plLog::Info("[test]Non-Virtual Function Calls: {0}ns", plArgF(tFC, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FastCall")
  {
    plInt32 iResult = 0;

    // warm up
    for (plUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->FastCall();
      iResult += ((Derived2*)Objects[i])->FastCall();
    }

    plTime t0 = plTime::Now();

    for (plUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->FastCall();
      iResult += ((Derived2*)Objects[i])->FastCall();
    }

    plTime t1 = plTime::Now();

    PLASMA_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    plTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double)iNumObjects;

    plLog::Info("[test]FastCall Function Calls: {0}ns", plArgF(tFC, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "32 Bit Integer Division")
  {
    plDynamicArray<plInt32> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (plInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100;

    plTime t0 = plTime::Now();

    plInt32 iResult = 0;

    for (plInt32 i = 1; i < iNumObjects; i += 1)
      iResult += Ints[i] / i;

    plTime t1 = plTime::Now();

    plTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects - 1);

    plLog::Info("[test]32 Bit Integer Division: {0}ns", plArgF(t, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "32 Bit Integer Multiplication")
  {
    plDynamicArray<plInt32> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (plInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = iNumObjects - i;

    plTime t0 = plTime::Now();

    plInt32 iResult = 0;

    for (plInt32 i = 0; i < iNumObjects; i += 1)
      iResult += Ints[i] * i;

    plTime t1 = plTime::Now();

    plTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    plLog::Info("[test]32 Bit Integer Multiplication: {0}ns", plArgF(t, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "64 Bit Integer Division")
  {
    plDynamicArray<plInt64> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (plInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = (plInt64)i * (plInt64)100;

    plTime t0 = plTime::Now();

    plInt64 iResult = 0;

    for (plInt32 i = 1; i < iNumObjects; i += 1)
      iResult += Ints[i] / (plInt64)i;

    plTime t1 = plTime::Now();

    plTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects - 1);

    plLog::Info("[test]64 Bit Integer Division: {0}ns", plArgF(t, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "64 Bit Integer Multiplication")
  {
    plDynamicArray<plInt64> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (plInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = iNumObjects - i;

    plTime t0 = plTime::Now();

    plInt64 iResult = 0;

    for (plInt32 i = 0; i < iNumObjects; i += 1)
      iResult += Ints[i] * (plInt64)i;

    plTime t1 = plTime::Now();

    plTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    plLog::Info("[test]64 Bit Integer Multiplication: {0}ns", plArgF(t, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "32 Bit Float Division")
  {
    plDynamicArray<float> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (plInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100.0f;

    plTime t0 = plTime::Now();

    float fResult = 0;

    float d = 1.0f;
    for (plInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      fResult += Ints[i] / d;

    plTime t1 = plTime::Now();

    plTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    plLog::Info("[test]32 Bit Float Division: {0}ns", plArgF(t, 2), fResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "32 Bit Float Multiplication")
  {
    plDynamicArray<float> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (plInt32 i = 0; i < iNumObjects; i++)
      Ints[i] = (float)(fNumObjects) - (float)(i);

    plTime t0 = plTime::Now();

    float iResult = 0;

    float d = 1.0f;
    for (plInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      iResult += Ints[i] * d;

    plTime t1 = plTime::Now();

    plTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    plLog::Info("[test]32 Bit Float Multiplication: {0}ns", plArgF(t, 2), iResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "64 Bit Double Division")
  {
    plDynamicArray<double> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (plInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100.0;

    plTime t0 = plTime::Now();

    double fResult = 0;

    double d = 1.0;
    for (plInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      fResult += Ints[i] / d;

    plTime t1 = plTime::Now();

    plTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    plLog::Info("[test]64 Bit Double Division: {0}ns", plArgF(t, 2), fResult);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "64 Bit Double Multiplication")
  {
    plDynamicArray<double> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (plInt32 i = 0; i < iNumObjects; i++)
      Ints[i] = (double)(fNumObjects) - (double)(i);

    plTime t0 = plTime::Now();

    double iResult = 0;

    double d = 1.0;
    for (plInt32 i = 0; i < iNumObjects; i++, d += 1.0)
      iResult += Ints[i] * d;

    plTime t1 = plTime::Now();

    plTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    plLog::Info("[test]64 Bit Double Multiplication: {0}ns", plArgF(t, 2), iResult);
  }
}
