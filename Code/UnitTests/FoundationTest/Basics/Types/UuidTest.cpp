#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/Uuid.h>


PLASMA_CREATE_SIMPLE_TEST(Basics, Uuid)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Uuid Generation")
  {
    plUuid ShouldBeInvalid;

    PLASMA_TEST_BOOL(ShouldBeInvalid.IsValid() == false);

    plUuid FirstGenerated;
    FirstGenerated.CreateNewUuid();
    PLASMA_TEST_BOOL(FirstGenerated.IsValid());

    plUuid SecondGenerated;
    SecondGenerated.CreateNewUuid();
    PLASMA_TEST_BOOL(SecondGenerated.IsValid());

    PLASMA_TEST_BOOL(!(FirstGenerated == SecondGenerated));
    PLASMA_TEST_BOOL(FirstGenerated != SecondGenerated);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Uuid Serialization")
  {
    plUuid Uuid;
    PLASMA_TEST_BOOL(Uuid.IsValid() == false);

    Uuid.CreateNewUuid();
    PLASMA_TEST_BOOL(Uuid.IsValid());

    plDefaultMemoryStreamStorage StreamStorage;

    // Create reader
    plMemoryStreamReader StreamReader(&StreamStorage);

    // Create writer
    plMemoryStreamWriter StreamWriter(&StreamStorage);

    StreamWriter << Uuid;

    plUuid ReadBack;
    PLASMA_TEST_BOOL(ReadBack.IsValid() == false);

    StreamReader >> ReadBack;

    PLASMA_TEST_BOOL(ReadBack == Uuid);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Stable Uuid From String")
  {
    plUuid uuid1 = plUuid::StableUuidForString("TEST 1");
    plUuid uuid2 = plUuid::StableUuidForString("TEST 2");
    plUuid uuid3 = plUuid::StableUuidForString("TEST 1");

    PLASMA_TEST_BOOL(uuid1 == uuid3);
    PLASMA_TEST_BOOL(uuid1 != uuid2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Uuid Combine")
  {
    plUuid uuid1;
    uuid1.CreateNewUuid();
    plUuid uuid2;
    uuid2.CreateNewUuid();
    plUuid combined = uuid1;
    combined.CombineWithSeed(uuid2);
    PLASMA_TEST_BOOL(combined != uuid1);
    PLASMA_TEST_BOOL(combined != uuid2);
    combined.RevertCombinationWithSeed(uuid2);
    PLASMA_TEST_BOOL(combined == uuid1);

    plUuid hashA = uuid1;
    hashA.HashCombine(uuid2);
    plUuid hashB = uuid2;
    hashA.HashCombine(uuid1);
    PLASMA_TEST_BOOL(hashA != hashB);
  }
}
