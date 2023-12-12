#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>

#include <vector>

namespace
{
  enum constants
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    NUM_SAMPLES = 128,
    NUM_APPENDS = 1024 * 32,
    NUM_RECUSRIVE_APPENDS = 128
#else
    NUM_SAMPLES = 1024,
    NUM_APPENDS = 1024 * 64,
    NUM_RECUSRIVE_APPENDS = 256
#endif
  };

  struct SomeBigObject
  {
    PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();

    static plUInt32 constructionCount;
    static plUInt32 destructionCount;
    plUInt64 i1, i2, i3, i4, i5, i6, i7, i8;

    SomeBigObject(plUInt64 uiInit)
      : i1(uiInit)
      , i2(uiInit)
      , i3(uiInit)
      , i4(uiInit)
      , i5(uiInit)
      , i6(uiInit)
      , i7(uiInit)
      , i8(uiInit)
    {
      constructionCount++;
    }

    ~SomeBigObject() { destructionCount++; }

    SomeBigObject(const SomeBigObject& rh)
    {
      constructionCount++;
      this->i1 = rh.i1;
      this->i2 = rh.i2;
      this->i3 = rh.i3;
      this->i4 = rh.i4;
      this->i5 = rh.i5;
      this->i6 = rh.i6;
      this->i7 = rh.i7;
      this->i8 = rh.i8;
    }

    void operator=(const SomeBigObject& rh)
    {
      constructionCount++;
      this->i1 = rh.i1;
      this->i2 = rh.i2;
      this->i3 = rh.i3;
      this->i4 = rh.i4;
      this->i5 = rh.i5;
      this->i6 = rh.i6;
      this->i7 = rh.i7;
      this->i8 = rh.i8;
    }
  };

  plUInt32 SomeBigObject::constructionCount = 0;
  plUInt32 SomeBigObject::destructionCount = 0;
} // namespace

// Enable when needed
#define PLASMA_PERFORMANCE_TESTS_STATE plTestBlock::DisabledNoWarning

