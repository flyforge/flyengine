#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Types/Variant.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace plRmlUiConversionUtils
{
  plVariant ToVariant(const Rml::Variant& value, plVariant::Type::Enum targetType /*= plVariant::Type::Invalid*/)
  {
    plVariant result;

    switch (value.GetType())
    {
      case Rml::Variant::BOOL:
        result = value.Get<bool>();
        break;

      case Rml::Variant::CHAR:
        result = value.Get<char>();
        break;

      case Rml::Variant::BYTE:
        result = value.Get<Rml::byte>();
        break;

      case Rml::Variant::INT:
        result = value.Get<int>();
        break;

      case Rml::Variant::INT64:
        result = value.Get<plInt64>();
        break;

      case Rml::Variant::FLOAT:
        result = value.Get<float>();
        break;

      case Rml::Variant::DOUBLE:
        result = value.Get<double>();
        break;

      case Rml::Variant::STRING:
        result = value.Get<Rml::String>().c_str();
        break;

      default:
        break;
    }

    if (targetType != plVariant::Type::Invalid && result.IsValid())
    {
      plResult conversionResult = PL_SUCCESS;
      result = result.ConvertTo(targetType, &conversionResult);

      if (conversionResult.Failed())
      {
        plLog::Warning("Failed to convert rml variant to target type '{}'", targetType);
      }
    }

    return result;
  }

  Rml::Variant ToVariant(const plVariant& value)
  {
    switch (value.GetType())
    {
      case plVariant::Type::Invalid:
        return Rml::Variant("&lt;Invalid&gt;");

      case plVariant::Type::Bool:
        return Rml::Variant(value.Get<bool>());

      case plVariant::Type::Int8:
        return Rml::Variant(value.Get<plInt8>());

      case plVariant::Type::UInt8:
        return Rml::Variant(value.Get<plUInt8>());

      case plVariant::Type::Int16:
      case plVariant::Type::UInt16:
      case plVariant::Type::Int32:
        return Rml::Variant(value.ConvertTo<int>());

      case plVariant::Type::UInt32:
      case plVariant::Type::Int64:
        return Rml::Variant(static_cast<int64_t>(value.ConvertTo<plInt64>()));

      case plVariant::Type::Float:
        return Rml::Variant(value.Get<float>());

      case plVariant::Type::Double:
        return Rml::Variant(value.Get<double>());

      case plVariant::Type::String:
        return Rml::Variant(value.Get<plString>());

      default:
        PL_ASSERT_NOT_IMPLEMENTED;
        return Rml::Variant();
    }
  }

} // namespace plRmlUiConversionUtils
