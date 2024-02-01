#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class plImageDataAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plImageDataAssetProperties, plReflectedClass);

public:
  plString m_sInputFile;

  // TODO: more plImageData options
  // * maximum resolution
  // * 1, 2, 3, 4 channels
  // * compression: lossy (jpg), lossless (png), uncompressed
  // * HDR data ?
  // * combine from multiple images (channel mapping)
};
