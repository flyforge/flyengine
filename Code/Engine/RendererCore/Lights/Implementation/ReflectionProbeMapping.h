#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Event generated on mapping changes.
/// \sa plReflectionProbeMapping::m_Events
struct plReflectionProbeMappingEvent
{
  enum class Type
  {
    ProbeMapped,          ///< The given probe was mapped to the atlas.
    ProbeUnmapped,        ///<  The given probe was unmapped from the atlas.
    ProbeUpdateRequested, ///< The given probe needs to be updated after which plReflectionProbeMapping::ProbeUpdateFinished must be called.
  };

  plReflectionProbeId m_Id;
  Type m_Type;
};

/// \brief This class creates a reflection probe atlas and controls the mapping of added probes to the available atlas indices.
class plReflectionProbeMapping
{
public:
  /// \brief Creates a reflection probe atlas and mapping of the given size.
  /// \param uiAtlasSize How many probes the atlas can contain.
  plReflectionProbeMapping(plUInt32 uiAtlasSize);
  ~plReflectionProbeMapping();

  /// \name Probe management
  ///@{

  /// \brief Adds a probe that will be considered for mapping into the atlas.
  void AddProbe(plReflectionProbeId probe, plBitflags<plProbeFlags> flags);

  /// \brief Marks previously added probe as dirty and potentially changes its flags.
  void UpdateProbe(plReflectionProbeId probe, plBitflags<plProbeFlags> flags);

  /// \brief Should be called once a requested plReflectionProbeMappingEvent::Type::ProbeUpdateRequested event has been completed.
  /// \param probe The probe that has finished its update.
  void ProbeUpdateFinished(plReflectionProbeId probe);

  /// \brief Removes a probe. If the probe was mapped, plReflectionProbeMappingEvent::Type::ProbeUnmapped will be fired when calling this function.
  void RemoveProbe(plReflectionProbeId probe);

  ///@}
  /// \name Render helpers
  ///@{

  /// \brief Returns the index at which a given probe is mapped.
  /// \param probe The probe that is being queried.
  /// \param bForExtraction If set, returns whether the index can be used for using the probe during rendering. If the probe was just mapped but not updated yet, -1 will be returned for bForExtraction = true but a valid index for bForExtraction = false so that the index can be rendered into.
  /// \return Returns the mapped index in the atlas or -1 of the probe is not mapped.
  plInt32 GetReflectionIndex(plReflectionProbeId probe, bool bForExtraction = false) const;

  /// \brief Returns the atlas texture.
  /// \return The texture handle of the cube map atlas.
  plGALTextureHandle GetTexture() const { return m_hReflectionSpecularTexture; }

  ///@}
  /// \name Compute atlas mapping
  ///@{

  /// \brief Should be called in the PreExtraction phase. This will reset all probe weights.
  void PreExtraction();

  /// \brief Adds weight to a probe. Should be called during extraction of the probe. The mapping will map the probes with the highest weights in the atlas over time. This can be called multiple times in a frame for a probe if it is visible in multiple views. The maximum weight is then taken.
  void AddWeight(plReflectionProbeId probe, float fPriority);

  /// \brief Should be called in the PostExtraction phase. This will compute the best probe mapping and potentially fire plReflectionProbeMappingEvent events to map / unmap or request updates of probes.
  void PostExtraction();

  ///@}

public:
  plEvent<const plReflectionProbeMappingEvent&> m_Events;

private:
  struct plProbeMappingFlags
  {
    using StorageType = plUInt8;

    enum Enum
    {
      SkyLight = plProbeFlags::SkyLight,
      HasCustomCubeMap = plProbeFlags::HasCustomCubeMap,
      Sphere = plProbeFlags::Sphere,
      Box = plProbeFlags::Box,
      Dynamic = plProbeFlags::Dynamic,
      Dirty = PLASMA_BIT(5),
      Usable = PLASMA_BIT(6),
      Default = 0
    };

    struct Bits
    {
      StorageType SkyLight : 1;
      StorageType HasCustomCubeMap : 1;
      StorageType Sphere : 1;
      StorageType Box : 1;
      StorageType Dynamic : 1;
      StorageType Dirty : 1;
      StorageType Usable : 1;
    };
  };

  //PLASMA_DECLARE_FLAGS_OPERATORS(plProbeMappingFlags);

  struct SortedProbes
  {
    PLASMA_DECLARE_POD_TYPE();

    PLASMA_ALWAYS_INLINE bool operator<(const SortedProbes& other) const
    {
      if (m_fPriority > other.m_fPriority) // we want to sort descending (higher priority first)
        return true;

      return m_uiIndex < other.m_uiIndex;
    }

    plReflectionProbeId m_uiIndex;
    float m_fPriority = 0.0f;
  };

  struct ProbeDataInternal
  {
    plBitflags<plProbeMappingFlags> m_Flags;
    plInt32 m_uiReflectionIndex = -1;
    float m_fPriority = 0.0f;
    plReflectionProbeId m_id;
  };

private:
  void MapProbe(plReflectionProbeId id, plInt32 iReflectionIndex);
  void UnmapProbe(plReflectionProbeId id);

private:
  plDynamicArray<ProbeDataInternal> m_RegisteredProbes;
  plReflectionProbeId m_SkyLight;

  plUInt32 m_uiAtlasSize = 32;
  plDynamicArray<plReflectionProbeId> m_MappedCubes;

  // GPU Data
  plGALTextureHandle m_hReflectionSpecularTexture;

  // Cleared every frame:
  plDynamicArray<SortedProbes> m_SortedProbes; // All probes exiting in the scene, sorted by priority.
  plDynamicArray<SortedProbes> m_ActiveProbes; // Probes that are currently mapped in the atlas.
  plDynamicArray<plInt32> m_UnusedProbeSlots;  // Probe slots are are currently unused in the atlas.
  plDynamicArray<SortedProbes> m_AddProbes;    // Probes that should be added to the atlas
};
