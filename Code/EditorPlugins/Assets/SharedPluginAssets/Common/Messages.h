#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

class PL_SHAREDPLUGINASSETS_DLL plEditorEngineRestartSimulationMsg : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plEditorEngineRestartSimulationMsg, plEditorEngineDocumentMsg);

public:
};

class PL_SHAREDPLUGINASSETS_DLL plEditorEngineLoopAnimationMsg : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plEditorEngineLoopAnimationMsg, plEditorEngineDocumentMsg);

public:
  bool m_bLoop;
};

class PL_SHAREDPLUGINASSETS_DLL plEditorEngineSetMaterialsMsg : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plEditorEngineSetMaterialsMsg, plEditorEngineDocumentMsg);

public:
  plHybridArray<plString, 16> m_Materials;
};
