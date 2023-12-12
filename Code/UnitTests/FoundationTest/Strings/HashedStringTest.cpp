#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/HashedString.h>

PLASMA_CREATE_SIMPLE_TEST(Strings, HashedString)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plHashedString s;
    plHashedString s2;

    s2.Assign("test"); // compile time hashing

    PLASMA_TEST_INT(s.GetHash(), 0xef46db3751d8e999llu);
    PLASMA_TEST_STRING(s.GetString().GetData(), "");
    PLASMA_TEST_BOOL(s.GetString().IsEmpty());

    plTempHashedString ts("test"); // compile time hashing
    PLASMA_TEST_INT(ts.GetHash(), 0x4fdcca5ddb678139llu);

    plStringBuilder sb = "test2";
    plTempHashedString ts2(sb.GetData()); // runtime hashing
    PLASMA_TEST_INT(ts2.GetHash(), 0x890e0a4c7111eb87llu);

    plTempHashedString ts3(s2);
    PLASMA_TEST_INT(ts3.GetHash(), 0x4fdcca5ddb678139llu);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Assign")
  {
    plHashedString s;
    s.Assign("Test"); // compile time hashing

    PLASMA_TEST_STRING(s.GetString().GetData(), "Test");
    PLASMA_TEST_INT(s.GetHash(), 0xda83efc38a8922b4llu);

    plStringBuilder sb = "test2";
    s.Assign(sb.GetData()); // runtime hashing
    PLASMA_TEST_STRING(s.GetString().GetData(), "test2");
    PLASMA_TEST_INT(s.GetHash(), 0x890e0a4c7111eb87llu);

    plTempHashedString ts("dummy");
    ts = "test"; // compile time hashing
    PLASMA_TEST_INT(ts.GetHash(), 0x4fdcca5ddb678139llu);

    ts = sb.GetData(); // runtime hashing
    PLASMA_TEST_INT(ts.GetHash(), 0x890e0a4c7111eb87llu);

    s.Assign("");
    PLASMA_TEST_INT(s.GetHash(), 0xef46db3751d8e999llu);
    PLASMA_TEST_STRING(s.GetString().GetData(), "");
    PLASMA_TEST_BOOL(s.GetString().IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TempHashedString")
  {
    plTempHashedString ts;
    plHashedString hs;

    PLASMA_TEST_INT(ts.GetHash(), hs.GetHash());

    PLASMA_TEST_INT(ts.GetHash(), 0xef46db3751d8e999llu);

    ts = "Test";
    plTempHashedString ts2 = ts;
    PLASMA_TEST_INT(ts.GetHash(), 0xda83efc38a8922b4llu);

    ts = "";
    ts2.Clear();
    PLASMA_TEST_INT(ts.GetHash(), 0xef46db3751d8e999llu);
    PLASMA_TEST_INT(ts2.GetHash(), 0xef46db3751d8e999llu);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator== / operator!=")
  {
    plHashedString s1, s2, s3, s4;
    s1.Assign("Test1");
    s2.Assign("Test2");
    s3.Assign("Test1");
    s4.Assign("Test2");

    plTempHashedString t1("Test1");
    plTempHashedString t2("Test2");

    PLASMA_TEST_STRING(s1.GetString().GetData(), "Test1");
    PLASMA_TEST_STRING(s2.GetString().GetData(), "Test2");
    PLASMA_TEST_STRING(s3.GetString().GetData(), "Test1");
    PLASMA_TEST_STRING(s4.GetString().GetData(), "Test2");

    PLASMA_TEST_BOOL(s1 == s1);
    PLASMA_TEST_BOOL(s2 == s2);
    PLASMA_TEST_BOOL(s3 == s3);
    PLASMA_TEST_BOOL(s4 == s4);
    PLASMA_TEST_BOOL(t1 == t1);
    PLASMA_TEST_BOOL(t2 == t2);

    PLASMA_TEST_BOOL(s1 != s2);
    PLASMA_TEST_BOOL(s1 == s3);
    PLASMA_TEST_BOOL(s1 != s4);
    PLASMA_TEST_BOOL(s1 == t1);
    PLASMA_TEST_BOOL(s1 != t2);

    PLASMA_TEST_BOOL(s2 != s3);
    PLASMA_TEST_BOOL(s2 == s4);
    PLASMA_TEST_BOOL(s2 != t1);
    PLASMA_TEST_BOOL(s2 == t2);

    PLASMA_TEST_BOOL(s3 != s4);
    PLASMA_TEST_BOOL(s3 == t1);
    PLASMA_TEST_BOOL(s3 != t2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copying")
  {
    plHashedString s1;
    s1.Assign("blaa");

    plHashedString s2(s1);
    plHashedString s3;
    s3 = s2;

    PLASMA_TEST_BOOL(s1 == s2);
    PLASMA_TEST_BOOL(s1 == s3);

    plHashedString s4(std::move(s2));
    plHashedString s5;
    s5 = std::move(s3);

    PLASMA_TEST_BOOL(s1 == s4);
    PLASMA_TEST_BOOL(s1 == s5);
    PLASMA_TEST_BOOL(s1 != s2);
    PLASMA_TEST_BOOL(s1 != s3);

    plTempHashedString t1("blaa");

    plTempHashedString t2(t1);
    plTempHashedString t3("urg");
    t3 = t2;

    PLASMA_TEST_BOOL(t1 == t2);
    PLASMA_TEST_BOOL(t1 == t3);

    t3 = s1;
    PLASMA_TEST_INT(t3.GetHash(), s1.GetHash());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator<")
  {
    plHashedString s1, s2, s3;
    s1.Assign("blaa");
    s2.Assign("blub");
    s3.Assign("tut");

    plMap<plHashedString, plInt32> m; // uses operator< internally
    m[s1] = 1;
    m[s2] = 2;
    m[s3] = 3;

    PLASMA_TEST_INT(m[s1], 1);
    PLASMA_TEST_INT(m[s2], 2);
    PLASMA_TEST_INT(m[s3], 3);

    plTempHashedString t1("blaa");
    plTempHashedString t2("blub");
    plTempHashedString t3("tut");

    PLASMA_TEST_BOOL((s1 < s1) == (t1 < t1));
    PLASMA_TEST_BOOL((s1 < s2) == (t1 < t2));
    PLASMA_TEST_BOOL((s1 < s3) == (t1 < t3));

    PLASMA_TEST_BOOL((s1 < s1) == (s1 < t1));
    PLASMA_TEST_BOOL((s1 < s2) == (s1 < t2));
    PLASMA_TEST_BOOL((s1 < s3) == (s1 < t3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetString")
  {
    plHashedString s1, s2, s3;
    s1.Assign("blaa");
    s2.Assign("blub");
    s3.Assign("tut");

    PLASMA_TEST_STRING(s1.GetString().GetData(), "blaa");
    PLASMA_TEST_STRING(s2.GetString().GetData(), "blub");
    PLASMA_TEST_STRING(s3.GetString().GetData(), "tut");
  }

#if PLASMA_ENABLED(PLASMA_HASHED_STRING_REF_COUNTING)
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ClearUnusedStrings")
  {
    plHashedString::ClearUnusedStrings();

    {
      plHashedString s1, s2, s3;
      s1.Assign("blaa");
      s2.Assign("blub");
      s3.Assign("tut");
    }

    PLASMA_TEST_INT(plHashedString::ClearUnusedStrings(), 3);
    PLASMA_TEST_INT(plHashedString::ClearUnusedStrings(), 0);
  }
#endif
}
