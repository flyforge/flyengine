#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPreferences, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("Name", GetName)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMap<const plDocument*, plMap<const plRTTI*, plPreferences*>> plPreferences::s_Preferences;

plPreferences::plPreferences(Domain domain, const char* szUniqueName)
{
  m_Domain = domain;
  m_sUniqueName = szUniqueName;
  m_pDocument = nullptr;
}

plPreferences* plPreferences::QueryPreferences(const plRTTI* pRtti, const plDocument* pDocument)
{
  PLASMA_ASSERT_DEV(plQtEditorApp::GetSingleton() != nullptr, "Editor app is not available in this process");

  auto it = s_Preferences[pDocument].Find(pRtti);

  if (it.IsValid())
    return it.Value();

  auto pAlloc = pRtti->GetAllocator();
  PLASMA_ASSERT_DEV(pAlloc != nullptr, "Invalid allocator for preferences type");

  if (!pAlloc->CanAllocate())
  {
    PLASMA_ASSERT_DEV(pAlloc->CanAllocate(), "Cannot create a preferences object that does not have a proper allocator");
    return nullptr;
  }

  plPreferences* pPref = pAlloc->Allocate<plPreferences>();
  pPref->m_pDocument = pDocument;
  s_Preferences[pDocument][pRtti] = pPref;

  if (pPref->m_Domain == Domain::Document)
  {
    PLASMA_ASSERT_DEV(pDocument != nullptr, "Preferences of this type can only be used per document");
  }
  else
  {
    PLASMA_ASSERT_DEV(pDocument == nullptr, "Preferences of this type cannot be used with a document");
  }

  pPref->Load();
  return pPref;
}

plString plPreferences::GetFilePath() const
{
  plStringBuilder path;

  if (m_Domain == Domain::Application)
  {
    path = plApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  if (m_Domain == Domain::Project)
  {
    path = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  if (m_Domain == Domain::Document)
  {
    path = plApplicationServices::GetSingleton()->GetDocumentPreferencesFolder(m_pDocument);
    path.AppendPath(m_sUniqueName);
    path.ChangeFileExtension("pref");
  }

  return path;
}

void plPreferences::Load()
{
  plFileReader file;
  if (file.Open(GetFilePath()).Failed())
    return;

  plReflectionSerializer::ReadObjectPropertiesFromDDL(file, *GetDynamicRTTI(), this);
}

void plPreferences::Save() const
{
  bool bNothingToSerialize = true;

  plHybridArray<const plAbstractProperty*, 32> allProperties;
  GetDynamicRTTI()->GetAllProperties(allProperties);

  for (const plAbstractProperty* pProp : allProperties)
  {
    if (pProp->GetCategory() == plPropertyCategory::Constant || pProp->GetFlags().IsAnySet(plPropertyFlags::ReadOnly))
      continue;

    bNothingToSerialize = false;
    break;
  }

  if (bNothingToSerialize)
    return;

  plDeferredFileWriter file;
  file.SetOutput(GetFilePath());

  plReflectionSerializer::WriteObjectToDDL(file, GetDynamicRTTI(), this, false, plOpenDdlWriter::TypeStringMode::Compliant);

  if (file.Close().Failed())
    plLog::Error("Failed to open file for writing '{0}'.", GetFilePath());
}


void plPreferences::SavePreferences(const plDocument* pDocument, Domain domain)
{
  auto& docPrefs = s_Preferences[pDocument];

  // save all preferences for the given document
  for (auto it = docPrefs.GetIterator(); it.IsValid(); ++it)
  {
    auto pPref = it.Value();

    if (pPref->m_Domain == domain)
      pPref->Save();
  }
}

void plPreferences::ClearPreferences(const plDocument* pDocument, Domain domain)
{
  auto& docPrefs = s_Preferences[pDocument];

  // save all preferences for the given document
  for (auto it = docPrefs.GetIterator(); it.IsValid();)
  {
    plPreferences* pPref = it.Value();

    if (pPref->m_Domain == domain)
    {
      pPref->GetDynamicRTTI()->GetAllocator()->Deallocate(pPref);
      it = docPrefs.Remove(it);
    }
    else
      ++it;
  }
}

void plPreferences::SaveDocumentPreferences(const plDocument* pDocument)
{
  SavePreferences(pDocument, Domain::Document);
}

void plPreferences::ClearDocumentPreferences(const plDocument* pDocument)
{
  ClearPreferences(pDocument, Domain::Document);
}

void plPreferences::SaveProjectPreferences()
{
  SavePreferences(nullptr, Domain::Project);
}

void plPreferences::ClearProjectPreferences()
{
  ClearPreferences(nullptr, Domain::Project);
}

void plPreferences::SaveApplicationPreferences()
{
  SavePreferences(nullptr, Domain::Application);
}

void plPreferences::ClearApplicationPreferences()
{
  ClearPreferences(nullptr, Domain::Application);
}

void plPreferences::GatherAllPreferences(plHybridArray<plPreferences*, 16>& out_AllPreferences)
{
  out_AllPreferences.Clear();
  out_AllPreferences.Reserve(s_Preferences.GetCount() * 2);

  for (auto itDoc = s_Preferences.GetIterator(); itDoc.IsValid(); ++itDoc)
  {
    for (auto itType = itDoc.Value().GetIterator(); itType.IsValid(); ++itType)
    {
      out_AllPreferences.PushBack(itType.Value());
    }
  }
}

plString plPreferences::GetName() const
{
  plStringBuilder s;

  if (m_Domain == Domain::Document)
  {
    s.Set(m_sUniqueName, ": ");
    s.Append(plPathUtils::GetFileName(m_pDocument->GetDocumentPath()));
  }
  else
  {
    if (m_Domain == Domain::Application)
      s.Append("Application");
    else if (m_Domain == Domain::Project)
      s.Append("Project");

    s.Append(": ", m_sUniqueName);
  }

  return s;
}
