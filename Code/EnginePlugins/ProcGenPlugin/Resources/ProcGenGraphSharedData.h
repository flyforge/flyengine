#pragma once

#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

namespace plProcGenInternal
{

  class PLASMA_PROCGENPLUGIN_DLL GraphSharedData : public GraphSharedDataBase
  {
  public:
    plUInt32 AddTagSet(const plTagSet& tagSet);

    const plTagSet& GetTagSet(plUInt32 uiIndex) const;

    void Save(plStreamWriter& stream) const;
    plResult Load(plStreamReader& stream);

  private:
    plDynamicArray<plTagSet> m_TagSets;
  };

} // namespace plProcGenInternal
