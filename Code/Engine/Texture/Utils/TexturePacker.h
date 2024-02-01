#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Texture/TextureDLL.h>

class PL_TEXTURE_DLL plTexturePacker
{
public:
  struct Texture
  {
    PL_DECLARE_POD_TYPE();

    plVec2U32 m_Size;
    plVec2U32 m_Position;
    plInt32 m_Priority = 0;
  };

  plTexturePacker();
  ~plTexturePacker();

  void SetTextureSize(plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiReserveTextures = 0);

  void AddTexture(plUInt32 uiWidth, plUInt32 uiHeight);

  const plDynamicArray<Texture>& GetTextures() const { return m_Textures; }

  plResult PackTextures();

private:
  bool CanPlaceAt(plVec2U32 pos, plVec2U32 size);
  bool TryPlaceAt(plVec2U32 pos, plVec2U32 size);
  plUInt32 PosToIndex(plUInt32 x, plUInt32 y) const;
  bool TryPlaceTexture(plUInt32 idx);

  plUInt32 m_uiWidth = 0;
  plUInt32 m_uiHeight = 0;

  plDynamicArray<Texture> m_Textures;
  plDynamicArray<bool> m_Grid;
};
