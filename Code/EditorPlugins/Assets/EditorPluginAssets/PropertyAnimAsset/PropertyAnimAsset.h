#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <Foundation/Communication/Event.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

struct plGameObjectContextEvent;
class plPropertyAnimObjectAccessor;
class plPropertyAnimAssetDocument;
struct plCommandHistoryEvent;

class plPropertyAnimationTrack : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPropertyAnimationTrack, plReflectedClass);

public:
  plString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  plString m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  plString m_sPropertyPath;
  plEnum<plPropertyAnimTarget> m_Target;

  plSingleCurveData m_FloatCurve;
  plColorGradientAssetData m_ColorGradient;
};

class plPropertyAnimationTrackGroup : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPropertyAnimationTrackGroup, plReflectedClass);

public:
  plPropertyAnimationTrackGroup() = default;
  plPropertyAnimationTrackGroup(const plPropertyAnimationTrackGroup&) = delete;
  plPropertyAnimationTrackGroup& operator=(const plPropertyAnimationTrackGroup& rhs) = delete;
  ~plPropertyAnimationTrackGroup();

  plUInt32 m_uiFramesPerSecond = 60;
  plUInt64 m_uiCurveDuration = 480;
  plEnum<plPropertyAnimMode> m_Mode;
  plDynamicArray<plPropertyAnimationTrack*> m_Tracks;
  plEventTrackData m_EventTrack;
};

struct plPropertyAnimAssetDocumentEvent
{
  enum class Type
  {
    AnimationLengthChanged,
    ScrubberPositionChanged,
    PlaybackChanged,
  };

  const plPropertyAnimAssetDocument* m_pDocument;
  Type m_Type;
};

class plPropertyAnimAssetDocument : public plSimpleAssetDocument<plPropertyAnimationTrackGroup, plGameObjectContextDocument>
{
  typedef plSimpleAssetDocument<plPropertyAnimationTrackGroup, plGameObjectContextDocument> BaseClass;
  PLASMA_ADD_DYNAMIC_REFLECTION(plPropertyAnimAssetDocument, BaseClass);

public:
  plPropertyAnimAssetDocument(const char* szDocumentPath);
  ~plPropertyAnimAssetDocument();

  void SetAnimationDurationTicks(plUInt64 uiNumTicks);
  plUInt64 GetAnimationDurationTicks() const;
  plTime GetAnimationDurationTime() const;
  void AdjustDuration();

  bool SetScrubberPosition(plUInt64 uiTick);
  plUInt64 GetScrubberPosition() const { return m_uiScrubberTickPos; }

  plEvent<const plPropertyAnimAssetDocumentEvent&> m_PropertyAnimEvents;

  void SetPlayAnimation(bool play);
  bool GetPlayAnimation() const { return m_bPlayAnimation; }
  void SetRepeatAnimation(bool repeat);
  bool GetRepeatAnimation() const { return m_bRepeatAnimation; }
  void ExecuteAnimationPlaybackStep();

  const plPropertyAnimationTrack* GetTrack(const plUuid& trackGuid) const;
  plPropertyAnimationTrack* GetTrack(const plUuid& trackGuid);

  plStatus CanAnimate(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target) const;

  plUuid FindTrack(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target) const;
  plUuid CreateTrack(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target);

  plUuid FindCurveCp(const plUuid& trackGuid, plInt64 tickX);
  plUuid InsertCurveCpAt(const plUuid& trackGuid, plInt64 tickX, double newPosY);

  plUuid FindGradientColorCp(const plUuid& trackGuid, plInt64 tickX);
  plUuid InsertGradientColorCpAt(const plUuid& trackGuid, plInt64 tickX, const plColorGammaUB& color);

  plUuid FindGradientAlphaCp(const plUuid& trackGuid, plInt64 tickX);
  plUuid InsertGradientAlphaCpAt(const plUuid& trackGuid, plInt64 tickX, plUInt8 alpha);

  plUuid FindGradientIntensityCp(const plUuid& trackGuid, plInt64 tickX);
  plUuid InsertGradientIntensityCpAt(const plUuid& trackGuid, plInt64 tickX, float intensity);

  plUuid InsertEventTrackCpAt(plInt64 tickX, const char* szValue);

  virtual plManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return plManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

private:
  void GameObjectContextEventHandler(const plGameObjectContextEvent& e);
  void TreeStructureEventHandler(const plDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const plDocumentObjectPropertyEvent& e);

  struct PropertyValue
  {
    plVariant m_InitialValue;
    plHybridArray<plUuid, 3> m_Tracks;
  };
  struct PropertyKeyHash
  {
    PLASMA_ALWAYS_INLINE static plUInt32 Hash(const plPropertyReference& key)
    {
      return plHashingUtils::xxHash32(&key.m_Object, sizeof(plUuid)) + plHashingUtils::xxHash32(&key.m_pProperty, sizeof(const plAbstractProperty*)) +
             (plUInt32)key.m_Index.ComputeHash();
    }

    PLASMA_ALWAYS_INLINE static bool Equal(const plPropertyReference& a, const plPropertyReference& b)
    {
      return a.m_Object == b.m_Object && a.m_pProperty == b.m_pProperty && a.m_Index == b.m_Index;
    }
  };

  void RebuildMapping();
  void RemoveTrack(const plUuid& track);
  void AddTrack(const plUuid& track);
  void FindTrackKeys(
    const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, plHybridArray<plPropertyReference, 1>& keys) const;
  void GenerateTrackInfo(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plStringBuilder& sObjectSearchSequence,
    plStringBuilder& sComponentType, plStringBuilder& sPropertyPath) const;
  void ApplyAnimation();
  void ApplyAnimation(const plPropertyReference& key, const PropertyValue& value);

  plHashTable<plPropertyReference, PropertyValue, PropertyKeyHash> m_PropertyTable;
  plHashTable<plUuid, plHybridArray<plPropertyReference, 1>> m_TrackTable;

  bool m_bPlayAnimation = false;
  bool m_bRepeatAnimation = false;
  plTime m_LastFrameTime;
  plUInt64 m_uiScrubberTickPos = 0;
};
