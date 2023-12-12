#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <ProcGenPlugin/Declarations.h>

namespace plProcGenInternal
{
  static Pattern::Point s_BayerPoints[64];

  static plHashTable<plUInt64, Pattern, plHashHelper<plUInt64>, plStaticAllocatorWrapper> s_Patterns;

  bool FillPatterns()
  {
    // generate Bayer pattern
    const plUInt32 M = 3;
    const plUInt32 n = 1 << M;

    for (plUInt32 y = 0; y < n; ++y)
    {
      for (plUInt32 x = 0; x < n; ++x)
      {
        plUInt32 v = 0, mask = M - 1, xc = x ^ y, yc = y;
        for (plUInt32 bit = 0; bit < 2 * M; --mask)
        {
          v |= ((yc >> mask) & 1) << bit++;
          v |= ((xc >> mask) & 1) << bit++;
        }

        auto& point = s_BayerPoints[y * n + x];
        point.m_Coordinates.Set(x + 0.5f, y + 0.5f);
        point.m_fThreshold = (v + 1.0f) / (n * n);
      }
    }

    Pattern bayerPattern;
    bayerPattern.m_Points = plMakeArrayPtr(s_BayerPoints);
    bayerPattern.m_fSize = (float)n;

    s_Patterns.Insert(plTempHashedString("Bayer").GetHash(), bayerPattern);

    return true;
  }

  static bool s_bFillPatternsDummy = FillPatterns();

  Pattern* GetPattern(plTempHashedString sName) { return s_Patterns.GetValue(sName.GetHash()); }
} // namespace plProcGenInternal
