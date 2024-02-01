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

  void GraphSharedData::Save(plStreamWriter& inout_stream) const
  {
    inout_stream.WriteVersion(s_GraphSharedDataVersion);

    {
      const plUInt32 uiCount = m_TagSets.GetCount();
      inout_stream << uiCount;

      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets[i].Save(inout_stream);
      }
    }
  }

  plResult GraphSharedData::Load(plStreamReader& inout_stream)
  {
    auto version = inout_stream.ReadVersion(s_GraphSharedDataVersion);
    PL_IGNORE_UNUSED(version);

    {
      plUInt32 uiCount = 0;
      inout_stream >> uiCount;

      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets.ExpandAndGetRef().Load(inout_stream, plTagRegistry::GetGlobalRegistry());
      }
    }

    return PL_SUCCESS;
  }

} // namespace plProcGenInternal
