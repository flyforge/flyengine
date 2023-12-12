#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Math/Declarations.h>

struct PlasmaEditorAppEvent;
class plPreferences;

struct plSnapProviderEvent
{
  enum class Type
  {
    RotationSnapChanged,
    ScaleSnapChanged,
    TranslationSnapChanged
  };

  Type m_Type;
};

class PLASMA_EDITORFRAMEWORK_DLL plSnapProvider
{
public:
  static void Startup();
  static void Shutdown();

  static plAngle GetRotationSnapValue();
  static float GetScaleSnapValue();
  static float GetTranslationSnapValue();

  static void SetRotationSnapValue(plAngle angle);
  static void SetScaleSnapValue(float fPercentage);
  static void SetTranslationSnapValue(float fUnits);

  /// \brief Rounds each component to the closest translation snapping value
  static void SnapTranslation(plVec3& value);

  /// \brief Inverts the rotation, applies that to the translation, snaps it and then transforms it back into the original space
  static void SnapTranslationInLocalSpace(const plQuat& rotation, plVec3& translation);

  static void SnapRotation(plAngle& rotation);

  static void SnapScale(float& scale);
  static void SnapScale(plVec3& scale);

  static plVec3 GetScaleSnapped(const plVec3& scale);

  static plEvent<const plSnapProviderEvent&> s_Events;

private:
  static void EditorEventHandler(const PlasmaEditorAppEvent& e);
  static void PreferenceChangedEventHandler(plPreferences* pPreferenceBase);

  static plAngle s_RotationSnapValue;
  static float s_fScaleSnapValue;
  static float s_fTranslationSnapValue;
  static plEventSubscriptionID s_UserPreferencesChanged;
};
