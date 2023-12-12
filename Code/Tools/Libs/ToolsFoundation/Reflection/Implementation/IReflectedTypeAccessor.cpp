#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

bool plIReflectedTypeAccessor::GetValues(const char* szProperty, plDynamicArray<plVariant>& out_values) const
{
  plHybridArray<plVariant, 16> keys;
  if (!GetKeys(szProperty, keys))
    return false;

  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (plVariant key : keys)
  {
    out_values.PushBack(GetValue(szProperty, key));
  }
  return true;
}
