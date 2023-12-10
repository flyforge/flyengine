#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EnginePluginScene/Components/CommentComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plCommentComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Comment", GetComment, SetComment),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Editing Utilities"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCommentComponent::plCommentComponent() = default;
plCommentComponent::~plCommentComponent() = default;

void plCommentComponent::SetComment(const char* text)
{
  m_sComment.Assign(text);
}

const char* plCommentComponent::GetComment() const
{
  return m_sComment.GetString();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier_RemoveCommentComponents, 1, plRTTIDefaultAllocator<plSceneExportModifier_RemoveCommentComponents>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSceneExportModifier_RemoveCommentComponents::ModifyWorld(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
  PLASMA_LOCK(world.GetWriteMarker());

  if (plCommentComponentManager* pMan = world.GetComponentManager<plCommentComponentManager>())
  {
    for (auto it = pMan->GetComponents(); it.IsValid(); it.Next())
    {
      pMan->DeleteComponent(it->GetHandle());
    }
  }
}
