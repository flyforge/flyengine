#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/SimdMath/SimdVec4i.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  plSpatialData::Category s_ProcVolumeCategory = plSpatialData::RegisterCategory("ProcVolume", plSpatialData::Flags::None);
  static plHashedString s_sVolumes = plMakeHashedString("Volumes");

  static const plEnum<plExpression::RegisterType> s_ApplyVolumesTypes[] = {
    plExpression::RegisterType::Float, // PosX
    plExpression::RegisterType::Float, // PosY
    plExpression::RegisterType::Float, // PosZ
    plExpression::RegisterType::Float, // InitialValue
    plExpression::RegisterType::Int,   // TagSetIndex
    plExpression::RegisterType::Int,   // ImageMode
    plExpression::RegisterType::Float, // RefColorR
    plExpression::RegisterType::Float, // RefColorG
    plExpression::RegisterType::Float, // RefColorB
    plExpression::RegisterType::Float, // RefColorA
  };

  static void ApplyVolumes(plExpression::Inputs inputs, plExpression::Output output, const plExpression::GlobalData& globalData)
  {
    const plVariantArray& volumes = globalData.GetValue(s_sVolumes)->Get<plVariantArray>();
    if (volumes.IsEmpty())
      return;

    plUInt32 uiTagSetIndex = inputs[4].GetPtr()->i.x();
    auto pVolumeCollection = plDynamicCast<const plVolumeCollection*>(volumes[uiTagSetIndex].Get<plReflectedClass*>());
    if (pVolumeCollection == nullptr)
      return;

    const plExpression::Register* pPosX = inputs[0].GetPtr();
    const plExpression::Register* pPosY = inputs[1].GetPtr();
    const plExpression::Register* pPosZ = inputs[2].GetPtr();
    const plExpression::Register* pPosXEnd = inputs[0].GetEndPtr();

    const plExpression::Register* pInitialValues = inputs[3].GetPtr();

    plProcVolumeImageMode::Enum imgMode = plProcVolumeImageMode::Default;
    plColor refColor = plColor::White;
    if (inputs.GetCount() >= 10)
    {
      imgMode = static_cast<plProcVolumeImageMode::Enum>(inputs[5].GetPtr()->i.x());

      const float refColR = inputs[6].GetPtr()->f.x();
      const float refColG = inputs[7].GetPtr()->f.x();
      const float refColB = inputs[8].GetPtr()->f.x();
      const float refColA = inputs[9].GetPtr()->f.x();
      refColor = plColor(refColR, refColG, refColB, refColA);
    }

    plExpression::Register* pOutput = output.GetPtr();

    plSimdMat4f helperMat;
    while (pPosX < pPosXEnd)
    {
      helperMat.SetRows(pPosX->f, pPosY->f, pPosZ->f, plSimdVec4f::MakeZero());

      const float x = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col0, pInitialValues->f.x(), imgMode, refColor);
      const float y = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col1, pInitialValues->f.y(), imgMode, refColor);
      const float z = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col2, pInitialValues->f.z(), imgMode, refColor);
      const float w = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col3, pInitialValues->f.w(), imgMode, refColor);
      pOutput->f.Set(x, y, z, w);

      ++pPosX;
      ++pPosY;
      ++pPosZ;
      ++pInitialValues;
      ++pOutput;
    }
  }

  static plResult ApplyVolumesValidate(const plExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const plVariant* pValue = globalData.GetValue("Volumes"))
      {
        if (pValue->GetType() == plVariantType::VariantArray)
        {
          return PLASMA_SUCCESS;
        }
      }
    }

    return PLASMA_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////

  static plHashedString s_sInstanceSeed = plMakeHashedString("InstanceSeed");

  static const plEnum<plExpression::RegisterType> s_GetInstanceSeedTypes = {};

  static void GetInstanceSeed(plExpression::Inputs inputs, plExpression::Output output, const plExpression::GlobalData& globalData)
  {
    int instanceSeed = globalData.GetValue(s_sInstanceSeed)->Get<int>();

    plExpression::Register* pOutput = output.GetPtr();
    plExpression::Register* pOutputEnd = output.GetEndPtr();

    while (pOutput < pOutputEnd)
    {
      pOutput->i.Set(instanceSeed);

      ++pOutput;
    }
  }

  static plResult GetInstanceSeedValidate(const plExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const plVariant* pValue = globalData.GetValue(s_sInstanceSeed))
      {
        if (pValue->GetType() == plVariantType::Int32)
        {
          return PLASMA_SUCCESS;
        }
      }
    }

    return PLASMA_FAILURE;
  }
} // namespace

plExpressionFunction plProcGenExpressionFunctions::s_ApplyVolumesFunc = {
  {plMakeHashedString("ApplyVolumes"), plMakeArrayPtr(s_ApplyVolumesTypes), 5, plExpression::RegisterType::Float},
  &ApplyVolumes,
  &ApplyVolumesValidate,
};

plExpressionFunction plProcGenExpressionFunctions::s_GetInstanceSeedFunc = {
  {plMakeHashedString("GetInstanceSeed"), plMakeArrayPtr(&s_GetInstanceSeedTypes, 0), 0, plExpression::RegisterType::Int},
  &GetInstanceSeed,
  &GetInstanceSeedValidate,
};

//////////////////////////////////////////////////////////////////////////

void plProcGenInternal::ExtractVolumeCollections(const plWorld& world, const plBoundingBox& box, const Output& output, plDeque<plVolumeCollection>& volumeCollections, plExpression::GlobalData& globalData)
{
  auto& volumeTagSetIndices = output.m_VolumeTagSetIndices;
  if (volumeTagSetIndices.IsEmpty())
    return;

  plVariantArray volumes;
  if (plVariant* volumesVar = globalData.GetValue(s_sVolumes))
  {
    volumes = volumesVar->Get<plVariantArray>();
  }

  for (plUInt8 tagSetIndex : volumeTagSetIndices)
  {
    if (tagSetIndex < volumes.GetCount() && volumes[tagSetIndex].IsValid())
    {
      continue;
    }

    auto pGraphSharedData = static_cast<const plProcGenInternal::GraphSharedData*>(output.m_pGraphSharedData.Borrow());
    auto& includeTags = pGraphSharedData->GetTagSet(tagSetIndex);

    auto& volumeCollection = volumeCollections.ExpandAndGetRef();
    plVolumeCollection::ExtractVolumesInBox(world, box, s_ProcVolumeCategory, includeTags, volumeCollection, plGetStaticRTTI<plProcVolumeComponent>());

    volumes.EnsureCount(tagSetIndex + 1);
    volumes[tagSetIndex] = plVariant(&volumeCollection);
  }

  globalData.Insert(s_sVolumes, volumes);
}

void plProcGenInternal::SetInstanceSeed(plUInt32 uiSeed, plExpression::GlobalData& globalData)
{
  globalData.Insert(s_sInstanceSeed, (int)uiSeed);
}