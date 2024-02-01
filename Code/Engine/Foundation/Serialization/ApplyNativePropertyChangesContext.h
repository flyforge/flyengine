#pragma once

#include <Foundation/Serialization/RttiConverter.h>

/// \brief The plApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those of the plAbstractObjectGraph that was passed in. This allows native changes to be tracked and applied to the object graph at a later point.
/// \sa plAbstractObjectGraph::ModifyNodeViaNativeCounterpart
class PL_FOUNDATION_DLL plApplyNativePropertyChangesContext : public plRttiConverterContext
{
public:
  plApplyNativePropertyChangesContext(plRttiConverterContext& ref_source, const plAbstractObjectGraph& originalGraph);

  virtual plUuid GenerateObjectGuid(const plUuid& parentGuid, const plAbstractProperty* pProp, plVariant index, void* pObject) const override;

private:
  plRttiConverterContext& m_NativeContext;
  const plAbstractObjectGraph& m_OriginalGraph;
};
