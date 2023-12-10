#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

class plQtPropertyGridWidget;
class plAbstractProperty;

struct plPropertyEvent
{
  enum class Type
  {
    SingleValueChanged,
    BeginTemporary,
    EndTemporary,
    CancelTemporary,
  };

  Type m_Type;
  const plAbstractProperty* m_pProperty;
  const plHybridArray<plPropertySelection, 8>* m_pItems;
  plVariant m_Value;
};
