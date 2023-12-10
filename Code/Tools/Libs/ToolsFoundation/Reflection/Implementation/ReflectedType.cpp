#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAttributeHolder, plNoBase, 1, plRTTINoAllocator)
{
  flags.Add(plTypeFlags::Abstract);
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_ACCESSOR_PROPERTY("Attributes", GetCount, GetValue, SetValue, Insert, Remove)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plAttributeHolder::plAttributeHolder() = default;

plAttributeHolder::plAttributeHolder(const plAttributeHolder& rhs)
{
  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

plAttributeHolder::~plAttributeHolder()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<plPropertyAttribute*>(pAttr));
  }
}

void plAttributeHolder::operator=(const plAttributeHolder& rhs)
{
  if (this == &rhs)
    return;

  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

plUInt32 plAttributeHolder::GetCount() const
{
  return plMath::Max(m_ReferenceAttributes.GetCount(), m_Attributes.GetCount());
}

const plPropertyAttribute* plAttributeHolder::GetValue(plUInt32 uiIndex) const
{
  if (!m_ReferenceAttributes.IsEmpty())
    return m_ReferenceAttributes[uiIndex];

  return m_Attributes[uiIndex];
}

void plAttributeHolder::SetValue(plUInt32 uiIndex, const plPropertyAttribute* value)
{
  m_Attributes[uiIndex] = value;
}

void plAttributeHolder::Insert(plUInt32 uiIndex, const plPropertyAttribute* value)
{
  m_Attributes.Insert(value, uiIndex);
}

void plAttributeHolder::Remove(plUInt32 uiIndex)
{
  m_Attributes.RemoveAtAndCopy(uiIndex);
}

////////////////////////////////////////////////////////////////////////
// plReflectedPropertyDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plReflectedPropertyDescriptor, plAttributeHolder, 2, plRTTIDefaultAllocator<plReflectedPropertyDescriptor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Category", plPropertyCategory, m_Category),
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
    PLASMA_MEMBER_PROPERTY("Type", m_sType),
    PLASMA_BITFLAGS_MEMBER_PROPERTY("Flags", plPropertyFlags, m_Flags),
    PLASMA_MEMBER_PROPERTY("ConstantValue", m_ConstantValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

class plReflectedPropertyDescriptorPatch_1_2 : public plGraphPatch
{
public:
  plReflectedPropertyDescriptorPatch_1_2()
    : plGraphPatch("plReflectedPropertyDescriptor", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    if (plAbstractObjectNode::Property* pProp = pNode->FindProperty("Flags"))
    {
      plStringBuilder sValue = pProp->m_Value.Get<plString>();
      plHybridArray<plStringView, 32> values;
      sValue.Split(false, values, "|");

      plStringBuilder sNewValue;
      for (plInt32 i = (plInt32)values.GetCount() - 1; i >= 0; i--)
      {
        if (values[i].IsEqual("plPropertyFlags::Constant"))
        {
          values.RemoveAtAndCopy(i);
        }
        else if (values[i].IsEqual("plPropertyFlags::EmbeddedClass"))
        {
          values[i] = plStringView("plPropertyFlags::Class");
        }
        else if (values[i].IsEqual("plPropertyFlags::Pointer"))
        {
          values.PushBack(plStringView("plPropertyFlags::Class"));
        }
      }
      for (plUInt32 i = 0; i < values.GetCount(); ++i)
      {
        if (i != 0)
          sNewValue.Append("|");
        sNewValue.Append(values[i]);
      }
      pProp->m_Value = sNewValue.GetData();
    }
  }
};

plReflectedPropertyDescriptorPatch_1_2 g_plReflectedPropertyDescriptorPatch_1_2;


plReflectedPropertyDescriptor::plReflectedPropertyDescriptor(plPropertyCategory::Enum category, plStringView sName, plStringView sType, plBitflags<plPropertyFlags> flags)
  : m_Category(category)
  , m_sName(sName)
  , m_sType(sType)
  , m_Flags(flags)
{
}

plReflectedPropertyDescriptor::plReflectedPropertyDescriptor(plPropertyCategory::Enum category, plStringView sName, plStringView sType,
  plBitflags<plPropertyFlags> flags, plArrayPtr<const plPropertyAttribute* const> attributes)
  : m_Category(category)
  , m_sName(sName)
  , m_sType(sType)
  , m_Flags(flags)
{
  m_ReferenceAttributes = attributes;
}

plReflectedPropertyDescriptor::plReflectedPropertyDescriptor(
  plStringView sName, const plVariant& constantValue, plArrayPtr<const plPropertyAttribute* const> attributes)
  : m_Category(plPropertyCategory::Constant)
  , m_sName(sName)
  , m_sType()
  , m_Flags(plPropertyFlags::StandardType | plPropertyFlags::ReadOnly)
  , m_ConstantValue(constantValue)
{
  m_ReferenceAttributes = attributes;
  const plRTTI* pType = plReflectionUtils::GetTypeFromVariant(constantValue);
  if (pType)
    m_sType = pType->GetTypeName();
}

plReflectedPropertyDescriptor::plReflectedPropertyDescriptor(const plReflectedPropertyDescriptor& rhs)
{
  operator=(rhs);
}

void plReflectedPropertyDescriptor::operator=(const plReflectedPropertyDescriptor& rhs)
{
  m_Category = rhs.m_Category;
  m_sName = rhs.m_sName;

  m_sType = rhs.m_sType;

  m_Flags = rhs.m_Flags;
  m_ConstantValue = rhs.m_ConstantValue;

  plAttributeHolder::operator=(rhs);
}

plReflectedPropertyDescriptor::~plReflectedPropertyDescriptor() = default;


////////////////////////////////////////////////////////////////////////
// plFunctionParameterDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plFunctionArgumentDescriptor, plNoBase, 1, plRTTIDefaultAllocator<plFunctionArgumentDescriptor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Type", m_sType),
    PLASMA_BITFLAGS_MEMBER_PROPERTY("Flags", plPropertyFlags, m_Flags),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plFunctionArgumentDescriptor::plFunctionArgumentDescriptor() = default;

plFunctionArgumentDescriptor::plFunctionArgumentDescriptor(plStringView sType, plBitflags<plPropertyFlags> flags)
  : m_sType(sType)
  , m_Flags(flags)
{
}


////////////////////////////////////////////////////////////////////////
// plReflectedFunctionDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plReflectedFunctionDescriptor, plAttributeHolder, 1, plRTTIDefaultAllocator<plReflectedFunctionDescriptor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
    PLASMA_BITFLAGS_MEMBER_PROPERTY("Flags", plPropertyFlags, m_Flags),
    PLASMA_ENUM_MEMBER_PROPERTY("Type", plFunctionType, m_Type),
    PLASMA_MEMBER_PROPERTY("ReturnValue", m_ReturnValue),
    PLASMA_ARRAY_MEMBER_PROPERTY("Arguments", m_Arguments),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plReflectedFunctionDescriptor::plReflectedFunctionDescriptor() = default;

plReflectedFunctionDescriptor::plReflectedFunctionDescriptor(plStringView sName, plBitflags<plPropertyFlags> flags, plEnum<plFunctionType> type, plArrayPtr<const plPropertyAttribute* const> attributes)
  : m_sName(sName)
  , m_Flags(flags)
  , m_Type(type)
{
  m_ReferenceAttributes = attributes;
}

plReflectedFunctionDescriptor::plReflectedFunctionDescriptor(const plReflectedFunctionDescriptor& rhs)
{
  operator=(rhs);
}

plReflectedFunctionDescriptor::~plReflectedFunctionDescriptor() = default;

void plReflectedFunctionDescriptor::operator=(const plReflectedFunctionDescriptor& rhs)
{
  m_sName = rhs.m_sName;
  m_Flags = rhs.m_Flags;
  m_Type = rhs.m_Type;
  m_ReturnValue = rhs.m_ReturnValue;
  m_Arguments = rhs.m_Arguments;
  plAttributeHolder::operator=(rhs);
}

////////////////////////////////////////////////////////////////////////
// plReflectedTypeDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plReflectedTypeDescriptor, plAttributeHolder, 1, plRTTIDefaultAllocator<plReflectedTypeDescriptor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("TypeName", m_sTypeName),
    PLASMA_MEMBER_PROPERTY("PluginName", m_sPluginName),
    PLASMA_MEMBER_PROPERTY("ParentTypeName", m_sParentTypeName),
    PLASMA_BITFLAGS_MEMBER_PROPERTY("Flags", plTypeFlags, m_Flags),
    PLASMA_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
    PLASMA_ARRAY_MEMBER_PROPERTY("Functions", m_Functions),
    PLASMA_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plReflectedTypeDescriptor::~plReflectedTypeDescriptor() = default;
