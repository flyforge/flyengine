#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>

struct plBlackboardTemplateAssetObject : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBlackboardTemplateAssetObject, plReflectedClass);

  plDynamicArray<plString> m_BaseTemplates;
  plDynamicArray<plBlackboardEntry> m_Entries;
};

class plBlackboardTemplateAssetDocument : public plSimpleAssetDocument<plBlackboardTemplateAssetObject>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBlackboardTemplateAssetDocument, plSimpleAssetDocument<plBlackboardTemplateAssetObject>);

public:
  plBlackboardTemplateAssetDocument(const char* szDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus RetrieveState(const plBlackboardTemplateAssetObject* pProp, plBlackboardTemplateResourceDescriptor& inout_Desc);
};
