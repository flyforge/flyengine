#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief What a translated string is used for.
enum class plTranslationUsage
{
  Default,
  Tooltip,
  HelpURL,

  ENUM_COUNT
};

/// \brief Base class to translate one string into another
class PL_FOUNDATION_DLL plTranslator
{
public:
  plTranslator();
  virtual ~plTranslator();

  /// \brief The given string (with the given hash) shall be translated
  virtual plStringView Translate(plStringView sString, plUInt64 uiStringHash, plTranslationUsage usage) = 0;

  /// \brief Called to reset internal state
  virtual void Reset();

  /// \brief May reload the known translations
  virtual void Reload();

  /// \brief Will call Reload() on all currently active translators
  static void ReloadAllTranslators();

  static void HighlightUntranslated(bool bHighlight);

  static bool GetHighlightUntranslated() { return s_bHighlightUntranslated; }

private:
  static bool s_bHighlightUntranslated;
  static plHybridArray<plTranslator*, 4> s_AllTranslators;
};

/// \brief Just returns the same string that is passed into it. Can be used to display the actually untranslated strings
class PL_FOUNDATION_DLL plTranslatorPassThrough : public plTranslator
{
public:
  virtual plStringView Translate(plStringView sString, plUInt64 uiStringHash, plTranslationUsage usage) override { return sString; }
};

/// \brief Can store translated strings and all translation requests will come from that storage. Returns nullptr if the requested string is
/// not known
class PL_FOUNDATION_DLL plTranslatorStorage : public plTranslator
{
public:
  /// \brief Stores szString as the translation for the string with the given hash
  virtual void StoreTranslation(plStringView sString, plUInt64 uiStringHash, plTranslationUsage usage);

  /// \brief Returns the translated string for uiStringHash, or nullptr, if not available
  virtual plStringView Translate(plStringView sString, plUInt64 uiStringHash, plTranslationUsage usage) override;

  /// \brief Clears all stored translation strings
  virtual void Reset() override;

  /// \brief Simply executes Reset() on this translator
  virtual void Reload() override;

protected:
  plMap<plUInt64, plString> m_Translations[(int)plTranslationUsage::ENUM_COUNT];
};

/// \brief Outputs a 'Missing Translation' warning the first time a string translation is requested.
/// Otherwise always returns nullptr, allowing the next translator to take over.
class PL_FOUNDATION_DLL plTranslatorLogMissing : public plTranslatorStorage
{
public:
  /// Can be used from external code to (temporarily) deactivate error logging (a bit hacky)
  static bool s_bActive;

  virtual plStringView Translate(plStringView sString, plUInt64 uiStringHash, plTranslationUsage usage) override;
};

/// \brief Loads translations from files. Each translator can have different search paths, but the files to be loaded are the same for all of them.
class PL_FOUNDATION_DLL plTranslatorFromFiles : public plTranslatorStorage
{
public:
  /// \brief Loads all files recursively from the specified folder as translation files.
  ///
  /// The given path must be absolute or resolvable to an absolute path.
  /// On failure, the function does nothing.
  /// This function depends on plFileSystemIterator to be available.
  void AddTranslationFilesFromFolder(const char* szFolder);

  virtual plStringView Translate(plStringView sString, plUInt64 uiStringHash, plTranslationUsage usage) override;

  virtual void Reload() override;

private:
  void LoadTranslationFile(const char* szFullPath);

  plHybridArray<plString, 4> m_Folders;
};

/// \brief Returns the same string that is passed into it, but strips off class names and separates the text at CamelCase boundaries.
class PL_FOUNDATION_DLL plTranslatorMakeMoreReadable : public plTranslatorStorage
{
public:
  virtual plStringView Translate(plStringView sString, plUInt64 uiStringHash, plTranslationUsage usage) override;
};

/// \brief Handles looking up translations for strings.
///
/// Multiple translators can be registered to get translations from different sources.
class PL_FOUNDATION_DLL plTranslationLookup
{
public:
  /// \brief Translators will be queried in the reverse order that they were added.
  static void AddTranslator(plUniquePtr<plTranslator> pTranslator);

  /// \brief Prefer to use the plTranslate macro instead of calling this function directly. Will query all translators for a translation,
  /// until one is found.
  static plStringView Translate(plStringView sString, plUInt64 uiStringHash, plTranslationUsage usage);

  /// \brief Deletes all translators.
  static void Clear();

private:
  static plHybridArray<plUniquePtr<plTranslator>, 16> s_Translators;
};

/// \brief Use this macro to query a translation for a string from the plTranslationLookup system
#define plTranslate(string) plTranslationLookup::Translate(string, plHashingUtils::StringHash(string), plTranslationUsage::Default)

/// \brief Use this macro to query a translation for a tooltip string from the plTranslationLookup system
#define plTranslateTooltip(string) plTranslationLookup::Translate(string, plHashingUtils::StringHash(string), plTranslationUsage::Tooltip)

/// \brief Use this macro to query a translation for a help URL from the plTranslationLookup system
#define plTranslateHelpURL(string) plTranslationLookup::Translate(string, plHashingUtils::StringHash(string), plTranslationUsage::HelpURL)
