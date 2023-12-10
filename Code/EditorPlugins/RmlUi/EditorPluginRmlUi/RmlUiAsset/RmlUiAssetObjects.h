#pragma once

#include <RmlUiPlugin/Resources/RmlUiResource.h>

class plRmlUiAssetProperties : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRmlUiAssetProperties, plReflectedClass);

public:
  plRmlUiAssetProperties();
  ~plRmlUiAssetProperties();

  plString m_sRmlFile;
  plEnum<plRmlUiScaleMode> m_ScaleMode;
  plVec2U32 m_ReferenceResolution;
};
