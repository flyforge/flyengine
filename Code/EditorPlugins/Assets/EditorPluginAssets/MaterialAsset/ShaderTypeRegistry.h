#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>

struct plPhantomRttiManagerEvent;

class plShaderTypeRegistry
{
  PLASMA_DECLARE_SINGLETON(plShaderTypeRegistry);

public:
  plShaderTypeRegistry();
  ~plShaderTypeRegistry();

  const plRTTI* GetShaderType(plStringView sShaderPath);
  const plRTTI* GetShaderBaseType() const { return m_pBaseType; }

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ShaderTypeRegistry);

  struct ShaderData
  {
    ShaderData()
      : m_pType(nullptr)
    {
    }

    plString m_sShaderPath;
    plString m_sAbsShaderPath;
    plTimestamp m_fileModifiedTime;
    const plRTTI* m_pType;
  };
  void UpdateShaderType(ShaderData& data);
  void PhantomTypeRegistryEventHandler(const plPhantomRttiManagerEvent& e);

  plMap<plString, ShaderData> m_ShaderTypes;
  const plRTTI* m_pBaseType;
};
