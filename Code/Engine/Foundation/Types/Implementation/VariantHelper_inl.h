

// for some reason MSVC does not accept the template keyword here
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC_PURE)
#  define CALL_FUNCTOR(functor, type) return functor.operator()<type>(std::forward<Args>(args)...)
#else
#  define CALL_FUNCTOR(functor, type) return functor.template operator()<type>(std::forward<Args>(args)...)
#endif

template <typename Functor, class... Args>
auto plVariant::DispatchTo(Functor& ref_functor, Type::Enum type, Args&&... args)
{
  switch (type)
  {
    case Type::Bool:
      CALL_FUNCTOR(ref_functor, bool);
      break;

    case Type::Int8:
      CALL_FUNCTOR(ref_functor, plInt8);
      break;

    case Type::UInt8:
      CALL_FUNCTOR(ref_functor, plUInt8);
      break;

    case Type::Int16:
      CALL_FUNCTOR(ref_functor, plInt16);
      break;

    case Type::UInt16:
      CALL_FUNCTOR(ref_functor, plUInt16);
      break;

    case Type::Int32:
      CALL_FUNCTOR(ref_functor, plInt32);
      break;

    case Type::UInt32:
      CALL_FUNCTOR(ref_functor, plUInt32);
      break;

    case Type::Int64:
      CALL_FUNCTOR(ref_functor, plInt64);
      break;

    case Type::UInt64:
      CALL_FUNCTOR(ref_functor, plUInt64);
      break;

    case Type::Float:
      CALL_FUNCTOR(ref_functor, float);
      break;

    case Type::Double:
      CALL_FUNCTOR(ref_functor, double);
      break;

    case Type::Color:
      CALL_FUNCTOR(ref_functor, plColor);
      break;

    case Type::ColorGamma:
      CALL_FUNCTOR(ref_functor, plColorGammaUB);
      break;

    case Type::Vector2:
      CALL_FUNCTOR(ref_functor, plVec2);
      break;

    case Type::Vector3:
      CALL_FUNCTOR(ref_functor, plVec3);
      break;

    case Type::Vector4:
      CALL_FUNCTOR(ref_functor, plVec4);
      break;

    case Type::Vector2I:
      CALL_FUNCTOR(ref_functor, plVec2I32);
      break;

    case Type::Vector3I:
      CALL_FUNCTOR(ref_functor, plVec3I32);
      break;

    case Type::Vector4I:
      CALL_FUNCTOR(ref_functor, plVec4I32);
      break;

    case Type::Vector2U:
      CALL_FUNCTOR(ref_functor, plVec2U32);
      break;

    case Type::Vector3U:
      CALL_FUNCTOR(ref_functor, plVec3U32);
      break;

    case Type::Vector4U:
      CALL_FUNCTOR(ref_functor, plVec4U32);
      break;

    case Type::Quaternion:
      CALL_FUNCTOR(ref_functor, plQuat);
      break;

    case Type::Matrix3:
      CALL_FUNCTOR(ref_functor, plMat3);
      break;

    case Type::Matrix4:
      CALL_FUNCTOR(ref_functor, plMat4);
      break;

    case Type::Transform:
      CALL_FUNCTOR(ref_functor, plTransform);
      break;

    case Type::String:
      CALL_FUNCTOR(ref_functor, plString);
      break;

    case Type::StringView:
      CALL_FUNCTOR(ref_functor, plStringView);
      break;

    case Type::DataBuffer:
      CALL_FUNCTOR(ref_functor, plDataBuffer);
      break;

    case Type::Time:
      CALL_FUNCTOR(ref_functor, plTime);
      break;

    case Type::Uuid:
      CALL_FUNCTOR(ref_functor, plUuid);
      break;

    case Type::Angle:
      CALL_FUNCTOR(ref_functor, plAngle);
      break;

    case Type::HashedString:
      CALL_FUNCTOR(ref_functor, plHashedString);
      break;

    case Type::TempHashedString:
      CALL_FUNCTOR(ref_functor, plTempHashedString);
      break;

    case Type::VariantArray:
      CALL_FUNCTOR(ref_functor, plVariantArray);
      break;

    case Type::VariantDictionary:
      CALL_FUNCTOR(ref_functor, plVariantDictionary);
      break;

    case Type::TypedObject:
      CALL_FUNCTOR(ref_functor, plTypedObject);
      break;

    default:
      PLASMA_REPORT_FAILURE("Could not dispatch type '{0}'", type);
      // Intended fall through to disable warning.
    case Type::TypedPointer:
      CALL_FUNCTOR(ref_functor, plTypedPointer);
      break;
  }
}

#undef CALL_FUNCTOR

class plVariantHelper
{
  friend class plVariant;
  friend struct ConvertFunc;

