#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

plHashTable<plUInt32, plTypeScriptBinding::FunctionBinding> plTypeScriptBinding::s_BoundFunctions;

static int __CPP_ComponentFunction_Call(duk_context* pDuk);

plResult plTypeScriptBinding::Init_FunctionBinding()
{
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_ComponentFunction_Call", __CPP_ComponentFunction_Call);

  return PLASMA_SUCCESS;
}

plUInt32 plTypeScriptBinding::ComputeFunctionBindingHash(const plRTTI* pType, const plAbstractFunctionProperty* pFunc)
{
  plStringBuilder sFuncName;

  sFuncName.Set(pType->GetTypeName(), "::", pFunc->GetPropertyName());

  return plHashingUtils::StringHashTo32(plHashingUtils::StringHash(sFuncName.GetData()));
}

void plTypeScriptBinding::SetupRttiFunctionBindings()
{
  if (!s_BoundFunctions.IsEmpty())
    return;

  plRTTI::ForEachDerivedType<plComponent>(
    [&](const plRTTI* pRtti) {
      for (const plAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
      {
        // TODO: static members ?
        if (pFunc->GetFunctionType() != plFunctionType::Member)
          continue;

        const plUInt32 uiHash = ComputeFunctionBindingHash(pRtti, pFunc);
        if (auto pExistingBinding = s_BoundFunctions.GetValue(uiHash))
        {
          PLASMA_ASSERT_DEV(plStringUtils::IsEqual(pExistingBinding->m_pFunc->GetPropertyName(), pFunc->GetPropertyName()), "Hash collision for bound function name!");
        }

        s_BoundFunctions[uiHash].m_pFunc = pFunc;
      }
    });
}

const char* plTypeScriptBinding::TsType(const plRTTI* pRtti)
{
  if (pRtti == nullptr)
    return "void";

  static plStringBuilder res;

  if (pRtti->IsDerivedFrom<plEnumBase>())
  {
    s_RequiredEnums.Insert(pRtti);

    res = pRtti->GetTypeName();
    res.TrimWordStart("pl");
    res.Prepend("Enum.");

    return res;
  }

  if (pRtti->IsDerivedFrom<plBitflagsBase>())
  {
    s_RequiredFlags.Insert(pRtti);

    res = pRtti->GetTypeName();
    res.TrimWordStart("pl");
    res.Prepend("Flags.");

    return res;
  }

  switch (pRtti->GetVariantType())
  {
    case plVariant::Type::Invalid:
    {
      if (pRtti->GetTypeName() == "plVariant")
        return "any";

      return nullptr;
    }

    case plVariant::Type::Angle:
      return "number";

    case plVariant::Type::Bool:
      return "boolean";

    case plVariant::Type::Int8:
    case plVariant::Type::UInt8:
    case plVariant::Type::Int16:
    case plVariant::Type::UInt16:
    case plVariant::Type::Int32:
    case plVariant::Type::UInt32:
    case plVariant::Type::Int64:
    case plVariant::Type::UInt64:
    case plVariant::Type::Float:
    case plVariant::Type::Double:
      return "number";

    case plVariant::Type::Color:
    case plVariant::Type::ColorGamma:
      return "Color";

    case plVariant::Type::Vector2:
    case plVariant::Type::Vector2I:
    case plVariant::Type::Vector2U:
      return "Vec2";

    case plVariant::Type::Vector3:
    case plVariant::Type::Vector3I:
    case plVariant::Type::Vector3U:
      return "Vec3";

    case plVariant::Type::Quaternion:
      return "Quat";


    case plVariant::Type::String:
    case plVariant::Type::StringView:
    case plVariant::Type::HashedString:
      return "string";

    case plVariant::Type::Time:
      return "number";

    case plVariant::Type::Transform:
      return "Transform";

    case plVariant::Type::Matrix3:
      return "Mat3";

    case plVariant::Type::Matrix4:
      return "Mat4";

      // TODO: implement these types
      // case plVariant::Type::Vector4:
      // case plVariant::Type::Vector4I:
      // case plVariant::Type::Vector4U:

    default:
      return nullptr;
  }
}

void plTypeScriptBinding::GenerateExposedFunctionsCode(plStringBuilder& out_Code, const plRTTI* pRtti)
{
  plStringBuilder sFunc;

  for (const plAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
  {
    // TODO: static members ?
    if (pFunc->GetFunctionType() != plFunctionType::Member)
      continue;

    // don't support functions with that many arguments
    if (pFunc->GetArgumentCount() > 16)
      continue;

    const plScriptableFunctionAttribute* pAttr = pFunc->GetAttributeByType<plScriptableFunctionAttribute>();

    if (pAttr == nullptr)
      goto ignore;

    sFunc.Set("  ", pFunc->GetPropertyName(), "(");

    for (plUInt32 i = 0; i < pFunc->GetArgumentCount(); ++i)
    {
      const char* szType = TsType(pFunc->GetArgumentType(i));

      if (szType == nullptr)
        goto ignore;

      sFunc.Append(i > 0 ? ", " : "", pAttr->GetArgumentName(i), ": ", szType);
    }

    sFunc.Append("): ");

    {
      const bool bHasReturnValue = pFunc->GetReturnType() != nullptr;

      {
        const char* szType = TsType(pFunc->GetReturnType());

        if (szType == nullptr)
          goto ignore;

        sFunc.Append(szType);
      }

      // function body
      {
        plUInt32 uiFuncHash = ComputeFunctionBindingHash(pRtti, pFunc);

        if (bHasReturnValue)
          sFunc.AppendFormat(" { return __CPP_ComponentFunction_Call(this, {0}", uiFuncHash);
        else
          sFunc.AppendFormat(" { __CPP_ComponentFunction_Call(this, {0}", uiFuncHash);

        for (plUInt32 arg = 0; arg < pFunc->GetArgumentCount(); ++arg)
        {
          sFunc.Append(", ", pAttr->GetArgumentName(arg));
        }

        sFunc.Append("); }\n");
      }
    }

    out_Code.Append(sFunc.GetView());

  ignore:
    continue;
  }
}

const plTypeScriptBinding::FunctionBinding* plTypeScriptBinding::FindFunctionBinding(plUInt32 uiFunctionHash)
{
  const FunctionBinding* pBinding = nullptr;
  s_BoundFunctions.TryGetValue(uiFunctionHash, pBinding);
  return pBinding;
}

int __CPP_ComponentFunction_Call(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plComponent* pComponent = plTypeScriptBinding::ExpectComponent<plComponent>(pDuk);

  const plUInt32 uiFuncHash = duk.GetUIntValue(1);

  const plTypeScriptBinding::FunctionBinding* pBinding = plTypeScriptBinding::FindFunctionBinding(uiFuncHash);

  if (pBinding == nullptr)
  {
    plLog::Error("Bound function with hash {} not found.", uiFuncHash);
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), +1);
  }

  const plUInt32 uiNumArgs = pBinding->m_pFunc->GetArgumentCount();

  plVariant ret0;
  plStaticArray<plVariant, 16> args;
  args.SetCount(uiNumArgs);

  for (plUInt32 arg = 0; arg < uiNumArgs; ++arg)
  {
    args[arg] = plTypeScriptBinding::GetVariant(duk, 2 + arg, pBinding->m_pFunc->GetArgumentType(arg));
  }

  pBinding->m_pFunc->Execute(pComponent, args, ret0);

  if (pBinding->m_pFunc->GetReturnType() != nullptr)
  {
    plTypeScriptBinding::PushVariant(duk, ret0);
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
  }
  else
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
  }
}
