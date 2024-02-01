#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLUTAssetProperties, 1, plRTTIDefaultAllocator<plLUTAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Input", GetInputFile, SetInputFile)->AddAttributes(new plFileBrowserAttribute("Select CUBE file", "*.cube")),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
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
