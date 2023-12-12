#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

void plToolsSerializationUtils::SerializeTypes(const plSet<const plRTTI*>& types, plAbstractObjectGraph& typesGraph)
{
  plRttiConverterContext context;
  plRttiConverterWriter rttiConverter(&typesGraph, &context, true, true);
  for (const plRTTI* pType : types)
  {
    plReflectedTypeDescriptor desc;
    if (pType->GetTypeFlags().IsSet(plTypeFlags::Phantom))
    {
      plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pType, desc);
    }
    else
    {
      plToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(pType, desc);
    }

    context.RegisterObject(plUuid::StableUuidForString(pType->GetTypeName()), plGetStaticRTTI<plReflectedTypeDescriptor>(), &desc);
    rttiConverter.AddObjectToGraph(plGetStaticRTTI<plReflectedTypeDescriptor>(), &desc);
  }
}

void plToolsSerializationUtils::CopyProperties(const plDocumentObject* pSource, const plDocumentObjectManager* pSourceManager, void* pTarget, const plRTTI* pTargetType, FilterFunction PropertFilter)
{
  plAbstractObjectGraph graph;
  plDocumentObjectConverterWriter writer(&graph, pSourceManager, [](const plDocumentObject*, const plAbstractProperty* p) { return p->GetAttributeByType<plHiddenAttribute>() == nullptr; });
  plAbstractObjectNode* pAbstractObj = writer.AddObjectToGraph(pSource);

  plRttiConverterContext context;
  plRttiConverterReader reader(&graph, &context);

  reader.ApplyPropertiesToObject(pAbstractObj, pTargetType, pTarget);
}
