#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/Tag.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Types/TagSet.h>

static_assert(sizeof(plTagSet) == 16);

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
static_assert(sizeof(plTag) == 16);
#else
static_assert(sizeof(plTag) == 12);
#endif

PLASMA_CREATE_SIMPLE_TEST(Basics, TagSet)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basic Tag Tests")
  {
    plTagRegistry TempTestRegistry;

    {
      plTag TestTag;
      PLASMA_TEST_BOOL(!TestTag.IsValid());
    }

    plHashedString TagName;
    TagName.Assign("BASIC_TAG_TEST");

    const plTag& SecondInstance = TempTestRegistry.RegisterTag(TagName);
    PLASMA_TEST_BOOL(SecondInstance.IsValid());

    const plTag* SecondInstance2 = TempTestRegistry.GetTagByName("BASIC_TAG_TEST");

    PLASMA_TEST_BOOL(SecondInstance2 != nullptr);
    PLASMA_TEST_BOOL(SecondInstance2->IsValid());

    PLASMA_TEST_BOOL(&SecondInstance == SecondInstance2);

    PLASMA_TEST_STRING(SecondInstance2->GetTagString(), "BASIC_TAG_TEST");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basic Tag Registration")
  {
    plTagRegistry TempTestRegistry;

    plTag TestTag;

    PLASMA_TEST_BOOL(!TestTag.IsValid());

    PLASMA_TEST_BOOL(TempTestRegistry.GetTagByName("TEST_TAG1") == nullptr);

    TestTag = TempTestRegistry.RegisterTag("TEST_TAG1");

    PLASMA_TEST_BOOL(TestTag.IsValid());

    PLASMA_TEST_BOOL(TempTestRegistry.GetTagByName("TEST_TAG1") != nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basic Tag Work")
  {
    plTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    const plTag* TestTag1 = TempTestRegistry.GetTagByName("TEST_TAG1");
    PLASMA_TEST_BOOL(TestTag1 != nullptr);

    const plTag& TestTag2 = TempTestRegistry.RegisterTag("TEST_TAG2");

    PLASMA_TEST_BOOL(TestTag2.IsValid());

    plTagSet tagSet;

    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(TestTag2);

    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
    PLASMA_TEST_INT(tagSet.GetNumTagsSet(), 1);

    tagSet.Set(*TestTag1);

    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
    PLASMA_TEST_INT(tagSet.GetNumTagsSet(), 2);

    tagSet.Remove(*TestTag1);

    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
    PLASMA_TEST_INT(tagSet.GetNumTagsSet(), 1);

    plTagSet tagSet2 = tagSet;
    PLASMA_TEST_BOOL(tagSet2.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet2.IsSet(TestTag2) == true);
    PLASMA_TEST_INT(tagSet2.GetNumTagsSet(), 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Many Tags")
  {
    plTagRegistry TempTestRegistry;

    // TagSets have local storage for 1 block (64 tags)
    // Allocate enough tags so the storage overflows (or doesn't start at block 0)
    // for these tests

    plTag RegisteredTags[250];

    // Pre register some tags
    TempTestRegistry.RegisterTag("TEST_TAG1");
    TempTestRegistry.RegisterTag("TEST_TAG2");

    for (plUInt32 i = 0; i < 250; ++i)
    {
      plStringBuilder TagName;
      TagName.Format("TEST_TAG{0}", i);

      RegisteredTags[i] = TempTestRegistry.RegisterTag(TagName.GetData());

      PLASMA_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    PLASMA_TEST_INT(TempTestRegistry.GetNumTags(), 250);

    // Set all tags
    plTagSet BigTagSet;

    BigTagSet.Set(RegisteredTags[128]);
    BigTagSet.Set(RegisteredTags[64]);
    BigTagSet.Set(RegisteredTags[0]);

    PLASMA_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[0]));
    PLASMA_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[64]));
    PLASMA_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[128]));

    for (plUInt32 i = 0; i < 250; ++i)
    {
      BigTagSet.Set(RegisteredTags[i]);
    }

    for (plUInt32 i = 0; i < 250; ++i)
    {
      PLASMA_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (plUInt32 i = 10; i < 60; ++i)
    {
      BigTagSet.Remove(RegisteredTags[i]);
    }

    for (plUInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (plUInt32 i = 10; i < 60; ++i)
    {
      PLASMA_TEST_BOOL(!BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (plUInt32 i = 60; i < 250; ++i)
    {
      PLASMA_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    // Set tags, but starting outside block 0. This should do no allocation
    plTagSet Non0BlockStartSet;
    Non0BlockStartSet.Set(RegisteredTags[100]);
    PLASMA_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[100]));
    PLASMA_TEST_BOOL(!Non0BlockStartSet.IsSet(RegisteredTags[0]));

    plTagSet Non0BlockStartSet2 = Non0BlockStartSet;
    PLASMA_TEST_BOOL(Non0BlockStartSet2.IsSet(RegisteredTags[100]));
    PLASMA_TEST_INT(Non0BlockStartSet2.GetNumTagsSet(), Non0BlockStartSet.GetNumTagsSet());

    // Also test allocating a tag in an earlier block than the first tag allocated in the set
    Non0BlockStartSet.Set(RegisteredTags[0]);
    PLASMA_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[100]));
    PLASMA_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[0]));

    // Copying a tag set should work as well
    plTagSet SecondTagSet = BigTagSet;

    for (plUInt32 i = 60; i < 250; ++i)
    {
      PLASMA_TEST_BOOL(SecondTagSet.IsSet(RegisteredTags[i]));
    }

    for (plUInt32 i = 10; i < 60; ++i)
    {
      PLASMA_TEST_BOOL(!SecondTagSet.IsSet(RegisteredTags[i]));
    }

    PLASMA_TEST_INT(SecondTagSet.GetNumTagsSet(), BigTagSet.GetNumTagsSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsAnySet")
  {
    plTagRegistry TempTestRegistry;

    // TagSets have local storage for 1 block (64 tags)
    // Allocate enough tags so the storage overflows (or doesn't start at block 0)
    // for these tests

    plTag RegisteredTags[250];

    for (plUInt32 i = 0; i < 250; ++i)
    {
      plStringBuilder TagName;
      TagName.Format("TEST_TAG{0}", i);

      RegisteredTags[i] = TempTestRegistry.RegisterTag(TagName.GetData());

      PLASMA_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    plTagSet EmptyTagSet;
    plTagSet SecondEmptyTagSet;

    PLASMA_TEST_BOOL(!EmptyTagSet.IsAnySet(SecondEmptyTagSet));
    PLASMA_TEST_BOOL(!SecondEmptyTagSet.IsAnySet(EmptyTagSet));


    plTagSet SimpleSingleTagBlock0;
    SimpleSingleTagBlock0.Set(RegisteredTags[0]);

    plTagSet SimpleSingleTagBlock1;
    SimpleSingleTagBlock1.Set(RegisteredTags[0]);

    PLASMA_TEST_BOOL(!SecondEmptyTagSet.IsAnySet(SimpleSingleTagBlock0));

    PLASMA_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock0));
    PLASMA_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock1.Remove(RegisteredTags[0]);
    PLASMA_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));

    // Try with different block sizes/offsets (but same bit index)
    SimpleSingleTagBlock1.Set(RegisteredTags[64]);

    PLASMA_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    PLASMA_TEST_BOOL(!SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock0.Set(RegisteredTags[65]);
    PLASMA_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    PLASMA_TEST_BOOL(!SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock0.Set(RegisteredTags[64]);
    PLASMA_TEST_BOOL(SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    PLASMA_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    plTagSet OffsetBlock;
    OffsetBlock.Set(RegisteredTags[65]);
    PLASMA_TEST_BOOL(OffsetBlock.IsAnySet(SimpleSingleTagBlock0));
    PLASMA_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(OffsetBlock));

    plTagSet OffsetBlock2;
    OffsetBlock2.Set(RegisteredTags[66]);
    PLASMA_TEST_BOOL(!OffsetBlock.IsAnySet(OffsetBlock2));
    PLASMA_TEST_BOOL(!OffsetBlock2.IsAnySet(OffsetBlock));

    OffsetBlock2.Set(RegisteredTags[65]);
    PLASMA_TEST_BOOL(OffsetBlock.IsAnySet(OffsetBlock2));
    PLASMA_TEST_BOOL(OffsetBlock2.IsAnySet(OffsetBlock));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Add / Remove / IsEmpty / Clear")
  {
    plTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    const plTag* TestTag1 = TempTestRegistry.GetTagByName("TEST_TAG1");
    PLASMA_TEST_BOOL(TestTag1 != nullptr);

    const plTag& TestTag2 = TempTestRegistry.RegisterTag("TEST_TAG2");

    PLASMA_TEST_BOOL(TestTag2.IsValid());

    plTagSet tagSet;

    PLASMA_TEST_BOOL(tagSet.IsEmpty());
    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Clear();

    PLASMA_TEST_BOOL(tagSet.IsEmpty());
    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(TestTag2);

    PLASMA_TEST_BOOL(!tagSet.IsEmpty());
    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Remove(TestTag2);

    PLASMA_TEST_BOOL(tagSet.IsEmpty());
    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(*TestTag1);
    tagSet.Set(TestTag2);

    PLASMA_TEST_BOOL(!tagSet.IsEmpty());
    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Remove(*TestTag1);
    tagSet.Remove(TestTag2);

    PLASMA_TEST_BOOL(tagSet.IsEmpty());
    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(*TestTag1);
    tagSet.Set(TestTag2);

    PLASMA_TEST_BOOL(!tagSet.IsEmpty());
    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Clear();

    PLASMA_TEST_BOOL(tagSet.IsEmpty());
    PLASMA_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    PLASMA_TEST_BOOL(tagSet.IsSet(TestTag2) == false);
  }
}
