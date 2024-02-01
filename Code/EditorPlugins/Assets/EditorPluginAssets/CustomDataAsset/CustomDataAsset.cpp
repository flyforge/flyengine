#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCustomDataAssetProperties, 1, plRTTIDefaultAllocator<plCustomDataAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Type", m_pType)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCustomDataAssetDocument, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCustomDataAssetDocument::plCustomDataAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plCustomDataAssetProperties>(sDocumentPath, plAssetDocEngineConnection::None)
{
}

plTransformStatus plCustomDataAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
  const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plAbstractObjectGraph abstractObjectGraph;
  plDocumentObjectConverterWriter objectWriter(&abstractObjectGraph, GetObjectManager());

  plDocumentObject* pObject = GetPropertyObject();

  plVariant type = pObject->GetTypeAccessor().GetValue("Type");
  PL_ASSERT_DEV(type.IsA<plUuid>(), "Implementation error");

  if (plDocumentObject* pDataObject = pObject->GetChild(type.Get<plUuid>()))
  {
    plAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pDataObject, "root");
  }

  plAbstractGraphBinarySerializer::Write(stream, &abstractObjectGraph);
  return plStatus(PL_SUCCESS);
}
