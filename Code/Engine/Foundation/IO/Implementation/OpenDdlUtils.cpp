#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Types/VariantTypeRegistry.h>

plResult plOpenDdlUtils::ConvertToColor(const plOpenDdlReaderElement* pElement, plColor& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = pValues[3];

      return PLASMA_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = 1.0f;

      return PLASMA_SUCCESS;
    }
  }
  else if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::UInt8)
  {
    const plUInt8* pValues = pElement->GetPrimitivesUInt8();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result = plColorGammaUB(pValues[0], pValues[1], pValues[2], pValues[3]);

      return PLASMA_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result = plColorGammaUB(pValues[0], pValues[1], pValues[2]);

      return PLASMA_SUCCESS;
    }
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToColorGamma(const plOpenDdlReaderElement* pElement, plColorGammaUB& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result = plColor(pValues[0], pValues[1], pValues[2], pValues[3]);

      return PLASMA_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result = plColor(pValues[0], pValues[1], pValues[2]);

      return PLASMA_SUCCESS;
    }
  }
  else if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::UInt8)
  {
    const plUInt8* pValues = pElement->GetPrimitivesUInt8();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = pValues[3];

      return PLASMA_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = 255;

      return PLASMA_SUCCESS;
    }
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToTime(const plOpenDdlReaderElement* pElement, plTime& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result = plTime::Seconds(pValues[0]);

    return PLASMA_SUCCESS;
  }

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_result = plTime::Seconds(pValues[0]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVec2(const plOpenDdlReaderElement* pElement, plVec2& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVec3(const plOpenDdlReaderElement* pElement, plVec3& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVec4(const plOpenDdlReaderElement* pElement, plVec4& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVec2I(const plOpenDdlReaderElement* pElement, plVec2I32& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Int32)
  {
    const plInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVec3I(const plOpenDdlReaderElement* pElement, plVec3I32& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Int32)
  {
    const plInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVec4I(const plOpenDdlReaderElement* pElement, plVec4I32& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Int32)
  {
    const plInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}


plResult plOpenDdlUtils::ConvertToVec2U(const plOpenDdlReaderElement* pElement, plVec2U32& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::UInt32)
  {
    const plUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVec3U(const plOpenDdlReaderElement* pElement, plVec3U32& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::UInt32)
  {
    const plUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVec4U(const plOpenDdlReaderElement* pElement, plVec4U32& out_vResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::UInt32)
  {
    const plUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}



plResult plOpenDdlUtils::ConvertToMat3(const plOpenDdlReaderElement* pElement, plMat3& out_mResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 9)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_mResult.SetFromArray(pValues, plMatrixLayout::ColumnMajor);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToMat4(const plOpenDdlReaderElement* pElement, plMat4& out_mResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 16)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_mResult.SetFromArray(pValues, plMatrixLayout::ColumnMajor);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}


plResult plOpenDdlUtils::ConvertToTransform(const plOpenDdlReaderElement* pElement, plTransform& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 10)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.m_vPosition.x = pValues[0];
    out_result.m_vPosition.y = pValues[1];
    out_result.m_vPosition.z = pValues[2];
    out_result.m_qRotation.v.x = pValues[3];
    out_result.m_qRotation.v.y = pValues[4];
    out_result.m_qRotation.v.z = pValues[5];
    out_result.m_qRotation.w = pValues[6];
    out_result.m_vScale.x = pValues[7];
    out_result.m_vScale.y = pValues[8];
    out_result.m_vScale.z = pValues[9];

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToQuat(const plOpenDdlReaderElement* pElement, plQuat& out_qResult)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_qResult.SetElements(pValues[0], pValues[1], pValues[2], pValues[3]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToUuid(const plOpenDdlReaderElement* pElement, plUuid& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::UInt64)
  {
    const plUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_result = plUuid(pValues[0], pValues[1]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToAngle(const plOpenDdlReaderElement* pElement, plAngle& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    // have to use radians to prevent precision loss
    out_result = plAngle::Radian(pValues[0]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToHashedString(const plOpenDdlReaderElement* pElement, plHashedString& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::String)
  {
    const plStringView* pValues = pElement->GetPrimitivesString();

    out_result.Assign(pValues[0]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToTempHashedString(const plOpenDdlReaderElement* pElement, plTempHashedString& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return PLASMA_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return PLASMA_FAILURE;

  if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::UInt64)
  {
    const plUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_result = plTempHashedString(pValues[0]);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plOpenDdlUtils::ConvertToVariant(const plOpenDdlReaderElement* pElement, plVariant& out_result)
{
  if (pElement == nullptr)
    return PLASMA_FAILURE;

  // expect a custom type
  if (pElement->IsCustomType())
  {
    if (pElement->GetCustomType() == "VarArray")
    {
      plVariantArray value;
      plVariant varChild;

      /// \test This is just quickly hacked
      /// \todo Store array size for reserving var array length

      for (const plOpenDdlReaderElement* pChild = pElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
      {
        if (ConvertToVariant(pChild, varChild).Failed())
          return PLASMA_FAILURE;

        value.PushBack(varChild);
      }

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "VarDict")
    {
      plVariantDictionary value;
      plVariant varChild;

      /// \test This is just quickly hacked
      /// \todo Store array size for reserving var array length

      for (const plOpenDdlReaderElement* pChild = pElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
      {
        // no name -> invalid dictionary entry
        if (!pChild->HasName())
          continue;

        if (ConvertToVariant(pChild, varChild).Failed())
          return PLASMA_FAILURE;

        value[pChild->GetName()] = varChild;
      }

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "VarDataBuffer")
    {
      /// \test This is just quickly hacked

      plDataBuffer value;

      const plOpenDdlReaderElement* pString = pElement->GetFirstChild();

      if (!pString->HasPrimitives(plOpenDdlPrimitiveType::String))
        return PLASMA_FAILURE;

      const plStringView* pValues = pString->GetPrimitivesString();

      value.SetCountUninitialized(pValues[0].GetElementCount() / 2);
      plConversionUtils::ConvertHexToBinary(pValues[0].GetStartPointer(), value.GetData(), value.GetCount());

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Color")
    {
      plColor value;
      if (ConvertToColor(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "ColorGamma")
    {
      plColorGammaUB value;
      if (ConvertToColorGamma(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Time")
    {
      plTime value;
      if (ConvertToTime(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2")
    {
      plVec2 value;
      if (ConvertToVec2(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3")
    {
      plVec3 value;
      if (ConvertToVec3(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4")
    {
      plVec4 value;
      if (ConvertToVec4(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2i")
    {
      plVec2I32 value;
      if (ConvertToVec2I(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3i")
    {
      plVec3I32 value;
      if (ConvertToVec3I(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4i")
    {
      plVec4I32 value;
      if (ConvertToVec4I(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2u")
    {
      plVec2U32 value;
      if (ConvertToVec2U(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3u")
    {
      plVec3U32 value;
      if (ConvertToVec3U(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4u")
    {
      plVec4U32 value;
      if (ConvertToVec4U(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Mat3")
    {
      plMat3 value;
      if (ConvertToMat3(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Mat4")
    {
      plMat4 value;
      if (ConvertToMat4(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Transform")
    {
      plTransform value;
      if (ConvertToTransform(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Quat")
    {
      plQuat value;
      if (ConvertToQuat(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Uuid")
    {
      plUuid value;
      if (ConvertToUuid(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Angle")
    {
      plAngle value;
      if (ConvertToAngle(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "HashedString")
    {
      plHashedString value;
      if (ConvertToHashedString(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "TempHashedString")
    {
      plTempHashedString value;
      if (ConvertToTempHashedString(pElement, value).Failed())
        return PLASMA_FAILURE;

      out_result = value;
      return PLASMA_SUCCESS;
    }

    if (pElement->GetCustomType() == "Invalid")
    {
      out_result = plVariant();
      return PLASMA_SUCCESS;
    }

    if (const plRTTI* pRTTI = plRTTI::FindTypeByName(pElement->GetCustomType()))
    {
      if (plVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pRTTI))
      {
        if (pElement == nullptr)
          return PLASMA_FAILURE;

        void* pObject = pRTTI->GetAllocator()->Allocate<void>();

        for (const plOpenDdlReaderElement* pChildElement = pElement->GetFirstChild(); pChildElement != nullptr; pChildElement = pChildElement->GetSibling())
        {
          if (!pChildElement->HasName())
            continue;

          if (const plAbstractProperty* pProp = pRTTI->FindPropertyByName(pChildElement->GetName()))
          {
            // Custom types should be POD and only consist of member properties.
            if (pProp->GetCategory() == plPropertyCategory::Member)
            {
              plVariant subValue;
              if (ConvertToVariant(pChildElement, subValue).Succeeded())
              {
                plReflectionUtils::SetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), pObject, subValue);
              }
            }
          }
        }
        out_result.MoveTypedObject(pObject, pRTTI);
        return PLASMA_SUCCESS;
      }
      else
      {
        plLog::Error("The type '{0}' was declared but not defined, add PLASMA_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", pElement->GetCustomType());
      }
    }
    else
    {
      plLog::Error("The type '{0}' is unknown.", pElement->GetCustomType());
    }
  }
  else
  {
    // always expect exactly one value
    if (pElement->GetNumPrimitives() != 1)
      return PLASMA_FAILURE;

    switch (pElement->GetPrimitivesType())
    {
      case plOpenDdlPrimitiveType::Bool:
        out_result = pElement->GetPrimitivesBool()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::Int8:
        out_result = pElement->GetPrimitivesInt8()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::Int16:
        out_result = pElement->GetPrimitivesInt16()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::Int32:
        out_result = pElement->GetPrimitivesInt32()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::Int64:
        out_result = pElement->GetPrimitivesInt64()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::UInt8:
        out_result = pElement->GetPrimitivesUInt8()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::UInt16:
        out_result = pElement->GetPrimitivesUInt16()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::UInt32:
        out_result = pElement->GetPrimitivesUInt32()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::UInt64:
        out_result = pElement->GetPrimitivesUInt64()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::Float:
        out_result = pElement->GetPrimitivesFloat()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::Double:
        out_result = pElement->GetPrimitivesDouble()[0];
        return PLASMA_SUCCESS;

      case plOpenDdlPrimitiveType::String:
        out_result = plString(pElement->GetPrimitivesString()[0]); // make sure this isn't stored as a string view by copying to to an plString first
        return PLASMA_SUCCESS;

      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  return PLASMA_FAILURE;
}

void plOpenDdlUtils::StoreColor(plOpenDdlWriter& ref_writer, const plColor& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Color", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreColorGamma(
  plOpenDdlWriter& ref_writer, const plColorGammaUB& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("ColorGamma", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt8);
    ref_writer.WriteUInt8(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreTime(plOpenDdlWriter& ref_writer, const plTime& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Time", sName, bGlobalName, true);
  {
    const double d = value.GetSeconds();

    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Double);
    ref_writer.WriteDouble(&d, 1);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec2(plOpenDdlWriter& ref_writer, const plVec2& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec3(plOpenDdlWriter& ref_writer, const plVec3& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec4(plOpenDdlWriter& ref_writer, const plVec4& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec2I(plOpenDdlWriter& ref_writer, const plVec2I32& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec3I(plOpenDdlWriter& ref_writer, const plVec3I32& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec4I(plOpenDdlWriter& ref_writer, const plVec4I32& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec2U(plOpenDdlWriter& ref_writer, const plVec2U32& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec3U(plOpenDdlWriter& ref_writer, const plVec3U32& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVec4U(plOpenDdlWriter& ref_writer, const plVec4U32& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}


void plOpenDdlUtils::StoreMat3(plOpenDdlWriter& ref_writer, const plMat3& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Mat3", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);

    float f[9];
    value.GetAsArray(f, plMatrixLayout::ColumnMajor);
    ref_writer.WriteFloat(f, 9);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreMat4(plOpenDdlWriter& ref_writer, const plMat4& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Mat4", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);

    float f[16];
    value.GetAsArray(f, plMatrixLayout::ColumnMajor);
    ref_writer.WriteFloat(f, 16);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreTransform(plOpenDdlWriter& ref_writer, const plTransform& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Transform", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);

    float f[10];

    f[0] = value.m_vPosition.x;
    f[1] = value.m_vPosition.y;
    f[2] = value.m_vPosition.z;

    f[3] = value.m_qRotation.v.x;
    f[4] = value.m_qRotation.v.y;
    f[5] = value.m_qRotation.v.z;
    f[6] = value.m_qRotation.w;

    f[7] = value.m_vScale.x;
    f[8] = value.m_vScale.y;
    f[9] = value.m_vScale.z;

    ref_writer.WriteFloat(f, 10);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreQuat(plOpenDdlWriter& ref_writer, const plQuat& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Quat", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.v.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreUuid(plOpenDdlWriter& ref_writer, const plUuid& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Uuid", sName, bGlobalName, true);
  {
    plUInt64 ui[2];
    value.GetValues(ui[0], ui[1]);

    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(ui, 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreAngle(plOpenDdlWriter& ref_writer, const plAngle& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Angle", sName, bGlobalName, true);
  {
    // have to use radians to prevent precision loss
    const float f = value.GetRadian();

    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(&f, 1);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreHashedString(plOpenDdlWriter& ref_writer, const plHashedString& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("HashedString", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::String);
    ref_writer.WriteString(value.GetView());
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreTempHashedString(plOpenDdlWriter& ref_writer, const plTempHashedString& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("TempHashedString", sName, bGlobalName, true);
  {
    const plUInt64 uiHash = value.GetHash();

    ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(&uiHash);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void plOpenDdlUtils::StoreVariant(plOpenDdlWriter& ref_writer, const plVariant& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  switch (value.GetType())
  {
    case plVariant::Type::Invalid:
      StoreInvalid(ref_writer, sName, bGlobalName);
      return;

    case plVariant::Type::Bool:
      StoreBool(ref_writer, value.Get<bool>(), sName, bGlobalName);
      return;

    case plVariant::Type::Int8:
      StoreInt8(ref_writer, value.Get<plInt8>(), sName, bGlobalName);
      return;

    case plVariant::Type::UInt8:
      StoreUInt8(ref_writer, value.Get<plUInt8>(), sName, bGlobalName);
      return;

    case plVariant::Type::Int16:
      StoreInt16(ref_writer, value.Get<plInt16>(), sName, bGlobalName);
      return;

    case plVariant::Type::UInt16:
      StoreUInt16(ref_writer, value.Get<plUInt16>(), sName, bGlobalName);
      return;

    case plVariant::Type::Int32:
      StoreInt32(ref_writer, value.Get<plInt32>(), sName, bGlobalName);
      return;

    case plVariant::Type::UInt32:
      StoreUInt32(ref_writer, value.Get<plUInt32>(), sName, bGlobalName);
      return;

    case plVariant::Type::Int64:
      StoreInt64(ref_writer, value.Get<plInt64>(), sName, bGlobalName);
      return;

    case plVariant::Type::UInt64:
      StoreUInt64(ref_writer, value.Get<plUInt64>(), sName, bGlobalName);
      return;

    case plVariant::Type::Float:
      StoreFloat(ref_writer, value.Get<float>(), sName, bGlobalName);
      return;

    case plVariant::Type::Double:
      StoreDouble(ref_writer, value.Get<double>(), sName, bGlobalName);
      return;

    case plVariant::Type::String:
      plOpenDdlUtils::StoreString(ref_writer, value.Get<plString>(), sName, bGlobalName);
      return;

    case plVariant::Type::StringView:
      plOpenDdlUtils::StoreString(ref_writer, value.Get<plStringView>(), sName, bGlobalName);
      return;

    case plVariant::Type::Color:
      StoreColor(ref_writer, value.Get<plColor>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector2:
      StoreVec2(ref_writer, value.Get<plVec2>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector3:
      StoreVec3(ref_writer, value.Get<plVec3>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector4:
      StoreVec4(ref_writer, value.Get<plVec4>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector2I:
      StoreVec2I(ref_writer, value.Get<plVec2I32>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector3I:
      StoreVec3I(ref_writer, value.Get<plVec3I32>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector4I:
      StoreVec4I(ref_writer, value.Get<plVec4I32>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector2U:
      StoreVec2U(ref_writer, value.Get<plVec2U32>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector3U:
      StoreVec3U(ref_writer, value.Get<plVec3U32>(), sName, bGlobalName);
      return;

    case plVariant::Type::Vector4U:
      StoreVec4U(ref_writer, value.Get<plVec4U32>(), sName, bGlobalName);
      return;

    case plVariant::Type::Quaternion:
      StoreQuat(ref_writer, value.Get<plQuat>(), sName, bGlobalName);
      return;

    case plVariant::Type::Matrix3:
      StoreMat3(ref_writer, value.Get<plMat3>(), sName, bGlobalName);
      return;

    case plVariant::Type::Matrix4:
      StoreMat4(ref_writer, value.Get<plMat4>(), sName, bGlobalName);
      return;

    case plVariant::Type::Transform:
      StoreTransform(ref_writer, value.Get<plTransform>(), sName, bGlobalName);
      return;

    case plVariant::Type::Time:
      StoreTime(ref_writer, value.Get<plTime>(), sName, bGlobalName);
      return;

    case plVariant::Type::Uuid:
      StoreUuid(ref_writer, value.Get<plUuid>(), sName, bGlobalName);
      return;

    case plVariant::Type::Angle:
      StoreAngle(ref_writer, value.Get<plAngle>(), sName, bGlobalName);
      return;

    case plVariant::Type::ColorGamma:
      StoreColorGamma(ref_writer, value.Get<plColorGammaUB>(), sName, bGlobalName);
      return;

    case plVariant::Type::HashedString:
      StoreHashedString(ref_writer, value.Get<plHashedString>(), sName, bGlobalName);
      return;

    case plVariant::Type::TempHashedString:
      StoreTempHashedString(ref_writer, value.Get<plTempHashedString>(), sName, bGlobalName);
      return;

    case plVariant::Type::VariantArray:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarArray", sName, bGlobalName);

      const plVariantArray& arr = value.Get<plVariantArray>();
      for (plUInt32 i = 0; i < arr.GetCount(); ++i)
      {
        plOpenDdlUtils::StoreVariant(ref_writer, arr[i]);
      }

      ref_writer.EndObject();
    }
      return;

    case plVariant::Type::VariantDictionary:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarDict", sName, bGlobalName);

      const plVariantDictionary& dict = value.Get<plVariantDictionary>();
      for (auto it = dict.GetIterator(); it.IsValid(); ++it)
      {
        plOpenDdlUtils::StoreVariant(ref_writer, it.Value(), it.Key(), false);
      }

      ref_writer.EndObject();
    }
      return;

    case plVariant::Type::DataBuffer:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarDataBuffer", sName, bGlobalName);
      ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::String);

      const plDataBuffer& db = value.Get<plDataBuffer>();
      ref_writer.WriteBinaryAsString(db.GetData(), db.GetCount());

      ref_writer.EndPrimitiveList();
      ref_writer.EndObject();
    }
      return;

    case plVariant::Type::TypedObject:
    {
      plTypedObject obj = value.Get<plTypedObject>();
      if (plVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(obj.m_pType))
      {
        ref_writer.BeginObject(obj.m_pType->GetTypeName(), sName, bGlobalName);
        {
          plHybridArray<const plAbstractProperty*, 32> properties;
          obj.m_pType->GetAllProperties(properties);
          for (const plAbstractProperty* pProp : properties)
          {
            // Custom types should be POD and only consist of member properties.
            switch (pProp->GetCategory())
            {
              case plPropertyCategory::Member:
              {
                plVariant subValue = plReflectionUtils::GetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), obj.m_pObject);
                StoreVariant(ref_writer, subValue, pProp->GetPropertyName(), false);
              }
              break;
              case plPropertyCategory::Array:
              case plPropertyCategory::Set:
              case plPropertyCategory::Map:
                PLASMA_REPORT_FAILURE("Only member properties are supported in custom variant types!");
                break;
              case plPropertyCategory::Constant:
              case plPropertyCategory::Function:
                break;
            }
          }
        }
        ref_writer.EndObject();
      }
      else
      {
        plLog::Error("The type '{0}' was declared but not defined, add PLASMA_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", obj.m_pType->GetTypeName());
      }
    }
      return;
    default:
      PLASMA_REPORT_FAILURE("Can't write this type of Variant");
  }
}

void plOpenDdlUtils::StoreString(plOpenDdlWriter& ref_writer, const plStringView& value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::String, sName, bGlobalName);
  ref_writer.WriteString(value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreBool(plOpenDdlWriter& ref_writer, bool value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Bool, sName, bGlobalName);
  ref_writer.WriteBool(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreFloat(plOpenDdlWriter& ref_writer, float value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Float, sName, bGlobalName);
  ref_writer.WriteFloat(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreDouble(plOpenDdlWriter& ref_writer, double value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Double, sName, bGlobalName);
  ref_writer.WriteDouble(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreInt8(plOpenDdlWriter& ref_writer, plInt8 value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Int8, sName, bGlobalName);
  ref_writer.WriteInt8(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreInt16(plOpenDdlWriter& ref_writer, plInt16 value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Int16, sName, bGlobalName);
  ref_writer.WriteInt16(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreInt32(plOpenDdlWriter& ref_writer, plInt32 value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Int32, sName, bGlobalName);
  ref_writer.WriteInt32(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreInt64(plOpenDdlWriter& ref_writer, plInt64 value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::Int64, sName, bGlobalName);
  ref_writer.WriteInt64(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreUInt8(plOpenDdlWriter& ref_writer, plUInt8 value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt8, sName, bGlobalName);
  ref_writer.WriteUInt8(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreUInt16(plOpenDdlWriter& ref_writer, plUInt16 value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt16, sName, bGlobalName);
  ref_writer.WriteUInt16(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreUInt32(plOpenDdlWriter& ref_writer, plUInt32 value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt32, sName, bGlobalName);
  ref_writer.WriteUInt32(&value);
  ref_writer.EndPrimitiveList();
}

void plOpenDdlUtils::StoreUInt64(plOpenDdlWriter& ref_writer, plUInt64 value, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(plOpenDdlPrimitiveType::UInt64, sName, bGlobalName);
  ref_writer.WriteUInt64(&value);
  ref_writer.EndPrimitiveList();
}

PLASMA_FOUNDATION_DLL void plOpenDdlUtils::StoreInvalid(plOpenDdlWriter& ref_writer, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Invalid", sName, bGlobalName, true);
  ref_writer.EndObject();
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OpenDdlUtils);
