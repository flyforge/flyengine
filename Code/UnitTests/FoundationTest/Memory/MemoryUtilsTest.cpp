#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HybridArray.h>

static plInt32 iCallPodConstructor = 0;
static plInt32 iCallPodDestructor = 0;
static plInt32 iCallNonPodConstructor = 0;
static plInt32 iCallNonPodDestructor = 0;

struct plConstructTest
{
public:
  static plHybridArray<void*, 10> s_dtorList;

  plConstructTest() { m_iData = 42; }

  ~plConstructTest() { s_dtorList.PushBack(this); }

  plInt32 m_iData;
};
plHybridArray<void*, 10> plConstructTest::s_dtorList;

PLASMA_CHECK_AT_COMPILETIME(sizeof(plConstructTest) == 4);


struct PODTest
{
  PLASMA_DECLARE_POD_TYPE();

  PODTest() { m_iData = -1; }

  plInt32 m_iData;
};

static const plUInt32 s_uiSize = sizeof(plConstructTest);

PLASMA_CREATE_SIMPLE_TEST(Memory, MemoryUtils)
{
  plConstructTest::s_dtorList.Clear();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Construct")
  {
    plUInt8 uiRawData[s_uiSize * 5] = {0};
    plConstructTest* pTest = (plConstructTest*)(uiRawData);

    plMemoryUtils::Construct<plConstructTest>(pTest + 1, 2);

    PLASMA_TEST_INT(pTest[0].m_iData, 0);
    PLASMA_TEST_INT(pTest[1].m_iData, 42);
    PLASMA_TEST_INT(pTest[2].m_iData, 42);
    PLASMA_TEST_INT(pTest[3].m_iData, 0);
    PLASMA_TEST_INT(pTest[4].m_iData, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakeConstructorFunction")
  {
    plMemoryUtils::ConstructorFunction func = plMemoryUtils::MakeConstructorFunction<plConstructTest>();
    PLASMA_TEST_BOOL(func != nullptr);

    plUInt8 uiRawData[s_uiSize] = {0};
    plConstructTest* pTest = (plConstructTest*)(uiRawData);

    (*func)(pTest);

    PLASMA_TEST_INT(pTest->m_iData, 42);

    func = plMemoryUtils::MakeConstructorFunction<PODTest>();
    PLASMA_TEST_BOOL(func != nullptr);

    func = plMemoryUtils::MakeConstructorFunction<plInt32>();
    PLASMA_TEST_BOOL(func == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DefaultConstruct")
  {
    plUInt32 uiRawData[5]; // not initialized here

    plMemoryUtils::DefaultConstruct(uiRawData + 1, 2);

    PLASMA_TEST_INT(uiRawData[1], 0);
    PLASMA_TEST_INT(uiRawData[2], 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakeDefaultConstructorFunction")
  {
    plMemoryUtils::ConstructorFunction func = plMemoryUtils::MakeDefaultConstructorFunction<plInt32>();
    PLASMA_TEST_BOOL(func != nullptr);

    plInt32 iTest = 2;

    (*func)(&iTest);

    PLASMA_TEST_INT(iTest, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Construct Copy(Array)")
  {
    plUInt8 uiRawData[s_uiSize * 5] = {0};
    plConstructTest* pTest = (plConstructTest*)(uiRawData);

    plConstructTest copy[2];
    copy[0].m_iData = 43;
    copy[1].m_iData = 44;

    plMemoryUtils::CopyConstructArray<plConstructTest>(pTest + 1, copy, 2);

    PLASMA_TEST_INT(pTest[0].m_iData, 0);
    PLASMA_TEST_INT(pTest[1].m_iData, 43);
    PLASMA_TEST_INT(pTest[2].m_iData, 44);
    PLASMA_TEST_INT(pTest[3].m_iData, 0);
    PLASMA_TEST_INT(pTest[4].m_iData, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Construct Copy(Element)")
  {
    plUInt8 uiRawData[s_uiSize * 5] = {0};
    plConstructTest* pTest = (plConstructTest*)(uiRawData);

    plConstructTest copy;
    copy.m_iData = 43;

    plMemoryUtils::CopyConstruct<plConstructTest>(pTest + 1, copy, 2);

    PLASMA_TEST_INT(pTest[0].m_iData, 0);
    PLASMA_TEST_INT(pTest[1].m_iData, 43);
    PLASMA_TEST_INT(pTest[2].m_iData, 43);
    PLASMA_TEST_INT(pTest[3].m_iData, 0);
    PLASMA_TEST_INT(pTest[4].m_iData, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakeCopyConstructorFunction")
  {
    plMemoryUtils::CopyConstructorFunction func = plMemoryUtils::MakeCopyConstructorFunction<plConstructTest>();
    PLASMA_TEST_BOOL(func != nullptr);

    plUInt8 uiRawData[s_uiSize] = {0};
    plConstructTest* pTest = (plConstructTest*)(uiRawData);

    plConstructTest copy;
    copy.m_iData = 43;

    (*func)(pTest, &copy);

    PLASMA_TEST_INT(pTest->m_iData, 43);

    func = plMemoryUtils::MakeCopyConstructorFunction<PODTest>();
    PLASMA_TEST_BOOL(func != nullptr);

    func = plMemoryUtils::MakeCopyConstructorFunction<plInt32>();
    PLASMA_TEST_BOOL(func != nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Destruct")
  {
    plUInt8 uiRawData[s_uiSize * 5] = {0};
    plConstructTest* pTest = (plConstructTest*)(uiRawData);

    plMemoryUtils::Construct<plConstructTest>(pTest + 1, 2);

    PLASMA_TEST_INT(pTest[0].m_iData, 0);
    PLASMA_TEST_INT(pTest[1].m_iData, 42);
    PLASMA_TEST_INT(pTest[2].m_iData, 42);
    PLASMA_TEST_INT(pTest[3].m_iData, 0);
    PLASMA_TEST_INT(pTest[4].m_iData, 0);

    plConstructTest::s_dtorList.Clear();
    plMemoryUtils::Destruct<plConstructTest>(pTest, 4);
    PLASMA_TEST_INT(4, plConstructTest::s_dtorList.GetCount());

    if (plConstructTest::s_dtorList.GetCount() == 4)
    {
      PLASMA_TEST_BOOL(plConstructTest::s_dtorList[0] == &pTest[3]);
      PLASMA_TEST_BOOL(plConstructTest::s_dtorList[1] == &pTest[2]);
      PLASMA_TEST_BOOL(plConstructTest::s_dtorList[2] == &pTest[1]);
      PLASMA_TEST_BOOL(plConstructTest::s_dtorList[3] == &pTest[0]);
      PLASMA_TEST_INT(pTest[4].m_iData, 0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakeDestructorFunction")
  {
    plMemoryUtils::DestructorFunction func = plMemoryUtils::MakeDestructorFunction<plConstructTest>();
    PLASMA_TEST_BOOL(func != nullptr);

    plUInt8 uiRawData[s_uiSize] = {0};
    plConstructTest* pTest = (plConstructTest*)(uiRawData);

    plMemoryUtils::Construct(pTest, 1);
    PLASMA_TEST_INT(pTest->m_iData, 42);

    plConstructTest::s_dtorList.Clear();
    (*func)(pTest);
    PLASMA_TEST_INT(1, plConstructTest::s_dtorList.GetCount());

    if (plConstructTest::s_dtorList.GetCount() == 1)
    {
      PLASMA_TEST_BOOL(plConstructTest::s_dtorList[0] == pTest);
    }

    func = plMemoryUtils::MakeDestructorFunction<PODTest>();
    PLASMA_TEST_BOOL(func == nullptr);

    func = plMemoryUtils::MakeDestructorFunction<plInt32>();
    PLASMA_TEST_BOOL(func == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy")
  {
    plUInt8 uiRawData[5] = {1, 2, 3, 4, 5};
    plUInt8 uiRawData2[5] = {6, 7, 8, 9, 0};

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 2);
    PLASMA_TEST_INT(uiRawData[2], 3);
    PLASMA_TEST_INT(uiRawData[3], 4);
    PLASMA_TEST_INT(uiRawData[4], 5);

    plMemoryUtils::Copy(uiRawData + 1, uiRawData2 + 2, 3);

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 8);
    PLASMA_TEST_INT(uiRawData[2], 9);
    PLASMA_TEST_INT(uiRawData[3], 0);
    PLASMA_TEST_INT(uiRawData[4], 5);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move")
  {
    plUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 2);
    PLASMA_TEST_INT(uiRawData[2], 3);
    PLASMA_TEST_INT(uiRawData[3], 4);
    PLASMA_TEST_INT(uiRawData[4], 5);

    plMemoryUtils::CopyOverlapped(uiRawData + 1, uiRawData + 3, 2);

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 4);
    PLASMA_TEST_INT(uiRawData[2], 5);
    PLASMA_TEST_INT(uiRawData[3], 4);
    PLASMA_TEST_INT(uiRawData[4], 5);

    plMemoryUtils::CopyOverlapped(uiRawData + 1, uiRawData, 4);

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 1);
    PLASMA_TEST_INT(uiRawData[2], 4);
    PLASMA_TEST_INT(uiRawData[3], 5);
    PLASMA_TEST_INT(uiRawData[4], 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    plUInt8 uiRawData1[5] = {1, 2, 3, 4, 5};
    plUInt8 uiRawData2[5] = {1, 2, 3, 4, 5};
    plUInt8 uiRawData3[5] = {1, 2, 3, 4, 6};

    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(uiRawData1, uiRawData2, 5));
    PLASMA_TEST_BOOL(!plMemoryUtils::IsEqual(uiRawData1, uiRawData3, 5));
    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(uiRawData1, uiRawData3, 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ZeroFill")
  {
    plUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 2);
    PLASMA_TEST_INT(uiRawData[2], 3);
    PLASMA_TEST_INT(uiRawData[3], 4);
    PLASMA_TEST_INT(uiRawData[4], 5);

    // T*, size_t N overload
    plMemoryUtils::ZeroFill(uiRawData + 1, 3);

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 0);
    PLASMA_TEST_INT(uiRawData[2], 0);
    PLASMA_TEST_INT(uiRawData[3], 0);
    PLASMA_TEST_INT(uiRawData[4], 5);

    // T[N] overload
    plMemoryUtils::ZeroFillArray(uiRawData);

    PLASMA_TEST_INT(uiRawData[0], 0);
    PLASMA_TEST_INT(uiRawData[1], 0);
    PLASMA_TEST_INT(uiRawData[2], 0);
    PLASMA_TEST_INT(uiRawData[3], 0);
    PLASMA_TEST_INT(uiRawData[4], 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PatternFill")
  {
    plUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 2);
    PLASMA_TEST_INT(uiRawData[2], 3);
    PLASMA_TEST_INT(uiRawData[3], 4);
    PLASMA_TEST_INT(uiRawData[4], 5);

    // T*, size_t N overload
    plMemoryUtils::PatternFill(uiRawData + 1, 0xAB, 3);

    PLASMA_TEST_INT(uiRawData[0], 1);
    PLASMA_TEST_INT(uiRawData[1], 0xAB);
    PLASMA_TEST_INT(uiRawData[2], 0xAB);
    PLASMA_TEST_INT(uiRawData[3], 0xAB);
    PLASMA_TEST_INT(uiRawData[4], 5);

    // T[N] overload
    plMemoryUtils::PatternFillArray(uiRawData, 0xCD);

    PLASMA_TEST_INT(uiRawData[0], 0xCD);
    PLASMA_TEST_INT(uiRawData[1], 0xCD);
    PLASMA_TEST_INT(uiRawData[2], 0xCD);
    PLASMA_TEST_INT(uiRawData[3], 0xCD);
    PLASMA_TEST_INT(uiRawData[4], 0xCD);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compare")
  {
    plUInt32 uiRawDataA[3] = {1, 2, 3};
    plUInt32 uiRawDataB[3] = {3, 4, 5};

    PLASMA_TEST_INT(uiRawDataA[0], 1);
    PLASMA_TEST_INT(uiRawDataA[1], 2);
    PLASMA_TEST_INT(uiRawDataA[2], 3);
    PLASMA_TEST_INT(uiRawDataB[0], 3);
    PLASMA_TEST_INT(uiRawDataB[1], 4);
    PLASMA_TEST_INT(uiRawDataB[2], 5);

    PLASMA_TEST_BOOL(plMemoryUtils::Compare(uiRawDataA, uiRawDataB, 3) < 0);
    PLASMA_TEST_BOOL(plMemoryUtils::Compare(uiRawDataA + 2, uiRawDataB, 1) == 0);
    PLASMA_TEST_BOOL(plMemoryUtils::Compare(uiRawDataB, uiRawDataA, 3) > 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddByteOffset")
  {
    plInt32* pData1 = nullptr;
    pData1 = plMemoryUtils::AddByteOffset(pData1, 13);
    PLASMA_TEST_BOOL(pData1 == reinterpret_cast<plInt32*>(13));

    const plInt32* pData2 = nullptr;
    const plInt32* pData3 = plMemoryUtils::AddByteOffset(pData2, 17);
    PLASMA_TEST_BOOL(pData3 == reinterpret_cast<plInt32*>(17));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Align / IsAligned")
  {
    {
      plInt32* pData = (plInt32*)1;
      PLASMA_TEST_BOOL(!plMemoryUtils::IsAligned(pData, 4));
      pData = plMemoryUtils::AlignBackwards(pData, 4);
      PLASMA_TEST_BOOL(pData == reinterpret_cast<plInt32*>(0));
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
    }
    {
      plInt32* pData = (plInt32*)2;
      PLASMA_TEST_BOOL(!plMemoryUtils::IsAligned(pData, 4));
      pData = plMemoryUtils::AlignBackwards(pData, 4);
      PLASMA_TEST_BOOL(pData == reinterpret_cast<plInt32*>(0));
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
    }
    {
      plInt32* pData = (plInt32*)3;
      PLASMA_TEST_BOOL(!plMemoryUtils::IsAligned(pData, 4));
      pData = plMemoryUtils::AlignBackwards(pData, 4);
      PLASMA_TEST_BOOL(pData == reinterpret_cast<plInt32*>(0));
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
    }
    {
      plInt32* pData = (plInt32*)4;
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
      pData = plMemoryUtils::AlignBackwards(pData, 4);
      PLASMA_TEST_BOOL(pData == reinterpret_cast<plInt32*>(4));
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
    }

    {
      plInt32* pData = (plInt32*)1;
      PLASMA_TEST_BOOL(!plMemoryUtils::IsAligned(pData, 4));
      pData = plMemoryUtils::AlignForwards(pData, 4);
      PLASMA_TEST_BOOL(pData == reinterpret_cast<plInt32*>(4));
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
    }
    {
      plInt32* pData = (plInt32*)2;
      PLASMA_TEST_BOOL(!plMemoryUtils::IsAligned(pData, 4));
      pData = plMemoryUtils::AlignForwards(pData, 4);
      PLASMA_TEST_BOOL(pData == reinterpret_cast<plInt32*>(4));
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
    }
    {
      plInt32* pData = (plInt32*)3;
      PLASMA_TEST_BOOL(!plMemoryUtils::IsAligned(pData, 4));
      pData = plMemoryUtils::AlignForwards(pData, 4);
      PLASMA_TEST_BOOL(pData == reinterpret_cast<plInt32*>(4));
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
    }
    {
      plInt32* pData = (plInt32*)4;
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
      pData = plMemoryUtils::AlignForwards(pData, 4);
      PLASMA_TEST_BOOL(pData == reinterpret_cast<plInt32*>(4));
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pData, 4));
    }
  }
}
