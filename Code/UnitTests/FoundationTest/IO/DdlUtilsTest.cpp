#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>

static plVariant CreateVariant(plVariant::Type::Enum t, const void* pData);

PLASMA_CREATE_SIMPLE_TEST(IO, DdlUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToColor")
  {
    const char* szTestData = "\
Color $c1 { float { 1, 0, 0.5 } }\
Color $c2 { float { 2, 1, 1.5, 0.1 } }\
Color $c3 { unsigned_int8 { 128, 2, 32 } }\
Color $c4 { unsigned_int8 { 128, 0, 32, 64 } }\
float $c5 { 1, 0, 0.5 }\
float $c6 { 2, 1, 1.5, 0.1 }\
unsigned_int8 $c7 { 128, 2, 32 }\
unsigned_int8 $c8 { 128, 0, 32, 64 }\
Color $c9 { float { 1, 0 } }\
Color $c10 { float { 1, 0, 3, 4, 5 } }\
Color $c11 { float { } }\
Color $c12 { }\
Color $c13 { double { 1, 1, 1, 2 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plColor c1, c2, c3, c4, c5, c6, c7, c8, c0;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("t0"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c1"), c1).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c2"), c2).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c3"), c3).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c4"), c4).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c5"), c5).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c6"), c6).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c7"), c7).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c8"), c8).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c9"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c10"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c11"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c12"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColor(doc.FindElement("c13"), c0).Failed());

    PLASMA_TEST_BOOL(c1 == plColor(1, 0, 0.5f, 1.0f));
    PLASMA_TEST_BOOL(c2 == plColor(2, 1, 1.5f, 0.1f));
    PLASMA_TEST_BOOL(c3 == plColorGammaUB(128, 2, 32));
    PLASMA_TEST_BOOL(c4 == plColorGammaUB(128, 0, 32, 64));
    PLASMA_TEST_BOOL(c5 == plColor(1, 0, 0.5f, 1.0f));
    PLASMA_TEST_BOOL(c6 == plColor(2, 1, 1.5f, 0.1f));
    PLASMA_TEST_BOOL(c7 == plColorGammaUB(128, 2, 32));
    PLASMA_TEST_BOOL(c8 == plColorGammaUB(128, 0, 32, 64));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToColorGamma")
  {
    const char* szTestData = "\
Color $c1 { float { 1, 0, 0.5 } }\
Color $c2 { float { 2, 1, 1.5, 0.1 } }\
Color $c3 { unsigned_int8 { 128, 2, 32 } }\
Color $c4 { unsigned_int8 { 128, 0, 32, 64 } }\
float $c5 { 1, 0, 0.5 }\
float $c6 { 2, 1, 1.5, 0.1 }\
unsigned_int8 $c7 { 128, 2, 32 }\
unsigned_int8 $c8 { 128, 0, 32, 64 }\
Color $c9 { float { 1, 0 } }\
Color $c10 { float { 1, 0, 3, 4, 5 } }\
Color $c11 { float { } }\
Color $c12 { }\
Color $c13 { double { 1, 1, 1, 2 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plColorGammaUB c1, c2, c3, c4, c5, c6, c7, c8, c0;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("t0"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c1"), c1).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c2"), c2).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c3"), c3).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c4"), c4).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c5"), c5).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c6"), c6).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c7"), c7).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c8"), c8).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c9"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c10"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c11"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c12"), c0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c13"), c0).Failed());

    PLASMA_TEST_BOOL(c1 == plColorGammaUB(plColor(1, 0, 0.5f, 1.0f)));
    PLASMA_TEST_BOOL(c2 == plColorGammaUB(plColor(2, 1, 1.5f, 0.1f)));
    PLASMA_TEST_BOOL(c3 == plColorGammaUB(128, 2, 32));
    PLASMA_TEST_BOOL(c4 == plColorGammaUB(128, 0, 32, 64));
    PLASMA_TEST_BOOL(c5 == plColorGammaUB(plColor(1, 0, 0.5f, 1.0f)));
    PLASMA_TEST_BOOL(c6 == plColorGammaUB(plColor(2, 1, 1.5f, 0.1f)));
    PLASMA_TEST_BOOL(c7 == plColorGammaUB(128, 2, 32));
    PLASMA_TEST_BOOL(c8 == plColorGammaUB(128, 0, 32, 64));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToTime")
  {
    const char* szTestData = "\
Time $t1 { float { 0.1 } }\
Time $t2 { double { 0.2 } }\
float $t3 { 0.3 }\
double $t4 { 0.4 }\
Time $t5 { double { 0.2, 2 } }\
Time $t6 { int8 { 0, 2 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plTime t1, t2, t3, t4, t0;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTime(doc.FindElement("t0"), t0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTime(doc.FindElement("t1"), t1).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTime(doc.FindElement("t2"), t2).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTime(doc.FindElement("t3"), t3).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTime(doc.FindElement("t4"), t4).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTime(doc.FindElement("t5"), t0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTime(doc.FindElement("t6"), t0).Failed());

    PLASMA_TEST_FLOAT(t1.GetSeconds(), 0.1, 0.0001f);
    PLASMA_TEST_FLOAT(t2.GetSeconds(), 0.2, 0.0001f);
    PLASMA_TEST_FLOAT(t3.GetSeconds(), 0.3, 0.0001f);
    PLASMA_TEST_FLOAT(t4.GetSeconds(), 0.4, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToVec2")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2 } }\
float $v2 { 0.3, 3 }\
Vector $v3 { float { 0.1 } }\
Vector $v4 { float { 0.1, 2.2, 3.33 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plVec2 v0, v1, v2;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec2(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec2(doc.FindElement("v1"), v1).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec2(doc.FindElement("v2"), v2).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec2(doc.FindElement("v3"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec2(doc.FindElement("v4"), v0).Failed());

    PLASMA_TEST_VEC2(v1, plVec2(0.1f, 2.0f), 0.0001f);
    PLASMA_TEST_VEC2(v2, plVec2(0.3f, 3.0f), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToVec3")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2 } }\
float $v2 { 0.3, 3,0}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33,44 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plVec3 v0, v1, v2;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec3(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec3(doc.FindElement("v1"), v1).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec3(doc.FindElement("v2"), v2).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec3(doc.FindElement("v3"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec3(doc.FindElement("v4"), v0).Failed());

    PLASMA_TEST_VEC3(v1, plVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    PLASMA_TEST_VEC3(v2, plVec3(0.3f, 3.0f, 0.0f), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToVec4")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2, 44.5 } }\
float $v2 { 0.3, 3,0, 12.}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plVec4 v0, v1, v2;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec4(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec4(doc.FindElement("v1"), v1).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec4(doc.FindElement("v2"), v2).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec4(doc.FindElement("v3"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVec4(doc.FindElement("v4"), v0).Failed());

    PLASMA_TEST_VEC4(v1, plVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    PLASMA_TEST_VEC4(v2, plVec4(0.3f, 3.0f, 0.0f, 12.0f), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToMat3")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plMat3 v0, v1;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToMat3(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToMat3(doc.FindElement("v1"), v1).Succeeded());

    PLASMA_TEST_BOOL(v1.IsEqual(plMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToMat4")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plMat4 v0, v1;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToMat4(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToMat4(doc.FindElement("v1"), v1).Succeeded());

    PLASMA_TEST_BOOL(v1.IsEqual(plMat4(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToTransform")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plTransform v0, v1;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTransform(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTransform(doc.FindElement("v1"), v1).Succeeded());

    PLASMA_TEST_VEC3(v1.m_vPosition, plVec3(1, 2, 3), 0.0001f);
    PLASMA_TEST_BOOL(v1.m_qRotation == plQuat(4, 5, 6, 7));
    PLASMA_TEST_VEC3(v1.m_vScale, plVec3(8, 9, 10), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToQuat")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2, 44.5 } }\
float $v2 { 0.3, 3,0, 12.}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plQuat v0, v1, v2;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToQuat(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToQuat(doc.FindElement("v1"), v1).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToQuat(doc.FindElement("v2"), v2).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToQuat(doc.FindElement("v3"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToQuat(doc.FindElement("v4"), v0).Failed());

    PLASMA_TEST_BOOL(v1 == plQuat(0.1f, 2.0f, 3.2f, 44.5f));
    PLASMA_TEST_BOOL(v2 == plQuat(0.3f, 3.0f, 0.0f, 12.0f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToUuid")
  {
    const char* szTestData = "\
Data $v1 { unsigned_int64 { 12345678910, 10987654321 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plUuid v0, v1;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToUuid(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToUuid(doc.FindElement("v1"), v1).Succeeded());

    PLASMA_TEST_BOOL(v1 == plUuid(12345678910, 10987654321));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToAngle")
  {
    const char* szTestData = "\
Data $v1 { float { 45.23 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plAngle v0, v1;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToAngle(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToAngle(doc.FindElement("v1"), v1).Succeeded());

    PLASMA_TEST_FLOAT(v1.GetRadian(), 45.23f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToHashedString")
  {
    const char* szTestData = "\
Data $v1 { string { \"Hello World\" } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plHashedString v0, v1;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToHashedString(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToHashedString(doc.FindElement("v1"), v1).Succeeded());

    PLASMA_TEST_STRING(v1.GetView(), "Hello World");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToTempHashedString")
  {
    const char* szTestData = "\
Data $v1 { uint64 { 2720389094277464445 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plTempHashedString v0, v1;

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTempHashedString(doc.FindElement("v0"), v0).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToTempHashedString(doc.FindElement("v1"), v1).Succeeded());

    PLASMA_TEST_BOOL(v1 == plTempHashedString("GHIJK"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plOpenDdlUtils::ConvertToVariant")
  {
    const char* szTestData = "\
Color $v1 { float { 1, 0, 0.5 } }\
ColorGamma $v2 { unsigned_int8 { 128, 0, 32, 64 } }\
Time $v3 { float { 0.1 } }\
Vec2 $v4 { float { 0.1, 2 } }\
Vec3 $v5 { float { 0.1, 2, 3.2 } }\
Vec4 $v6 { float { 0.1, 2, 3.2, 44.5 } }\
Mat3 $v7 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
Mat4 $v8 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
Transform $v9 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
Quat $v10 { float { 0.1, 2, 3.2, 44.5 } }\
Uuid $v11 { unsigned_int64 { 12345678910, 10987654321 } }\
Angle $v12 { float { 45.23 } }\
HashedString $v13 { string { \"Soo much string\" } }\
TempHashedString $v14 { uint64 { 2720389094277464445 } }\
";

    StringStream stream(szTestData);
    plOpenDdlReader doc;
    PLASMA_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    plVariant v[15];

    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v0"), v[0]).Failed());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v1"), v[1]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v2"), v[2]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v3"), v[3]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v4"), v[4]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v5"), v[5]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v6"), v[6]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v7"), v[7]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v8"), v[8]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v9"), v[9]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v10"), v[10]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v11"), v[11]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v12"), v[12]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v13"), v[13]).Succeeded());
    PLASMA_TEST_BOOL(plOpenDdlUtils::ConvertToVariant(doc.FindElement("v14"), v[14]).Succeeded());

    PLASMA_TEST_BOOL(v[1].IsA<plColor>());
    PLASMA_TEST_BOOL(v[2].IsA<plColorGammaUB>());
    PLASMA_TEST_BOOL(v[3].IsA<plTime>());
    PLASMA_TEST_BOOL(v[4].IsA<plVec2>());
    PLASMA_TEST_BOOL(v[5].IsA<plVec3>());
    PLASMA_TEST_BOOL(v[6].IsA<plVec4>());
    PLASMA_TEST_BOOL(v[7].IsA<plMat3>());
    PLASMA_TEST_BOOL(v[8].IsA<plMat4>());
    PLASMA_TEST_BOOL(v[9].IsA<plTransform>());
    PLASMA_TEST_BOOL(v[10].IsA<plQuat>());
    PLASMA_TEST_BOOL(v[11].IsA<plUuid>());
    PLASMA_TEST_BOOL(v[12].IsA<plAngle>());
    PLASMA_TEST_BOOL(v[13].IsA<plHashedString>());
    PLASMA_TEST_BOOL(v[14].IsA<plTempHashedString>());

    PLASMA_TEST_BOOL(v[1].Get<plColor>() == plColor(1, 0, 0.5));
    PLASMA_TEST_BOOL(v[2].Get<plColorGammaUB>() == plColorGammaUB(128, 0, 32, 64));
    PLASMA_TEST_FLOAT(v[3].Get<plTime>().GetSeconds(), 0.1, 0.0001f);
    PLASMA_TEST_VEC2(v[4].Get<plVec2>(), plVec2(0.1f, 2.0f), 0.0001f);
    PLASMA_TEST_VEC3(v[5].Get<plVec3>(), plVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    PLASMA_TEST_VEC4(v[6].Get<plVec4>(), plVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    PLASMA_TEST_BOOL(v[7].Get<plMat3>().IsEqual(plMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    PLASMA_TEST_BOOL(v[8].Get<plMat4>().IsEqual(plMat4(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
    PLASMA_TEST_BOOL(v[9].Get<plTransform>().m_qRotation == plQuat(4, 5, 6, 7));
    PLASMA_TEST_VEC3(v[9].Get<plTransform>().m_vPosition, plVec3(1, 2, 3), 0.0001f);
    PLASMA_TEST_VEC3(v[9].Get<plTransform>().m_vScale, plVec3(8, 9, 10), 0.0001f);
    PLASMA_TEST_BOOL(v[10].Get<plQuat>() == plQuat(0.1f, 2.0f, 3.2f, 44.5f));
    PLASMA_TEST_BOOL(v[11].Get<plUuid>() == plUuid(12345678910, 10987654321));
    PLASMA_TEST_FLOAT(v[12].Get<plAngle>().GetRadian(), 45.23f, 0.0001f);
    PLASMA_TEST_STRING(v[13].Get<plHashedString>().GetView(), "Soo much string");
    PLASMA_TEST_BOOL(v[14].Get<plTempHashedString>() == plTempHashedString("GHIJK"));


    /// \test Test primitive types in plVariant
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreColor")
  {
    StreamComparer sc("Color $v1{float{1,2,3,4}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreColor(js, plColor(1, 2, 3, 4), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreColorGamma")
  {
    StreamComparer sc("ColorGamma $v1{uint8{1,2,3,4}}\n");

    plOpenDdlWriter js;
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreColorGamma(js, plColorGammaUB(1, 2, 3, 4), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreTime")
  {
    StreamComparer sc("Time $v1{double{2.3}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreTime(js, plTime::Seconds(2.3), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreVec2")
  {
    StreamComparer sc("Vec2 $v1{float{1,2}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreVec2(js, plVec2(1, 2), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreVec3")
  {
    StreamComparer sc("Vec3 $v1{float{1,2,3}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreVec3(js, plVec3(1, 2, 3), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreVec4")
  {
    StreamComparer sc("Vec4 $v1{float{1,2,3,4}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreVec4(js, plVec4(1, 2, 3, 4), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreMat3")
  {
    StreamComparer sc("Mat3 $v1{float{1,4,7,2,5,8,3,6,9}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreMat3(js, plMat3(1, 2, 3, 4, 5, 6, 7, 8, 9), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreMat4")
  {
    StreamComparer sc("Mat4 $v1{float{1,5,9,13,2,6,10,14,3,7,11,15,4,8,12,16}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreMat4(js, plMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), "v1", true);
  }

  // PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreTransform")
  //{
  //  StreamComparer sc("Transform $v1{float{1,4,7,2,5,8,3,6,9,10}}\n");

  //  plOpenDdlWriter js;
  //  js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
  //  js.SetOutputStream(&sc);

  //  plOpenDdlUtils::StoreTransform(js, plTransform(plVec3(10, 20, 30), plMat3(1, 2, 3, 4, 5, 6, 7, 8, 9)), "v1", true);
  //}

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreQuat")
  {
    StreamComparer sc("Quat $v1{float{1,2,3,4}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreQuat(js, plQuat(1, 2, 3, 4), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreUuid")
  {
    StreamComparer sc("Uuid $v1{u4{12345678910,10987654321}}\n");

    plOpenDdlWriter js;
    js.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::Shortest);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreUuid(js, plUuid(12345678910, 10987654321), "v1", true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreAngle")
  {
    StreamComparer sc("Angle $v1{float{2.3}}\n");

    plOpenDdlWriter js;
    js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    plOpenDdlUtils::StoreAngle(js, plAngle::Radian(2.3f), "v1", true);
    }

  // this test also covers all the types that Variant supports
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StoreVariant")
  {
    alignas(PLASMA_ALIGNMENT_OF(float)) plUInt8 rawData[sizeof(float) * 16]; // enough for mat4

    for (plUInt8 i = 0; i < PLASMA_ARRAY_SIZE(rawData); ++i)
    {
      rawData[i] = i + 33;
    }

    rawData[PLASMA_ARRAY_SIZE(rawData) - 1] = 0; // string terminator

    for (plUInt32 t = plVariant::Type::FirstStandardType + 1; t < plVariant::Type::LastStandardType; ++t)
    {
      const plVariant var = CreateVariant((plVariant::Type::Enum)t, rawData);

      plDefaultMemoryStreamStorage storage;
      plMemoryStreamWriter writer(&storage);
      plMemoryStreamReader reader(&storage);

      plOpenDdlWriter js;
      js.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Exact);
      js.SetOutputStream(&writer);

      plOpenDdlUtils::StoreVariant(js, var, "bla");

      plOpenDdlReader doc;
      PLASMA_TEST_BOOL(doc.ParseDocument(reader).Succeeded());

      const auto pVarElem = doc.GetRootElement()->FindChild("bla");

      plVariant result;
      plOpenDdlUtils::ConvertToVariant(pVarElem, result).IgnoreResult();

      PLASMA_TEST_BOOL(var == result);
    }
  }
}

static plVariant CreateVariant(plVariant::Type::Enum t, const void* pData)
{
  switch (t)
  {
    case plVariant::Type::Bool:
      return plVariant(*(plInt8*)pData != 0);
    case plVariant::Type::Int8:
      return plVariant(*((plInt8*)pData));
    case plVariant::Type::UInt8:
      return plVariant(*((plUInt8*)pData));
    case plVariant::Type::Int16:
      return plVariant(*((plInt16*)pData));
    case plVariant::Type::UInt16:
      return plVariant(*((plUInt16*)pData));
    case plVariant::Type::Int32:
      return plVariant(*((plInt32*)pData));
    case plVariant::Type::UInt32:
      return plVariant(*((plUInt32*)pData));
    case plVariant::Type::Int64:
      return plVariant(*((plInt64*)pData));
    case plVariant::Type::UInt64:
      return plVariant(*((plUInt64*)pData));
    case plVariant::Type::Float:
      return plVariant(*((float*)pData));
    case plVariant::Type::Double:
      return plVariant(*((double*)pData));
    case plVariant::Type::Color:
      return plVariant(*((plColor*)pData));
    case plVariant::Type::Vector2:
      return plVariant(*((plVec2*)pData));
    case plVariant::Type::Vector3:
      return plVariant(*((plVec3*)pData));
    case plVariant::Type::Vector4:
      return plVariant(*((plVec4*)pData));
    case plVariant::Type::Vector2I:
      return plVariant(*((plVec2I32*)pData));
    case plVariant::Type::Vector3I:
      return plVariant(*((plVec3I32*)pData));
    case plVariant::Type::Vector4I:
      return plVariant(*((plVec4I32*)pData));
    case plVariant::Type::Vector2U:
      return plVariant(*((plVec2U32*)pData));
    case plVariant::Type::Vector3U:
      return plVariant(*((plVec3U32*)pData));
    case plVariant::Type::Vector4U:
      return plVariant(*((plVec4U32*)pData));
    case plVariant::Type::Quaternion:
      return plVariant(*((plQuat*)pData));
    case plVariant::Type::Matrix3:
      return plVariant(*((plMat3*)pData));
    case plVariant::Type::Matrix4:
      return plVariant(*((plMat4*)pData));
    case plVariant::Type::Transform:
      return plVariant(*((plTransform*)pData));
    case plVariant::Type::String:
    case plVariant::Type::StringView: // string views are stored as full strings as well
      return plVariant((const char*)pData);
    case plVariant::Type::DataBuffer:
    {
      plDataBuffer db;
      db.SetCountUninitialized(sizeof(float) * 16);
      for (plUInt32 i = 0; i < db.GetCount(); ++i)
        db[i] = ((plUInt8*)pData)[i];

      return plVariant(db);
    }
    case plVariant::Type::Time:
      return plVariant(*((plTime*)pData));
    case plVariant::Type::Uuid:
      return plVariant(*((plUuid*)pData));
    case plVariant::Type::Angle:
      return plVariant(*((plAngle*)pData));
    case plVariant::Type::ColorGamma:
      return plVariant(*((plColorGammaUB*)pData));
    case plVariant::Type::HashedString:
    {
      plHashedString s;
      s.Assign((const char*)pData);
      return plVariant(s);
    }
    case plVariant::Type::TempHashedString:
      return plVariant(plTempHashedString((const char*)pData));

    default:
      PLASMA_REPORT_FAILURE("Unknown type");
  }

  return plVariant();
}
