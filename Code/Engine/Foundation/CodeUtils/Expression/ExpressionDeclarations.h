#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Types/Variant.h>

class plStreamWriter;
class plStreamReader;

namespace plExpression
{
  struct Register
  {
    PLASMA_DECLARE_POD_TYPE();

    Register(){}; // NOLINT: using = default doesn't work here.

    union
    {
      plSimdVec4b b;
      plSimdVec4i i;
      plSimdVec4f f;
    };
  };

  struct RegisterType
  {
    using StorageType = plUInt8;

    enum Enum
    {
      Unknown,

      Bool,
      Int,
      Float,

      Count,

      Default = Float,
      MaxNumBits = 4,
    };

    static const char* GetName(Enum registerType);
  };

  using Output = plArrayPtr<Register>;
  using Inputs = plArrayPtr<plArrayPtr<const Register>>; // Inputs are in SOA form, means inner array contains all values for one input parameter, one for each instance.
  using GlobalData = plHashTable<plHashedString, plVariant>;

  /// \brief Describes an input or output stream for a expression VM
  struct StreamDesc
  {
    plHashedString m_sName;
    plProcessingStream::DataType m_DataType;

    bool operator==(const StreamDesc& other) const
    {
      return m_sName == other.m_sName && m_DataType == other.m_DataType;
    }

    plResult Serialize(plStreamWriter& inout_stream) const;
    plResult Deserialize(plStreamReader& inout_stream);
  };

  /// \brief Describes an expression function and its signature, e.g. how many input parameter it has and their type
  struct FunctionDesc
  {
    plHashedString m_sName;
    plSmallArray<plEnum<plExpression::RegisterType>, 8> m_InputTypes;
    plUInt8 m_uiNumRequiredInputs = 0;
    plEnum<plExpression::RegisterType> m_OutputType;

    bool operator==(const FunctionDesc& other) const
    {
      return m_sName == other.m_sName &&
             m_InputTypes == other.m_InputTypes &&
             m_uiNumRequiredInputs == other.m_uiNumRequiredInputs &&
             m_OutputType == other.m_OutputType;
    }

    bool operator<(const FunctionDesc& other) const;

    plResult Serialize(plStreamWriter& inout_stream) const;
    plResult Deserialize(plStreamReader& inout_stream);

    plHashedString GetMangledName() const;
  };

  using Function = void (*)(plExpression::Inputs, plExpression::Output, const plExpression::GlobalData&);
  using ValidateGlobalDataFunction = plResult (*)(const plExpression::GlobalData&);

} // namespace plExpression

/// \brief Describes an external function that can be called in expressions.
///  These functions need to be state-less and thread-safe.
struct plExpressionFunction
{
  plExpression::FunctionDesc m_Desc;

  plExpression::Function m_Func;

  // Optional validation function used to validate required global data for an expression function
  plExpression::ValidateGlobalDataFunction m_ValidateGlobalDataFunc;
};

struct PLASMA_FOUNDATION_DLL plDefaultExpressionFunctions
{
  static plExpressionFunction s_RandomFunc;
  static plExpressionFunction s_PerlinNoiseFunc;
};
