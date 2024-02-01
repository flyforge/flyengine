#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureAtlasDesc.h>

plResult plTextureAtlasCreationDesc::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(3);

  if (m_Layers.GetCount() > 255u)
    return PL_FAILURE;

  const plUInt8 uiNumLayers = static_cast<plUInt8>(m_Layers.GetCount());
  inout_stream << uiNumLayers;

  for (plUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream << m_Layers[l].m_Usage;
    inout_stream << m_Layers[l].m_uiNumChannels;
  }

  inout_stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    inout_stream << item.m_uiUniqueID;
    inout_stream << item.m_uiFlags;

    for (plUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream << item.m_sLayerInput[l];
    }

    inout_stream << item.m_sAlphaInput;
  }

  return PL_SUCCESS;
}

plResult plTextureAtlasCreationDesc::Deserialize(plStreamReader& inout_stream)
{
  const plTypeVersion uiVersion = inout_stream.ReadVersion(3);

  plUInt8 uiNumLayers = 0;
  inout_stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (plUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream >> m_Layers[l].m_Usage;
    inout_stream >> m_Layers[l].m_uiNumChannels;
  }

  plUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    inout_stream >> item.m_uiUniqueID;
    inout_stream >> item.m_uiFlags;

    for (plUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream >> item.m_sLayerInput[l];
    }

    if (uiVersion >= 3)
    {
      inout_stream >> item.m_sAlphaInput;
    }
  }

  return PL_SUCCESS;
}

plResult plTextureAtlasCreationDesc::Save(plStringView sFile) const
{
  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  return Serialize(file);
}

plResult plTextureAtlasCreationDesc::Load(plStringView sFile)
{
  plFileReader file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  return Deserialize(file);
}

void plTextureAtlasRuntimeDesc::Clear()
{
  m_uiNumLayers = 0;
  m_Items.Clear();
}

plResult plTextureAtlasRuntimeDesc::Serialize(plStreamWriter& inout_stream) const
{
  m_Items.Sort();

  inout_stream << m_uiNumLayers;
  inout_stream << m_Items.GetCount();

  for (plUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    inout_stream << m_Items.GetKey(i);
    inout_stream << m_Items.GetValue(i).m_uiFlags;

    for (plUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      inout_stream << r.x;
      inout_stream << r.y;
      inout_stream << r.width;
      inout_stream << r.height;
    }
  }

  return PL_SUCCESS;
}

plResult plTextureAtlasRuntimeDesc::Deserialize(plStreamReader& inout_stream)
{
  Clear();

  inout_stream >> m_uiNumLayers;

  plUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (plUInt32 i = 0; i < uiNumItems; ++i)
  {
    plUInt32 key = 0;
    inout_stream >> key;

    auto& item = m_Items[key];
    inout_stream >> item.m_uiFlags;

    for (plUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      inout_stream >> r.x;
      inout_stream >> r.y;
      inout_stream >> r.width;
      inout_stream >> r.height;
    }
  }

  m_Items.Sort();
  return PL_SUCCESS;
}


