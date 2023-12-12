#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/JSONReader.h>

namespace JSONReaderTestDetail
{

  class StringStream : public plStreamReader
  {
  public:
    StringStream(const void* pData)
    {
      m_pData = pData;
      m_uiLength = plStringUtils::GetStringElementCount((const char*)pData);
    }

    virtual plUInt64 ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead)
    {
      uiBytesToRead = plMath::Min(uiBytesToRead, m_uiLength);
      m_uiLength -= uiBytesToRead;

      if (uiBytesToRead > 0)
      {
        plMemoryUtils::Copy((plUInt8*)pReadBuffer, (plUInt8*)m_pData, (size_t)uiBytesToRead);
        m_pData = plMemoryUtils::AddByteOffset(m_pData, (ptrdiff_t)uiBytesToRead);
      }

      return uiBytesToRead;
    }

  private:
    const void* m_pData;
    plUInt64 m_uiLength;
  };

  void TraverseTree(const plVariant& var, plDeque<plString>& ref_compare)
  {
    if (ref_compare.IsEmpty())
      return;

    switch (var.GetType())
    {
      case plVariant::Type::VariantDictionary:
      {
        // plLog::Printf("Expect: %s - Is: %s\n", "<object>", Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), "<object>");
        ref_compare.PopFront();

        const plVariantDictionary& vd = var.Get<plVariantDictionary>();

        for (auto it = vd.GetIterator(); it.IsValid(); ++it)
        {
          if (ref_compare.IsEmpty())
            return;

          // plLog::Printf("Expect: %s - Is: %s\n", it.Key().GetData(), Compare.PeekFront().GetData());
          PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), it.Key().GetData());
          ref_compare.PopFront();

          TraverseTree(it.Value(), ref_compare);
        }

        if (ref_compare.IsEmpty())
          return;

        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), "</object>");
        // plLog::Printf("Expect: %s - Is: %s\n", "</object>", Compare.PeekFront().GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::VariantArray:
      {
        // plLog::Printf("Expect: %s - Is: %s\n", "<array>", Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), "<array>");
        ref_compare.PopFront();

        const plVariantArray& va = var.Get<plVariantArray>();

        for (plUInt32 i = 0; i < va.GetCount(); ++i)
        {
          TraverseTree(va[i], ref_compare);
        }

        if (ref_compare.IsEmpty())
          return;

        // plLog::Printf("Expect: %s - Is: %s\n", "</array>", Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), "</array>");
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Invalid:
        // plLog::Printf("Expect: %s - Is: %s\n", "null", Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), "null");
        ref_compare.PopFront();
        break;

      case plVariant::Type::Bool:
        // plLog::Printf("Expect: %s - Is: %s\n", var.Get<bool>() ? "bool true" : "bool false", Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), var.Get<bool>() ? "bool true" : "bool false");
        ref_compare.PopFront();
        break;

      case plVariant::Type::Int8:
      {
        plStringBuilder sTemp;
        sTemp.Format("int8 {0}", var.Get<plInt8>());
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::UInt8:
      {
        plStringBuilder sTemp;
        sTemp.Format("uint8 {0}", var.Get<plUInt8>());
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Int16:
      {
        plStringBuilder sTemp;
        sTemp.Format("int16 {0}", var.Get<plInt16>());
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::UInt16:
      {
        plStringBuilder sTemp;
        sTemp.Format("uint16 {0}", var.Get<plUInt16>());
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Int32:
      {
        plStringBuilder sTemp;
        sTemp.Format("int32 {0}", var.Get<plInt32>());
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::UInt32:
      {
        plStringBuilder sTemp;
        sTemp.Format("uint32 {0}", var.Get<plUInt32>());
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Int64:
      {
        plStringBuilder sTemp;
        sTemp.Format("int64 {0}", var.Get<plInt64>());
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::UInt64:
      {
        plStringBuilder sTemp;
        sTemp.Format("uint64 {0}", var.Get<plUInt64>());
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Float:
      {
        plStringBuilder sTemp;
        sTemp.Format("float {0}", plArgF(var.Get<float>(), 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Double:
      {
        plStringBuilder sTemp;
        sTemp.Format("double {0}", plArgF(var.Get<double>(), 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Time:
      {
        plStringBuilder sTemp;
        sTemp.Format("time {0}", plArgF(var.Get<plTime>().GetSeconds(), 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Angle:
      {
        plStringBuilder sTemp;
        sTemp.Format("angle {0}", plArgF(var.Get<plAngle>().GetDegree(), 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::String:
        // plLog::Printf("Expect: %s - Is: %s\n", var.Get<plString>().GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), var.Get<plString>().GetData());
        ref_compare.PopFront();
        break;

      case plVariant::Type::StringView:
        // plLog::Printf("Expect: %s - Is: %s\n", var.Get<plString>().GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront(), var.Get<plStringView>());
        ref_compare.PopFront();
        break;

      case plVariant::Type::Vector2:
      {
        plStringBuilder sTemp;
        sTemp.Format("vec2 ({0}, {1})", plArgF(var.Get<plVec2>().x, 4), plArgF(var.Get<plVec2>().y, 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Vector3:
      {
        plStringBuilder sTemp;
        sTemp.Format("vec3 ({0}, {1}, {2})", plArgF(var.Get<plVec3>().x, 4), plArgF(var.Get<plVec3>().y, 4), plArgF(var.Get<plVec3>().z, 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Vector4:
      {
        plStringBuilder sTemp;
        sTemp.Format("vec4 ({0}, {1}, {2}, {3})", plArgF(var.Get<plVec4>().x, 4), plArgF(var.Get<plVec4>().y, 4), plArgF(var.Get<plVec4>().z, 4), plArgF(var.Get<plVec4>().w, 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Vector2I:
      {
        plStringBuilder sTemp;
        sTemp.Format("vec2i ({0}, {1})", var.Get<plVec2I32>().x, var.Get<plVec2I32>().y);
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Vector3I:
      {
        plStringBuilder sTemp;
        sTemp.Format("vec3i ({0}, {1}, {2})", var.Get<plVec3I32>().x, var.Get<plVec3I32>().y, var.Get<plVec3I32>().z);
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Vector4I:
      {
        plStringBuilder sTemp;
        sTemp.Format("vec4i ({0}, {1}, {2}, {3})", var.Get<plVec4I32>().x, var.Get<plVec4I32>().y, var.Get<plVec4I32>().z, var.Get<plVec4I32>().w);
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Color:
      {
        plStringBuilder sTemp;
        sTemp.Format("color ({0}, {1}, {2}, {3})", plArgF(var.Get<plColor>().r, 4), plArgF(var.Get<plColor>().g, 4), plArgF(var.Get<plColor>().b, 4), plArgF(var.Get<plColor>().a, 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::ColorGamma:
      {
        plStringBuilder sTemp;
        const plColorGammaUB c = var.ConvertTo<plColorGammaUB>();

        sTemp.Format("gamma ({0}, {1}, {2}, {3})", c.r, c.g, c.b, c.a);
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Quaternion:
      {
        plStringBuilder sTemp;
        sTemp.Format("quat ({0}, {1}, {2}, {3})", plArgF(var.Get<plQuat>().v.x, 4), plArgF(var.Get<plQuat>().v.y, 4), plArgF(var.Get<plQuat>().v.z, 4), plArgF(var.Get<plQuat>().w, 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Matrix3:
      {
        plMat3 m = var.Get<plMat3>();

        plStringBuilder sTemp;
        sTemp.Format("mat3 ({0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8})", plArgF(m.m_fElementsCM[0], 4), plArgF(m.m_fElementsCM[1], 4), plArgF(m.m_fElementsCM[2], 4), plArgF(m.m_fElementsCM[3], 4), plArgF(m.m_fElementsCM[4], 4), plArgF(m.m_fElementsCM[5], 4), plArgF(m.m_fElementsCM[6], 4), plArgF(m.m_fElementsCM[7], 4), plArgF(m.m_fElementsCM[8], 4));
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Matrix4:
      {
        plMat4 m = var.Get<plMat4>();

        plStringBuilder sTemp;
        sTemp.Printf("mat4 (%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f)", m.m_fElementsCM[0], m.m_fElementsCM[1], m.m_fElementsCM[2], m.m_fElementsCM[3], m.m_fElementsCM[4], m.m_fElementsCM[5], m.m_fElementsCM[6], m.m_fElementsCM[7], m.m_fElementsCM[8], m.m_fElementsCM[9], m.m_fElementsCM[10], m.m_fElementsCM[11], m.m_fElementsCM[12], m.m_fElementsCM[13], m.m_fElementsCM[14], m.m_fElementsCM[15]);
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case plVariant::Type::Uuid:
      {
        plUuid uuid = var.Get<plUuid>();
        plStringBuilder sTemp;
        plConversionUtils::ToString(uuid, sTemp);
        sTemp.Prepend("uuid ");
        // plLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        PLASMA_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }
} // namespace JSONReaderTestDetail

PLASMA_CREATE_SIMPLE_TEST(IO, JSONReader)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Test")
  {
    plStringUtf8 sTD(L"{\n\
\"myarray2\":[\"\",2.2],\n\
\"myarray\" : [1, 2.2, 3.3, false, \"ende\" ],\n\
\"String\"/**/ : \"testvälue\",\n\
\"double\"/***/ : 43.56,//comment\n\
\"float\" :/**//*a*/ 64/*comment*/.720001,\n\
\"bool\" : tr/*asdf*/ue,\n\
\"int\" : 23,\n\
\"MyNüll\" : nu/*asdf*/ll,\n\
\"object\" :\n\
/* totally \n weird \t stuff \n\n\n going on here // thats a line comment \n */ \
// more line comments \n\n\n\n\
{\n\
  \"variable in object\" : \"bla\\\\\\\"\\/\",\n\
    \"Subobject\" :\n\
  {\n\
    \"variable in subobject\" : \"blub\\r\\f\\n\\b\\t\",\n\
      \"array in sub\" : [\n\
    {\n\
      \"obj var\" : 234\n\
            /*stuff ] */ \
    },\n\
    {\n\
      \"obj var 2\" : -235\n//breakingcomment\n\
    }, true, 4, false ]\n\
  }\n\
},\n\
\"test\" : \"text\"\n\
}");
    const char* szTestData = sTD.GetData();

    // NOTE: The way this test is implemented, it might break, if the HashMap uses another insertion algorithm.
    // plVariantDictionary is an plHashmap and this test currently relies on one exact order in of the result.
    // If this should ever change (or be arbitrary at runtime), the test needs to be implemented in a more robust way.

    JSONReaderTestDetail::StringStream stream(szTestData);

    plJSONReader reader;
    PLASMA_TEST_BOOL(reader.Parse(stream).Succeeded());

    plDeque<plString> sCompare;
    sCompare.PushBack("<object>");
    sCompare.PushBack("int");
    sCompare.PushBack("double 23.0000");
    sCompare.PushBack("String");
    sCompare.PushBack(plStringUtf8(L"testvälue").GetData()); // unicode literal

    sCompare.PushBack("double");
    sCompare.PushBack("double 43.5600");

    sCompare.PushBack("myarray");
    sCompare.PushBack("<array>");
    sCompare.PushBack("double 1.0000");
    sCompare.PushBack("double 2.2000");
    sCompare.PushBack("double 3.3000");
    sCompare.PushBack("bool false");
    sCompare.PushBack("ende");
    sCompare.PushBack("</array>");

    sCompare.PushBack("object");
    sCompare.PushBack("<object>");

    sCompare.PushBack("Subobject");
    sCompare.PushBack("<object>");

    sCompare.PushBack("array in sub");
    sCompare.PushBack("<array>");

    sCompare.PushBack("<object>");
    sCompare.PushBack("obj var");
    sCompare.PushBack("double 234.0000");
    sCompare.PushBack("</object>");

    sCompare.PushBack("<object>");
    sCompare.PushBack("obj var 2");
    sCompare.PushBack("double -235.0000");
    sCompare.PushBack("</object>");

    sCompare.PushBack("bool true");
    sCompare.PushBack("double 4.0000");
    sCompare.PushBack("bool false");

    sCompare.PushBack("</array>");


    sCompare.PushBack("variable in subobject");
    sCompare.PushBack("blub\r\f\n\b\t"); // escaped special characters

    sCompare.PushBack("</object>");

    sCompare.PushBack("variable in object");
    sCompare.PushBack("bla\\\"/"); // escaped backslash, quotation mark, slash

    sCompare.PushBack("</object>");

    sCompare.PushBack("float");
    sCompare.PushBack("double 64.7200");

    sCompare.PushBack("myarray2");
    sCompare.PushBack("<array>");
    sCompare.PushBack("");
    sCompare.PushBack("double 2.2000");
    sCompare.PushBack("</array>");

    sCompare.PushBack(plStringUtf8(L"MyNüll").GetData()); // unicode literal
    sCompare.PushBack("null");

    sCompare.PushBack("test");
    sCompare.PushBack("text");

    sCompare.PushBack("bool");
    sCompare.PushBack("bool true");

    sCompare.PushBack("</object>");

    JSONReaderTestDetail::TraverseTree(reader.GetTopLevelObject(), sCompare);

    PLASMA_TEST_BOOL(sCompare.IsEmpty());
  }
}
