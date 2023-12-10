#pragma once

#include <Foundation/Application/Application.h>

class plStreamWriter;

class plNoiseGen : public plApplication
{
public:
  typedef plApplication SUPER;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(const char* szKey, plInt32 iVal)
      : m_szKey(szKey)
      , m_iEnumValue(iVal)
    {
    }

    const char* m_szKey;
    plInt32 m_iEnumValue = -1;
  };

  plNoiseGen();

public:
  virtual Execution Run() override;
  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

private:
  plString m_sOutputFile;
};