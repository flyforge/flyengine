#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>

struct plBlackboardTemplateAssetObject : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plBlackboardTemplateAssetObject, plReflectedClass);

  plDynamicArray<plString> m_BaseTemplates;
  plDynamicArray<plBlackboardEntry> m_Entries;
};

class plBlackboardTemplateAssetDocument : public plSimpleAssetDocument<plBlackboardTemplateAssetObject>
{
  PL_ADD_DYNAMIC_REFLECTION(plBlackboardTemplateAssetDocument, plSimpleAssetDocument<plBlackboardTemplateAssetObject>);

public:
  plBlackboardTemplateAssetDocument(plStringView sDocumentPath);

  plStatus WriteAsset(plStreamWriter& inout_stream, const plPlatformProfile* pAssetProfile) const;

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& inout_stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus RetrieveState(const plBlackboardTemplateAssetObject* pProp, plBlackboardTemplateResourceDescriptor& inout_Desc) const;
};
