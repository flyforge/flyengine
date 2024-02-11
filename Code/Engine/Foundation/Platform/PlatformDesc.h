#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Utilities/EnumerableClass.h>

class PL_FOUNDATION_DLL plPlatformDesc : public plEnumerable<plPlatformDesc>
{
  PL_DECLARE_ENUMERABLE_CLASS(plPlatformDesc);

public:
  plPlatformDesc(const char* szName)
  {
    m_szName = szName;
  }

  const char* GetName() const
  {
    return m_szName;
  }

  static const plPlatformDesc& GetThisPlatformDesc()
  {
    return *s_pThisPlatform;
  }

private:
  static const plPlatformDesc* s_pThisPlatform;

  const char* m_szName;
};