#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>


plApplyNativePropertyChangesContext::plApplyNativePropertyChangesContext(plRttiConverterContext& ref_source, const plAbstractObjectGraph& originalGraph)
  : m_NativeContext(ref_source)
  , m_OriginalGraph(originalGraph)
{
}

plUuid plApplyNativePropertyChangesContext::GenerateObjectGuid(const plUuid& parentGuid, const plAbstractProperty* pProp, plVariant index, void* pObject) const
{
  if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
  {
    // If the object is already known by the native context (a pointer that existed before the native changes)
    // we can just return it. Any other pointer will get a new guid assigned.
    plUuid guid = m_NativeContext.GetObjectGUID(pProp->GetSpecificType(), pObject);
    if (guid.IsValid())
      return guid;
  }
  else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
  {
    // In case of by-value classes we lookup the guid in the object manager graph by using
    // the index as the identify of the object. If the index is not valid (e.g. the array was expanded by native changes)
    // a new guid is assigned.
    if (const plAbstractObjectNode* originalNode = m_OriginalGraph.GetNode(parentGuid))
    {
      if (const plAbstractObjectNode::Property* originalProp = originalNode->FindProperty(pProp->GetPropertyName()))
      {
        switch (pProp->GetCategory())
        {
          case plPropertyCategory::Member:
          {
            if (originalProp->m_Value.IsA<plUuid>() && originalProp->m_Value.Get<plUuid>().IsValid())
              return originalProp->m_Value.Get<plUuid>();
          }
          break;
          case plPropertyCategory::Array:
          {
            plUInt32 uiIndex = index.Get<plUInt32>();
            if (originalProp->m_Value.IsA<plVariantArray>())
            {
              const plVariantArray& values = originalProp->m_Value.Get<plVariantArray>();
              if (uiIndex < values.GetCount())
              {
                const auto& originalElemValue = values[uiIndex];
                if (originalElemValue.IsA<plUuid>() && originalElemValue.Get<plUuid>().IsValid())
                  return originalElemValue.Get<plUuid>();
              }
            }
          }
          break;
          case plPropertyCategory::Map:
          {
            const plString& sIndex = index.Get<plString>();
            if (originalProp->m_Value.IsA<plVariantDictionary>())
            {
              const plVariantDictionary& values = originalProp->m_Value.Get<plVariantDictionary>();
              if (values.Contains(sIndex))
              {
                const auto& originalElemValue = *values.GetValue(sIndex);
                if (originalElemValue.IsA<plUuid>() && originalElemValue.Get<plUuid>().IsValid())
                  return originalElemValue.Get<plUuid>();
              }
            }
          }
          break;

          default:
            break;
        }
      }
    }
  }

  return plUuid::MakeUuid();
}


