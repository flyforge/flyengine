#pragma once

#include <Foundation/Application/Application.h>

class plStreamWriter;

class plAlphaComp : public plApplication
{
public:
  typedef plApplication SUPER;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(const char* key, plInt32 val)
      : m_szKey(key)
      , m_iEnumValue(val)
    {
    }

    const char* m_szKey;
    plInt32 m_iEnumValue = -1;
  };

  plAlphaComp();

public:
  virtual Execution Run() override;
  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

private:
};