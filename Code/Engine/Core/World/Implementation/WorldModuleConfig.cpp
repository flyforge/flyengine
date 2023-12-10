#include <Core/CorePCH.h>

#include <Core/World/WorldModule.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

plResult plWorldModuleConfig::Save()
{
  m_InterfaceImpls.Sort();

  plStringBuilder sPath;
  sPath = ":project/WorldModules.ddl";

  plFileWriter file;
  if (file.Open(sPath).Failed())
    return PLASMA_FAILURE;

  plOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::Compliant);

  for (auto& interfaceImpl : m_InterfaceImpls)
  {
    writer.BeginObject("InterfaceImpl");

    plOpenDdlUtils::StoreString(writer, interfaceImpl.m_sInterfaceName, "Interface");
    plOpenDdlUtils::StoreString(writer, interfaceImpl.m_sImplementationName, "Implementation");

    writer.EndObject();
  }

  return PLASMA_SUCCESS;
}

void plWorldModuleConfig::Load()
{
  const char* szPath = ":project/WorldModules.ddl";

  PLASMA_LOG_BLOCK("plWorldModuleConfig::Load()", szPath);

  m_InterfaceImpls.Clear();

  plFileReader file;
  if (file.Open(szPath).Failed())
  {
    plLog::Dev("World module config file is not available: '{0}'", szPath);
    return;
  }
  else
  {
    plLog::Success("World module config file is available: '{0}'", szPath);
  }

  plOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    plLog::Error("Failed to parse world module config file '{0}'", szPath);
    return;
  }

  const plOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const plOpenDdlReaderElement* pInterfaceImpl = pTree->GetFirstChild(); pInterfaceImpl != nullptr;
       pInterfaceImpl = pInterfaceImpl->GetSibling())
  {
    if (!pInterfaceImpl->IsCustomType("InterfaceImpl"))
      continue;

    const plOpenDdlReaderElement* pInterface = pInterfaceImpl->FindChildOfType(plOpenDdlPrimitiveType::String, "Interface");
    const plOpenDdlReaderElement* pImplementation = pInterfaceImpl->FindChildOfType(plOpenDdlPrimitiveType::String, "Implementation");

    // this prevents duplicates
    AddInterfaceImplementation(pInterface->GetPrimitivesString()[0], pImplementation->GetPrimitivesString()[0]);
  }
}

void plWorldModuleConfig::Apply()
{
  PLASMA_LOG_BLOCK("plWorldModuleConfig::Apply");

  for (const auto& interfaceImpl : m_InterfaceImpls)
  {
    plWorldModuleFactory::GetInstance()->RegisterInterfaceImplementation(interfaceImpl.m_sInterfaceName, interfaceImpl.m_sImplementationName);
  }
}

void plWorldModuleConfig::AddInterfaceImplementation(plStringView sInterfaceName, plStringView sImplementationName)
{
  for (auto& interfaceImpl : m_InterfaceImpls)
  {
    if (interfaceImpl.m_sInterfaceName == sInterfaceName)
    {
      interfaceImpl.m_sImplementationName = sImplementationName;
      return;
    }
  }

  m_InterfaceImpls.PushBack({sInterfaceName, sImplementationName});
}

void plWorldModuleConfig::RemoveInterfaceImplementation(plStringView sInterfaceName)
{
  for (plUInt32 i = 0; i < m_InterfaceImpls.GetCount(); ++i)
  {
    if (m_InterfaceImpls[i].m_sInterfaceName == sInterfaceName)
    {
      m_InterfaceImpls.RemoveAtAndCopy(i);
      return;
    }
  }
}


PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_WorldModuleConfig);
