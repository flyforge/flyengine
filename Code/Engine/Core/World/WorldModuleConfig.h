#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class PLASMA_CORE_DLL plWorldModuleConfig
{
public:
  plResult Save();
  void Load();
  void Apply();

  void AddInterfaceImplementation(plStringView sInterfaceName, plStringView sImplementationName);
  void RemoveInterfaceImplementation(plStringView sInterfaceName);

  struct InterfaceImpl
  {
    plString m_sInterfaceName;
    plString m_sImplementationName;

    bool operator<(const InterfaceImpl& rhs) const { return m_sInterfaceName < rhs.m_sInterfaceName; }
  };

  plHybridArray<InterfaceImpl, 8> m_InterfaceImpls;
};
