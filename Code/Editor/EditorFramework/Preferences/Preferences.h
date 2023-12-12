#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class plDocument;

/// \brief Base class for all preferences.
///
/// Derive from this to implement a custom class containing preferences.
/// All properties in such a class are exposed in the preferences UI and are automatically stored and restored.
///
/// Pass the 'Domain' and 'Visibility' to the constructor to configure whether the preference class
/// is per application, per project or per document, and whether the data is shared among all users
/// or custom for every user.
class PLASMA_EDITORFRAMEWORK_DLL plPreferences : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPreferences, plReflectedClass);

public:
  enum class Domain
  {
    Application,
    Project,
    Document
  };

  /// \brief Static function to query a preferences object of the given type.
  /// If the instance does not exist yet, it is created and the data is restored from file.
  template <typename TYPE>
  static TYPE* QueryPreferences(const plDocument* pDocument = nullptr)
  {
    PLASMA_CHECK_AT_COMPILETIME_MSG((std::is_base_of<plPreferences, TYPE>::value == true), "All preferences objects must be derived from plPreferences");
    return static_cast<TYPE*>(QueryPreferences(plGetStaticRTTI<TYPE>(), pDocument));
  }

  /// \brief Static function to query a preferences object of the given type.
  /// If the instance does not exist yet, it is created and the data is restored from file.
  static plPreferences* QueryPreferences(const plRTTI* pRtti, const plDocument* pDocument = nullptr);

  /// \brief Saves all preferences that are tied to the given document
  static void SaveDocumentPreferences(const plDocument* pDocument);

  /// \brief Removes all preferences for the given document. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearDocumentPreferences(const plDocument* pDocument);

  /// \brief Saves all project specific preferences.
  static void SaveProjectPreferences();

  /// \brief Removes all project specific preferences. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearProjectPreferences();

  /// \brief Saves all application specific preferences.
  static void SaveApplicationPreferences();

  /// \brief Removes all application specific preferences. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearApplicationPreferences();

  //// \brief Fills the list with all currently known preferences
  static void GatherAllPreferences(plHybridArray<plPreferences*, 16>& out_AllPreferences);

  /// \brief Whether the preferences are app, project or document specific
  Domain GetDomain() const { return m_Domain; }

  /// \brief Within the same domain and visibility the name must be unique, but across those it can be reused.
  plString GetName() const;

  /// \brief If these preferences are per document, the pointer is valid, otherwise nullptr.
  const plDocument* GetDocumentAssociation() const { return m_pDocument; }

  /// A simple event that can be fired when any preference property changes. No specific change details are given.
  plEvent<plPreferences*> m_ChangedEvent;

  /// Call this to broadcast that this preference object was modified.
  void TriggerPreferencesChangedEvent() { m_ChangedEvent.Broadcast(this); }

protected:
  plPreferences(Domain domain, const char* szUniqueName);

  plString GetFilePath() const;

private:
  static void SavePreferences(const plDocument* pDocument, Domain domain);
  static void ClearPreferences(const plDocument* pDocument, Domain domain);

  void Load();
  void Save() const;



private:
  Domain m_Domain;
  plString m_sUniqueName;
  const plDocument* m_pDocument;

  static plMap<const plDocument*, plMap<const plRTTI*, plPreferences*>> s_Preferences;
};
