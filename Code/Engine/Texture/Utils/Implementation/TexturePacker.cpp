#include <Texture/TexturePCH.h>

#include <Texture/Utils/TexturePacker.h>

plTexturePacker::plTexturePacker() {}

plTexturePacker::~plTexturePacker() {}

void plTexturePacker::SetTextureSize(plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiReserveTextures /*= 0*/)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  m_Textures.Clear();
  m_Textures.Reserve(uiReserveTextures);

  // initializes all values to false
  m_Grid.Clear();
  m_Grid.SetCount(m_uiWidth * m_uiHeight);
}

void plTexturePacker::AddTexture(plUInt32 uiWidth, plUInt32 uiHeight)
{
  Texture& tex = m_Textures.ExpandAndGetRef();
  tex.m_Size.x = uiWidth;
  tex.m_Size.y = uiHeight;
  tex.m_Priority = 2 * uiWidth + 2 * uiHeight;
}

struct sortdata
{
  PLASMA_DECLARE_POD_TYPE();

  plInt32 m_Priority;
  plInt32 m_Index;
};

plResult plTexturePacker::PackTextures()
{
  plDynamicArray<sortdata> sorted;
  sorted.SetCountUninitialized(m_Textures.GetCount());

  for (plUInt32 i = 0; i < m_Textures.GetCount(); ++i)
  {
    sorted[i].m_Index = i;
    sorted[i].m_Priority = m_Textures[i].m_Priority;
  }

  sorted.Sort([](const sortdata& lhs, const sortdata& rhs) -> bool { return lhs.m_Priority > rhs.m_Priority; });

  for (plUInt32 idx = 0; idx < sorted.GetCount(); ++idx)
  {
    if (!TryPlaceTexture(sorted[idx].m_Index))
      return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plUInt32 plTexturePacker::PosToIndex(plUInt32 x, plUInt32 y) const
{
  return (y * m_uiWidth + x);
}

bool plTexturePacker::TryPlaceTexture(plUInt32 idx)
{
  Texture& tex = m_Textures[idx];

  for (plUInt32 y = 0; y < m_uiHeight; ++y)
  {
    for (plUInt32 x = 0; x < m_uiWidth; ++x)
    {
      if (!TryPlaceAt(plVec2U32(x, y), tex.m_Size))
        continue;

      tex.m_Position.Set(x, y);
      return true;
    }
  }

  return false;
}

bool plTexturePacker::CanPlaceAt(plVec2U32 pos, plVec2U32 size)
{
  if (pos.x + size.x > m_uiWidth)
    return false;
  if (pos.y + size.y > m_uiHeight)
    return false;

  for (plUInt32 y = 0; y < size.y; ++y)
  {
    for (plUInt32 x = 0; x < size.x; ++x)
    {
      if (m_Grid[PosToIndex(pos.x + x, pos.y + y)])
        return false;
    }
  }

  return true;
}

bool plTexturePacker::TryPlaceAt(plVec2U32 pos, plVec2U32 size)
{
  if (!CanPlaceAt(pos, size))
    return false;

  for (plUInt32 y = 0; y < size.y; ++y)
  {
    for (plUInt32 x = 0; x < size.x; ++x)
    {
      m_Grid[PosToIndex(pos.x + x, pos.y + y)] = true;
    }
  }

  return true;
}



PLASMA_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TexturePacker);
