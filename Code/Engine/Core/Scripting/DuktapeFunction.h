#pragma once

#include <Core/CoreDLL.h>
#include <Core/Scripting/DuktapeHelper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

class PL_CORE_DLL plDuktapeFunction final : public plDuktapeHelper
{
public:
  plDuktapeFunction(duk_context* pExistingContext);
  ~plDuktapeFunction();

  /// \name Retrieving function parameters
  ///@{

  /// Returns how many Parameters were passed to the called C-Function.
  plUInt32 GetNumVarArgFunctionParameters() const;

  plInt16 GetFunctionMagicValue() const;

  ///@}

  /// \name Returning values from C function
  ///@{

  plInt32 ReturnVoid();
  plInt32 ReturnNull();
  plInt32 ReturnUndefined();
  plInt32 ReturnBool(bool value);
  plInt32 ReturnInt(plInt32 value);
  plInt32 ReturnUInt(plUInt32 value);
  plInt32 ReturnFloat(float value);
  plInt32 ReturnNumber(double value);
  plInt32 ReturnString(plStringView value);
  plInt32 ReturnCustom();

  ///@}

private:
  bool m_bDidReturnValue = false;
};

#endif
