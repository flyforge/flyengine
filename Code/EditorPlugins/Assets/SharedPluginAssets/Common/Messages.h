#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

class PLASMA_SHAREDPLUGINASSETS_DLL plEditorEngineRestartSimulationMsg : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditorEngineRestartSimulationMsg, plEditorEngineDocumentMsg);

public:
};

class PLASMA_SHAREDPLUGINASSETS_DLL plEditorEngineLoopAnimationMsg : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditorEngineLoopAnimationMsg, plEditorEngineDocumentMsg);

public:
  bool m_bLoop;
};

class PLASMA_SHAREDPLUGINASSETS_DLL plEditorEngineSetMaterialsMsg : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditorEngineSetMaterialsMsg, plEditorEngineDocumentMsg);

public:
  plHybridArray<plString, 16> m_Materials;
};
