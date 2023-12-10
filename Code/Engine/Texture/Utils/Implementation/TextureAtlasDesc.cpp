#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureAtlasDesc.h>

plResult plTextureAtlasCreationDesc::Serialize(plStreamWriter& stream) const
{
  stream.WriteVersion(3);

  if (m_Layers.GetCount() > 255u)
    return PLASMA_FAILURE;

  const plUInt8 uiNumLayers = static_cast<plUInt8>(m_Layers.GetCount());
  stream << uiNumLayers;

  for (plUInt32 l = 0; l < uiNumLayers; ++l)
  {
    stream << m_Layers[l].m_Usage;
    stream << m_Layers[l].m_uiNumChannels;
  }

  stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    stream << item.m_uiUniqueID;
    stream << item.m_uiFlags;

    for (plUInt32 l = 0; l < uiNumLayers; ++l)
    {
      stream << item.m_sLayerInput[l];
    }

    stream << item.m_sAlphaInput;
  }

  return PLASMA_SUCCESS;
}

plResult plTextureAtlasCreationDesc::Deserialize(plStreamReader& stream)
{
  const plTypeVersion uiVersion = stream.ReadVersion(3);

  plUInt8 uiNumLayers = 0;
  stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (plUInt32 l = 0; l < uiNumLayers; ++l)
  {
    stream >> m_Layers[l].m_Usage;
    stream >> m_Layers[l].m_uiNumChannels;
  }

  plUInt32 uiNumItems = 0;
  stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    stream >> item.m_uiUniqueID;
    stream >> item.m_uiFlags;

    for (plUInt32 l = 0; l < uiNumLayers; ++l)
    {
      stream >> item.m_sLayerInput[l];
    }

    if (uiVersion >= 3)
    {
      stream >> item.m_sAlphaInput;
    }
  }

  return PLASMA_SUCCESS;
}

plResult plTextureAtlasCreationDesc::Save(plStringView sFile) const
{
  plFileWriter file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(sFile));

  return Serialize(file);
}

plResult plTextureAtlasCreationDesc::Load(plStringView sFile)
{
  plFileReader file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(sFile));

  return Deserialize(file);
}

void plTextureAtlasRuntimeDesc::Clear()
{
  m_uiNumLayers = 0;
  m_Items.Clear();
}

plResult plTextureAtlasRuntimeDesc::Serialize(plStreamWriter& stream) const
{
  m_Items.Sort();

  stream << m_uiNumLayers;
  stream << m_Items.GetCount();

  for (plUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    stream << m_Items.GetKey(i);
    stream << m_Items.GetValue(i).m_uiFlags;

    for (plUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      stream << r.x;
      stream << r.y;
      stream << r.width;
      stream << r.height;
    }
  }

  return PLASMA_SUCCESS;
}

plResult plTextureAtlasRuntimeDesc::Deserialize(plStreamReader& stream)
{
  Clear();

  stream >> m_uiNumLayers;

  plUInt32 uiNumItems = 0;
  stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (plUInt32 i = 0; i < uiNumItems; ++i)
  {
    plUInt32 key = 0;
    stream >> key;

    auto& item = m_Items[key];
    stream >> item.m_uiFlags;

    for (plUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      stream >> r.x;
      stream >> r.y;
      stream >> r.width;
      stream >> r.height;
    }
  }

  m_Items.Sort();
  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TextureAtlasDesc);
