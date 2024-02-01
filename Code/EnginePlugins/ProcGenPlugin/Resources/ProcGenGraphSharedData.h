#pragma once

#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

namespace plProcGenInternal
{

  class PL_PROCGENPLUGIN_DLL GraphSharedData : public GraphSharedDataBase
  {
  public:
    plUInt32 AddTagSet(const plTagSet& tagSet);

    const plTagSet& GetTagSet(plUInt32 uiIndex) const;

    void Save(plStreamWriter& inout_stream) const;
    plResult Load(plStreamReader& inout_stream);

  private:
    plDynamicArray<plTagSet> m_TagSets;
  };

} // namespace plProcGenInternal
