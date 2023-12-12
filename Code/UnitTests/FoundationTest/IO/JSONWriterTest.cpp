#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/OSFile.h>
#include <FoundationTest/IO/JSONTestHelpers.h>


PLASMA_CREATE_SIMPLE_TEST(IO, StandardJSONWriter)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Object")
  {
    StreamComparer sc("\"TestObject\" : {\n\
  \n\
}");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject("TestObject");
    js.EndObject();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Anonymous Object")
  {
    StreamComparer sc("{\n\
  \n\
}");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();
    js.EndObject();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableBool")
  {
    StreamComparer sc("\"var1\" : true,\n\"var2\" : false");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableBool("var1", true);
    js.AddVariableBool("var2", false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableInt32")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : -42");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableInt32("var1", 23);
    js.AddVariableInt32("var2", -42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableUInt32")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : 42");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUInt32("var1", 23);
    js.AddVariableUInt32("var2", 42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableInt64")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : -42");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableInt64("var1", 23);
    js.AddVariableInt64("var2", -42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableUInt64")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : 42");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUInt64("var1", 23);
    js.AddVariableUInt64("var2", 42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableFloat")
  {
    StreamComparer sc("\"var1\" : -65.5,\n\"var2\" : 2621.25");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableFloat("var1", -65.5f);
    js.AddVariableFloat("var2", 2621.25f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableDouble")
  {
    StreamComparer sc("\"var1\" : -65.125,\n\"var2\" : 2621.0625");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableDouble("var1", -65.125f);
    js.AddVariableDouble("var2", 2621.0625f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableString")
  {
    StreamComparer sc("\"var1\" : \"bla\",\n\"var2\" : \"blub\",\n\"special\" : \"I\\\\m\\t\\\"s\\bec/al\\\" \\f\\n//\\\\\\r\"");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableString("var1", "bla");
    js.AddVariableString("var2", "blub");

    js.AddVariableString("special", "I\\m\t\"s\bec/al\" \f\n//\\\r");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableNULL")
  {
    StreamComparer sc("\"var1\" : null");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableNULL("var1");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableTime")
  {
    StreamComparer sc("\"var1\" : 0.5,\n\"var2\" : 2.25");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableTime("var1", plTime::Seconds(0.5));
    js.AddVariableTime("var2", plTime::Seconds(2.25));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableUuid")
  {
    plUuid guid;
    plUInt64 val[2];
    val[0] = 0x1122334455667788;
    val[1] = 0x99AABBCCDDEEFF00;
    plMemoryUtils::Copy(reinterpret_cast<plUInt64*>(&guid), val, 2);

    StreamComparer sc("\"uuid_var\" : { \"$t\" : \"uuid\", \"$b\" : \"0x887766554433221100FFEEDDCCBBAA99\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUuid("uuid_var", guid);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableAngle")
  {
    StreamComparer sc("\"var1\" : 90,\n\"var2\" : 180");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    // vs2019 is so imprecise, that the degree->radian conversion introduces differences in the final output
    js.AddVariableAngle("var1", plAngle::Radian(1.5707963267f));
    js.AddVariableAngle("var2", plAngle::Radian(1.0f * plMath::Pi<float>()));
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableColor")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"color\", \"$v\" : \"(1.0000, 2.0000, 3.0000, 4.0000)\", \"$b\" : "
                      "\"0x0000803F000000400000404000008040\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableColor("var1", plColor(1, 2, 3, 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableVec2")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec2\", \"$v\" : \"(1.0000, 2.0000)\", \"$b\" : \"0x0000803F00000040\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec2("var1", plVec2(1, 2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableVec3")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec3\", \"$v\" : \"(1.0000, 2.0000, 3.0000)\", \"$b\" : \"0x0000803F0000004000004040\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec3("var1", plVec3(1, 2, 3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableVec4")
  {
    StreamComparer sc(
      "\"var1\" : { \"$t\" : \"vec4\", \"$v\" : \"(1.0000, 2.0000, 3.0000, 4.0000)\", \"$b\" : \"0x0000803F000000400000404000008040\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec4("var1", plVec4(1, 2, 3, 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableVec2I32")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec2i\", \"$v\" : \"(1, 2)\", \"$b\" : \"0x0100000002000000\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec2I32("var1", plVec2I32(1, 2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableVec3I32")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec3i\", \"$v\" : \"(1, 2, 3)\", \"$b\" : \"0x010000000200000003000000\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec3I32("var1", plVec3I32(1, 2, 3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableVec4I32")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec4i\", \"$v\" : \"(1, 2, 3, 4)\", \"$b\" : \"0x01000000020000000300000004000000\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec4I32("var1", plVec4I32(1, 2, 3, 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableDataBuffer")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"data\", \"$b\" : \"0xFF00DA\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    plDataBuffer db;
    db.PushBack(0xFF);
    db.PushBack(0x00);
    db.PushBack(0xDA);
    js.AddVariableDataBuffer("var1", db);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableQuat")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"quat\", \"$b\" : \"0x0000803F000000400000404000008040\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableQuat("var1", plQuat(1, 2, 3, 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableMat3")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"mat3\", \"$b\" : \"0x0000803F000080400000E040000000400000A04000000041000040400000C04000001041\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableMat3("var1", plMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableMat4")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"mat4\", \"$b\" : "
                      "\"0x0000803F0000A0400000104100005041000000400000C0400000204100006041000040400000E04000003041000070410000804000000041"
                      "0000404100008041\" }");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableMat4("var1", plMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddVariableVariant")
  {
    StreamComparer sc("\
\"var1\" : 23,\n\
\"var2\" : 42.5,\n\
\"var3\" : 21.25,\n\
\"var4\" : true,\n\
\"var5\" : \"pups\"");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVariant("var1", plVariant(23));
    js.AddVariableVariant("var2", plVariant(42.5f));
    js.AddVariableVariant("var3", plVariant(21.25));
    js.AddVariableVariant("var4", plVariant(true));
    js.AddVariableVariant("var5", plVariant("pups"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Arrays")
  {
    StreamComparer sc("\
{\n\
  \"EmptyArray\" : [  ],\n\
  \"NamedArray\" : [ 13 ],\n\
  \"NamedArray2\" : [ 1337, -4996 ],\n\
  \"Nested\" : [ null, [ 1, 2, 3 ], [ 4, 5, 6 ], [  ], \"That was an empty array\" ]\n\
}");

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();
    js.BeginArray("EmptyArray");
    js.EndArray();

    js.BeginArray("NamedArray");
    js.WriteInt32(13);
    js.EndArray();

    js.BeginArray("NamedArray2");
    js.WriteInt32(1337);
    js.WriteInt32(-4996);
    js.EndArray();

    js.BeginVariable("Nested");
    js.BeginArray();
    js.WriteNULL();

    js.BeginArray();
    js.WriteInt32(1);
    js.WriteInt32(2);
    js.WriteInt32(3);
    js.EndArray();

    js.BeginArray();
    js.WriteInt32(4);
    js.WriteInt32(5);
    js.WriteInt32(6);
    js.EndArray();

    js.BeginArray();
    js.EndArray();

    js.WriteString("That was an empty array");
    js.EndArray();
    js.EndVariable();

    js.EndObject();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Complex Objects")
  {
    plStringUtf8 sExp(L"\
{\n\
  \"String\" : \"testvälue\",\n\
  \"double\" : 43.56,\n\
  \"float\" : 64.720001,\n\
  \"bööl\" : true,\n\
  \"int\" : 23,\n\
  \"myarray\" : [ 1, 2.2, 3.3, false, \"ende\" ],\n\
  \"object\" : {\n\
    \"variable in object\" : \"bla/*asdf*/ //tuff\",\n\
    \"Subobject\" : {\n\
      \"variable in subobject\" : \"bla\\\\\",\n\
      \"array in sub\" : [ {\n\
          \"obj var\" : 234\n\
        },\n\
        {\n\
          \"obj var 2\" : -235\n\
        }, true, 4, false ]\n\
    }\n\
  },\n\
  \"test\" : \"text\"\n\
}");

    StreamComparer sc(sExp.GetData());

    plStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();

    js.AddVariableString("String", plStringUtf8(L"testvälue").GetData()); // Unicode / Utf-8 test (in string)
    js.AddVariableDouble("double", 43.56);
    js.AddVariableFloat("float", 64.72f);
    js.AddVariableBool(plStringUtf8(L"bööl").GetData(), true); // Unicode / Utf-8 test (identifier)
    js.AddVariableInt32("int", 23);

    js.BeginArray("myarray");
    js.WriteInt32(1);
    js.WriteFloat(2.2f);
    js.WriteDouble(3.3);
    js.WriteBool(false);
    js.WriteString("ende");
    js.EndArray();

    js.BeginObject("object");
    js.AddVariableString("variable in object", "bla/*asdf*/ //tuff"); // 'comment' in string
    js.BeginObject("Subobject");
    js.AddVariableString("variable in subobject", "bla\\"); // character to be escaped

    js.BeginArray("array in sub");
    js.BeginObject();
    js.AddVariableUInt64("obj var", 234);
    js.EndObject();
    js.BeginObject();
    js.AddVariableInt64("obj var 2", -235);
    js.EndObject();
    js.WriteBool(true);
    js.WriteInt32(4);
    js.WriteBool(false);
    js.EndArray();
    js.EndObject();
    js.EndObject();

    js.AddVariableString("test", "text");

    js.EndObject();
  }
}
