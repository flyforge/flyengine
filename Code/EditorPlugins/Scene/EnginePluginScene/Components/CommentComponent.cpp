#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EnginePluginScene/Components/CommentComponent.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plCommentComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Comment", GetComment, SetComment),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Editing"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCommentComponent::plCommentComponent() = default;
plCommentComponent::~plCommentComponent() = default;

void plCommentComponent::SetComment(const char* szText)
{
  m_sComment.Assign(szText);
}

const char* plCommentComponent::GetComment() const
{
  return m_sComment.GetString();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier_RemoveCommentComponents, 1, plRTTIDefaultAllocator<plSceneExportModifier_RemoveCommentComponents>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSceneExportModifier_RemoveCommentComponents::ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
  PL_LOCK(ref_world.GetWriteMarker());

  if (plCommentComponentManager* pMan = ref_world.GetComponentManager<plCommentComponentManager>())
  {
    for (auto it = pMan->GetComponents(); it.IsValid(); it.Next())
    {
      pMan->DeleteComponent(it->GetHandle());
    }
  }
}