PLASMA_CREATE_SIMPLE_TEST(Performance, Container)
{
  const char* TestString = "There are 10 types of people in the world. Those who understand binary and those who don't.";
  const plUInt32 TestStringLength = (plUInt32)strlen(TestString);

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "POD Dynamic Array Appending")
  {
    plTime t0 = plTime::Now();
    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      plDynamicArray<int> a;
      for (plUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.PushBack(i);
      }

      for (plUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        sum += a[i];
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info("[test]POD Dynamic Array Appending {0}ms", plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "POD std::vector Appending")
  {
    plTime t0 = plTime::Now();

    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<int> a;
      for (plUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.push_back(i);
      }

      for (plUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        sum += a[i];
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info("[test]POD std::vector Appending {0}ms", plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "plDynamicArray<plDynamicArray<char>> Appending")
  {
    plTime t0 = plTime::Now();

    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      plDynamicArray<plDynamicArray<char>> a;
      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        plUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        plDynamicArray<char>& cur = a[count];
        for (plUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.PushBack(TestString[j]);
        }
      }

      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetCount();
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info(
      "[test]plDynamicArray<plDynamicArray<char>> Appending {0}ms", plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "plDynamicArray<plHybridArray<char, 64>> Appending")
  {
    plTime t0 = plTime::Now();

    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      plDynamicArray<plHybridArray<char, 64>> a;
      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        plUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        plHybridArray<char, 64>& cur = a[count];
        for (plUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.PushBack(TestString[j]);
        }
      }

      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetCount();
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info("[test]plDynamicArray<plHybridArray<char, 64>> Appending {0}ms",
      plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "std::vector<std::vector<char>> Appending")
  {
    plTime t0 = plTime::Now();

    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<std::vector<char>> a;
      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        plUInt32 count = (plUInt32)a.size();
        a.resize(count + 1);
        std::vector<char>& cur = a[count];
        for (plUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.push_back(TestString[j]);
        }
      }

      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (plUInt32)a[i].size();
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info(
      "[test]std::vector<std::vector<char>> Appending {0}ms", plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "plDynamicArray<plString> Appending")
  {
    plTime t0 = plTime::Now();

    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      plDynamicArray<plString> a;
      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        plUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        plString& cur = a[count];
        plStringBuilder b;
        for (plUInt32 j = 0; j < TestStringLength; j++)
        {
          b.Append(TestString[i]);
        }
        cur = std::move(b);
      }

      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetElementCount();
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info("[test]plDynamicArray<plString> Appending {0}ms", plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "std::vector<std::string> Appending")
  {
    plTime t0 = plTime::Now();

    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<std::string> a;
      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        std::string cur;
        for (plUInt32 j = 0; j < TestStringLength; j++)
        {
          cur += TestString[i];
        }
        a.push_back(std::move(cur));
      }

      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (plUInt32)a[i].length();
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info("[test]std::vector<std::string> Appending {0}ms", plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "plDynamicArray<SomeBigObject> Appending")
  {
    plTime t0 = plTime::Now();

    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      plDynamicArray<SomeBigObject> a;
      for (plUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.PushBack(SomeBigObject(i));
      }

      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (plUInt32)a[i].i1;
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info(
      "[test]plDynamicArray<SomeBigObject> Appending {0}ms", plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(PLASMA_PERFORMANCE_TESTS_STATE, "std::vector<SomeBigObject> Appending")
  {
    plTime t0 = plTime::Now();

    plUInt32 sum = 0;
    for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<SomeBigObject> a;
      for (plUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.push_back(SomeBigObject(i));
      }

      for (plUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (plUInt32)a[i].i1;
      }
    }

    plTime t1 = plTime::Now();
    plLog::Info("[test]std::vector<SomeBigObject> Appending {0}ms", plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  PLASMA_TEST_BLOCK(plTestBlock::DisabledNoWarning, "plMap<void*, plUInt32>")
  {
    plUInt32 sum = 0;



    for (plUInt32 size = 1024; size < 4096 * 32; size += 1024)
    {
      plMap<void*, plUInt32> map;

      for (plUInt32 i = 0; i < size; i++)
      {
        map.Insert(malloc(64), 64);
      }

      void* ptrs[1024];

      plTime t0 = plTime::Now();
      for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
      {
        for (plUInt32 i = 0; i < 1024; i++)
        {
          void* mem = malloc(64);
          map.Insert(mem, 64);
          map.Remove(mem);
          ptrs[i] = mem;
        }

        for (plUInt32 i = 0; i < 1024; i++)
          free(ptrs[i]);

        auto last = map.GetLastIterator();
        for (auto it = map.GetIterator(); it != last; ++it)
        {
          sum += it.Value();
        }
      }
      plTime t1 = plTime::Now();

      auto last = map.GetLastIterator();
      for (auto it = map.GetIterator(); it != last; ++it)
      {
        free(it.Key());
      }
      plLog::Info(
        "[test]plMap<void*, plUInt32> size = {0} => {1}ms", size, plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::DisabledNoWarning, "plHashTable<void*, plUInt32>")
  {
    plUInt32 sum = 0;



    for (plUInt32 size = 1024; size < 4096 * 32; size += 1024)
    {
      plHashTable<void*, plUInt32> map;

      for (plUInt32 i = 0; i < size; i++)
      {
        map.Insert(malloc(64), 64);
      }

      void* ptrs[1024];

      plTime t0 = plTime::Now();
      for (plUInt32 n = 0; n < NUM_SAMPLES; n++)
      {

        for (plUInt32 i = 0; i < 1024; i++)
        {
          void* mem = malloc(64);
          map.Insert(mem, 64);
          map.Remove(mem);
          ptrs[i] = mem;
        }

        for (plUInt32 i = 0; i < 1024; i++)
          free(ptrs[i]);

        for (auto it = map.GetIterator(); it.IsValid(); it.Next())
        {
          sum += it.Value();
        }
      }
      plTime t1 = plTime::Now();

      for (auto it = map.GetIterator(); it.IsValid(); it.Next())
      {
        free(it.Key());
      }

      plLog::Info("[test]plHashTable<void*, plUInt32> size = {0} => {1}ms", size,
        plArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
    }
  }
}
