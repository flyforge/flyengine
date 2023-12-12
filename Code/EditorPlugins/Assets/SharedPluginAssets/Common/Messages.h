#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

class PLASMA_SHAREDPLUGINASSETS_DLL PlasmaEditorEngineRestartSimulationMsg : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorEngineRestartSimulationMsg, PlasmaEditorEngineDocumentMsg);

public:
};

class PLASMA_SHAREDPLUGINASSETS_DLL PlasmaEditorEngineLoopAnimationMsg : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorEngineLoopAnimationMsg, PlasmaEditorEngineDocumentMsg);

public:
  bool m_bLoop;
};

class PLASMA_SHAREDPLUGINASSETS_DLL PlasmaEditorEngineSetMaterialsMsg : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorEngineSetMaterialsMsg, PlasmaEditorEngineDocumentMsg);

public:
  plHybridArray<plString, 16> m_Materials;
};
