#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <RmlUiPlugin/Implementation/EventListener.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
#include <RmlUiPlugin/Implementation/FileInterface.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

plResult plRmlUiConfiguration::Save(plStringView sFile) const
{
  PLASMA_LOG_BLOCK("plRmlUiConfiguration::Save()");

  plFileWriter file;
  if (file.Open(sFile).Failed())
    return PLASMA_FAILURE;

  plOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::Compliant);

  writer.BeginObject("Fonts");
  for (auto& font : m_Fonts)
  {
    plOpenDdlUtils::StoreString(writer, font);
  }
  writer.EndObject();

  return PLASMA_SUCCESS;
}

plResult plRmlUiConfiguration::Load(plStringView sFile)
{
  PLASMA_LOG_BLOCK("plRmlUiConfiguration::Load()");

  m_Fonts.Clear();

#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
  if (sFile == s_sConfigFile)
  {
    sFile = plFileSystem::MigrateFileLocation(":project/RmlUiConfig.ddl", s_sConfigFile);
  }
#endif

  plFileReader file;
  if (file.Open(sFile).Failed())
    return PLASMA_FAILURE;

  plOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    plLog::Error("Failed to parse RmlUi config file '{0}'", sFile);
    return PLASMA_FAILURE;
  }

  const plOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const plOpenDdlReaderElement* pChild = pTree->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Fonts"))
    {
      for (const plOpenDdlReaderElement* pFont = pChild->GetFirstChild(); pFont != nullptr; pFont = pFont->GetSibling())
      {
        m_Fonts.PushBack(pFont->GetPrimitivesString()[0]);
      }
    }
  }

  return PLASMA_SUCCESS;
}

bool plRmlUiConfiguration::operator==(const plRmlUiConfiguration& rhs) const
{
  return m_Fonts == rhs.m_Fonts;
}

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_SINGLETON(plRmlUi);

struct plRmlUi::Data
{
  plMutex m_ExtractionMutex;
  plRmlUiInternal::Extractor m_Extractor;

  plRmlUiInternal::FileInterface m_FileInterface;
  plRmlUiInternal::SystemInterface m_SystemInterface;

  plRmlUiInternal::ContextInstancer m_ContextInstancer;
  plRmlUiInternal::EventListenerInstancer m_EventListenerInstancer;

  plDynamicArray<plRmlUiContext*> m_Contexts;

  plRmlUiConfiguration m_Config;
};

plRmlUi::plRmlUi()
  : m_SingletonRegistrar(this)
{
  m_pData = PLASMA_DEFAULT_NEW(Data);

  Rml::SetRenderInterface(&m_pData->m_Extractor);
  Rml::SetFileInterface(&m_pData->m_FileInterface);
  Rml::SetSystemInterface(&m_pData->m_SystemInterface);

  Rml::Initialise();

  Rml::Factory::RegisterContextInstancer(&m_pData->m_ContextInstancer);
  Rml::Factory::RegisterEventListenerInstancer(&m_pData->m_EventListenerInstancer);

  if (m_pData->m_Config.Load().Failed())
  {
    plLog::Warning("No valid RmlUi configuration file available in '{}'.", plRmlUiConfiguration::s_sConfigFile);
    return;
  }

  for (auto& font : m_pData->m_Config.m_Fonts)
  {
    if (Rml::LoadFontFace(font.GetData()) == false)
    {
      plLog::Warning("Failed to load font face '{0}'.", font);
    }
  }
}

plRmlUi::~plRmlUi()
{
  Rml::Shutdown();
}

plRmlUiContext* plRmlUi::CreateContext(const char* szName, const plVec2U32& initialSize)
{
  plRmlUiContext* pContext = static_cast<plRmlUiContext*>(Rml::CreateContext(szName, Rml::Vector2i(initialSize.x, initialSize.y)));

  m_pData->m_Contexts.PushBack(pContext);

  return pContext;
}

void plRmlUi::DeleteContext(plRmlUiContext* pContext)
{
  m_pData->m_Contexts.RemoveAndCopy(pContext);

  Rml::RemoveContext(pContext->GetName());
}

bool plRmlUi::AnyContextWantsInput()
{
  for (auto pContext : m_pData->m_Contexts)
  {
    if (pContext->WantsInput())
      return true;
  }

  return false;
}

void plRmlUi::ExtractContext(plRmlUiContext& context, plMsgExtractRenderData& msg)
{
  if (context.HasDocument() == false)
    return;

  // Unfortunately we need to hold a lock for the whole extraction of a context since RmlUi is not thread safe.
  PLASMA_LOCK(m_pData->m_ExtractionMutex);

  context.ExtractRenderData(m_pData->m_Extractor);

  if (context.m_pRenderData != nullptr)
  {
    msg.AddRenderData(context.m_pRenderData, plDefaultRenderDataCategories::GUI, plRenderData::Caching::Never);
  }
}
