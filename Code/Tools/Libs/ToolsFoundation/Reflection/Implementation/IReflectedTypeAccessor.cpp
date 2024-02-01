#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

bool plIReflectedTypeAccessor::GetValues(plStringView sProperty, plDynamicArray<plVariant>& out_values) const
{
  plHybridArray<plVariant, 16> keys;
  if (!GetKeys(sProperty, keys))
    return false;

  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (plVariant key : keys)
  {
    out_values.PushBack(GetValue(sProperty, key));
  }
  return true;
}