  static void To(const plVariant& value, bool& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= plVariant::Type::Double)
      result = value.ConvertNumber<plInt32>() != 0;
    else if (value.GetType() == plVariant::Type::String || value.GetType() == plVariant::Type::HashedString)
    {
      plStringView s = value.IsA<plString>() ? value.Cast<plString>().GetView() : value.Cast<plHashedString>().GetView();
      if (plConversionUtils::StringToBool(s, result) == PLASMA_FAILURE)
      {
        result = false;
        bSuccessful = false;
      }
    }
    else
      PLASMA_REPORT_FAILURE("Conversion to bool failed");
  }

  static void To(const plVariant& value, plInt8& result, bool& bSuccessful)
  {
    plInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (plInt8)tempResult;
  }

  static void To(const plVariant& value, plUInt8& result, bool& bSuccessful)
  {
    plUInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (plUInt8)tempResult;
  }

  static void To(const plVariant& value, plInt16& result, bool& bSuccessful)
  {
    plInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (plInt16)tempResult;
  }

  static void To(const plVariant& value, plUInt16& result, bool& bSuccessful)
  {
    plUInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (plUInt16)tempResult;
  }

  static void To(const plVariant& value, plInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= plVariant::Type::Double)
      result = value.ConvertNumber<plInt32>();
    else if (value.GetType() == plVariant::Type::String || value.GetType() == plVariant::Type::HashedString)
    {
      plStringView s = value.IsA<plString>() ? value.Cast<plString>().GetView() : value.Cast<plHashedString>().GetView();
      if (plConversionUtils::StringToInt(s, result) == PLASMA_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
    }
    else
      PLASMA_REPORT_FAILURE("Conversion to int failed");
  }

  static void To(const plVariant& value, plUInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= plVariant::Type::Double)
      result = value.ConvertNumber<plUInt32>();
    else if (value.GetType() == plVariant::Type::String || value.GetType() == plVariant::Type::HashedString)
    {
      plStringView s = value.IsA<plString>() ? value.Cast<plString>().GetView() : value.Cast<plHashedString>().GetView();
      plInt64 tmp = result;
      if (plConversionUtils::StringToInt64(s, tmp) == PLASMA_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
      else
        result = (plUInt32)tmp;
    }
    else
      PLASMA_REPORT_FAILURE("Conversion to uint failed");
  }

  static void To(const plVariant& value, plInt64& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= plVariant::Type::Double)
      result = value.ConvertNumber<plInt64>();
    else if (value.GetType() == plVariant::Type::String || value.GetType() == plVariant::Type::HashedString)
    {
      plStringView s = value.IsA<plString>() ? value.Cast<plString>().GetView() : value.Cast<plHashedString>().GetView();
      if (plConversionUtils::StringToInt64(s, result) == PLASMA_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
    }
    else
      PLASMA_REPORT_FAILURE("Conversion to int64 failed");
  }

  static void To(const plVariant& value, plUInt64& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= plVariant::Type::Double)
      result = value.ConvertNumber<plUInt64>();
    else if (value.GetType() == plVariant::Type::String || value.GetType() == plVariant::Type::HashedString)
    {
      plStringView s = value.IsA<plString>() ? value.Cast<plString>().GetView() : value.Cast<plHashedString>().GetView();
      plInt64 tmp = result;
      if (plConversionUtils::StringToInt64(s, tmp) == PLASMA_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
      else
        result = (plUInt64)tmp;
    }
    else
      PLASMA_REPORT_FAILURE("Conversion to uint64 failed");
  }

  static void To(const plVariant& value, float& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= plVariant::Type::Double)
      result = value.ConvertNumber<float>();
    else if (value.GetType() == plVariant::Type::String || value.GetType() == plVariant::Type::HashedString)
    {
      plStringView s = value.IsA<plString>() ? value.Cast<plString>().GetView() : value.Cast<plHashedString>().GetView();
      double tmp = result;
      if (plConversionUtils::StringToFloat(s, tmp) == PLASMA_FAILURE)
      {
        result = 0.0f;
        bSuccessful = false;
      }
      else
        result = (float)tmp;
    }
    else
      PLASMA_REPORT_FAILURE("Conversion to float failed");
  }

  static void To(const plVariant& value, double& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= plVariant::Type::Double)
      result = value.ConvertNumber<double>();
    else if (value.GetType() == plVariant::Type::String || value.GetType() == plVariant::Type::HashedString)
    {
      plStringView s = value.IsA<plString>() ? value.Cast<plString>().GetView() : value.Cast<plHashedString>().GetView();
      if (plConversionUtils::StringToFloat(s, result) == PLASMA_FAILURE)
      {
        result = 0.0;
        bSuccessful = false;
      }
    }
    else
      PLASMA_REPORT_FAILURE("Conversion to double failed");
  }

  static void To(const plVariant& value, plString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsValid() == false)
    {
      result = "<Invalid>";
      return;
    }

    ToStringFunc toStringFunc;
    toStringFunc.m_pThis = &value;
    toStringFunc.m_pResult = &result;

    plVariant::DispatchTo(toStringFunc, value.GetType());
  }

  static void To(const plVariant& value, plStringView& result, bool& bSuccessful)
  {
    bSuccessful = true;

    result = value.IsA<plString>() ? value.Get<plString>().GetView() : value.Get<plHashedString>().GetView();
  }

  static void To(const plVariant& value, plTypedPointer& result, bool& bSuccessful)
  {
    bSuccessful = true;
    PLASMA_ASSERT_DEBUG(value.GetType() == plVariant::Type::TypedPointer, "Only ptr can be converted to void*!");
    result = value.Cast<plTypedPointer>();
  }

  static void To(const plVariant& value, plColor& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == plVariant::Type::ColorGamma)
      result = value.Cast<plColorGammaUB>();
    else
      PLASMA_REPORT_FAILURE("Conversion to plColor failed");
  }

  static void To(const plVariant& value, plColorGammaUB& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == plVariant::Type::Color)
      result = value.Cast<plColor>();
    else
      PLASMA_REPORT_FAILURE("Conversion to plColorGammaUB failed");
  }

  template <typename T, typename V1, typename V2>
  static void ToVec2X(const plVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else
    {
      PLASMA_REPORT_FAILURE("Conversion to plVec2X failed");
      bSuccessful = false;
    }
  }

  static void To(const plVariant& value, plVec2& result, bool& bSuccessful) { ToVec2X<plVec2, plVec2I32, plVec2U32>(value, result, bSuccessful); }

  static void To(const plVariant& value, plVec2I32& result, bool& bSuccessful) { ToVec2X<plVec2I32, plVec2, plVec2U32>(value, result, bSuccessful); }

  static void To(const plVariant& value, plVec2U32& result, bool& bSuccessful) { ToVec2X<plVec2U32, plVec2I32, plVec2>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2>
  static void ToVec3X(const plVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else
    {
      PLASMA_REPORT_FAILURE("Conversion to plVec3X failed");
      bSuccessful = false;
    }
  }

  static void To(const plVariant& value, plVec3& result, bool& bSuccessful) { ToVec3X<plVec3, plVec3I32, plVec3U32>(value, result, bSuccessful); }

  static void To(const plVariant& value, plVec3I32& result, bool& bSuccessful) { ToVec3X<plVec3I32, plVec3, plVec3U32>(value, result, bSuccessful); }

  static void To(const plVariant& value, plVec3U32& result, bool& bSuccessful) { ToVec3X<plVec3U32, plVec3I32, plVec3>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2>
  static void ToVec4X(const plVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else
    {
      PLASMA_REPORT_FAILURE("Conversion to plVec4X failed");
      bSuccessful = false;
    }
  }

  static void To(const plVariant& value, plVec4& result, bool& bSuccessful) { ToVec4X<plVec4, plVec4I32, plVec4U32>(value, result, bSuccessful); }

  static void To(const plVariant& value, plVec4I32& result, bool& bSuccessful) { ToVec4X<plVec4I32, plVec4, plVec4U32>(value, result, bSuccessful); }

  static void To(const plVariant& value, plVec4U32& result, bool& bSuccessful) { ToVec4X<plVec4U32, plVec4I32, plVec4>(value, result, bSuccessful); }

  static void To(const plVariant& value, plHashedString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == plVariantType::String)
      result.Assign(value.Cast<plString>());
    else if (value.GetType() == plVariantType::StringView)
      result.Assign(value.Cast<plStringView>());
    else
    {
      plString s;
      To(value, s, bSuccessful);
      result.Assign(s.GetView());
    }
  }

  static void To(const plVariant& value, plTempHashedString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == plVariantType::String)
      result = value.Cast<plString>();
    else if (value.GetType() == plVariantType::StringView)
      result = value.Cast<plStringView>();
    else if (value.GetType() == plVariant::Type::HashedString)
      result = value.Cast<plHashedString>();
    else
    {
      plString s;
      To(value, s, bSuccessful);
      result = s.GetView();
    }
  }

  template <typename T>
  static void To(const plVariant& value, T& result, bool& bSuccessful)
  {
    PLASMA_REPORT_FAILURE("Conversion function not implemented for target type '{0}'", plVariant::TypeDeduction<T>::value);
    bSuccessful = false;
  }

  struct ToStringFunc
  {
    template <typename T>
    PLASMA_ALWAYS_INLINE void operator()()
    {
      plStringBuilder tmp;
      *m_pResult = plConversionUtils::ToString(m_pThis->Cast<T>(), tmp);
    }

    const plVariant* m_pThis;
    plString* m_pResult;
  };
};
