#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

namespace plProcGenInternal
{

  plUInt32 GraphSharedData::AddTagSet(const plTagSet& tagSet)
  {
    plUInt32 uiIndex = m_TagSets.IndexOf(tagSet);
    if (uiIndex == plInvalidIndex)
    {
      uiIndex = m_TagSets.GetCount();
      m_TagSets.PushBack(tagSet);
    }
    return uiIndex;
  }

  const plTagSet& GraphSharedData::GetTagSet(plUInt32 uiIndex) const { return m_TagSets[uiIndex]; }

  static plTypeVersion s_GraphSharedDataVersion = 1;

  void GraphSharedData::Save(plStreamWriter& stream) const
  {
    stream.WriteVersion(s_GraphSharedDataVersion);

    {
      const plUInt32 uiCount = m_TagSets.GetCount();
      stream << uiCount;

      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets[i].Save(stream);
      }
    }
  }

  plResult GraphSharedData::Load(plStreamReader& stream)
  {
    auto version = stream.ReadVersion(s_GraphSharedDataVersion);

    {
      plUInt32 uiCount = 0;
      stream >> uiCount;

      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets.ExpandAndGetRef().Load(stream, plTagRegistry::GetGlobalRegistry());
      }
    }

    return PLASMA_SUCCESS;
  }

} // namespace plProcGenInternal
