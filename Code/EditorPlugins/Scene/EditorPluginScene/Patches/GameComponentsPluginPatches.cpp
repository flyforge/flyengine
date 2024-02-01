#include <EditorPluginScene/EditorPluginScenePCH.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plFakeRopeComponentPatch_2_3 : public plGraphPatch
{
public:
  plFakeRopeComponentPatch_2_3()
    : plGraphPatch("plFakeRopeComponent", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

plFakeRopeComponentPatch_2_3 g_plFakeRopeComponentPatch_2_3;
