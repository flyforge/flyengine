#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <Texture/TexConv/TexConvEnums.h>

struct PLASMA_TEXTURE_DLL plTextureAtlasCreationDesc
{
  struct Layer
  {
    plEnum<plTexConvUsage> m_Usage;
    plUInt8 m_uiNumChannels = 4;
  };

  struct Item
  {
    plUInt32 m_uiUniqueID;
    plUInt32 m_uiFlags;
    plString m_sAlphaInput;
    plString m_sLayerInput[4];
  };

  plHybridArray<Layer, 4> m_Layers;
  plDynamicArray<Item> m_Items;

  plResult Serialize(plStreamWriter& stream) const;
  plResult Deserialize(plStreamReader& stream);

  plResult Save(plStringView sFile) const;
  plResult Load(plStringView sFile);
};

struct PLASMA_TEXTURE_DLL plTextureAtlasRuntimeDesc
{
  struct Item
  {
    plUInt32 m_uiFlags;
    plRectU32 m_LayerRects[4];
  };

  plUInt32 m_uiNumLayers = 0;
  plArrayMap<plUInt32, Item> m_Items;

  void Clear();

  plResult Serialize(plStreamWriter& stream) const;
  plResult Deserialize(plStreamReader& stream);
};
