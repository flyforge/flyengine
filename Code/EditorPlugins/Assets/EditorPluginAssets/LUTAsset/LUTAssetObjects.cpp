#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLUTAssetProperties, 1, plRTTIDefaultAllocator<plLUTAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Input", GetInputFile, SetInputFile)->AddAttributes(new plFileBrowserAttribute("Select CUBE file", "*.cube")),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plLUTAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plLUTAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    props["Input"].m_Visibility = plPropertyUiState::Default;
    props["Input"].m_sNewLabelText = "plLUTAssetProperties::CUBEfile";
  }
}

plString plLUTAssetProperties::GetAbsoluteInputFilePath() const
{
  plStringBuilder sPath = m_sInput;
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}
