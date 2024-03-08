#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <KrautGenerator/Description/SpawnNodeDesc.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Material/MaterialResource.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plKrautAssetMaterial, plNoBase, 1, plRTTIDefaultAllocator<plKrautAssetMaterial>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new plReadOnlyAttribute()),
    PL_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plKrautBranchTypeMode, 1)
  PL_ENUM_CONSTANTS(plKrautBranchTypeMode::Regular, plKrautBranchTypeMode::Umbrella)
PL_END_STATIC_REFLECTED_ENUM

PL_BEGIN_STATIC_REFLECTED_ENUM(plKrautBranchTargetDir, 1)
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Straight),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Upwards),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Degree22),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Degree45),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Degree67),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Degree90),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Degree112),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Degree135),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Degree157),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir::Downwards),
PL_END_STATIC_REFLECTED_ENUM

PL_BEGIN_STATIC_REFLECTED_ENUM(plKrautLeafOrientation, 1)
  PL_ENUM_CONSTANT(plKrautLeafOrientation::Upwards),
  PL_ENUM_CONSTANT(plKrautLeafOrientation::AlongBranch),
  PL_ENUM_CONSTANT(plKrautLeafOrientation::OrthogonalToBranch),
PL_END_STATIC_REFLECTED_ENUM

PL_BEGIN_STATIC_REFLECTED_ENUM(plKrautFrondContourMode, 1)
  PL_ENUM_CONSTANT(plKrautFrondContourMode::Full),
  PL_ENUM_CONSTANT(plKrautFrondContourMode::Symetric),
  PL_ENUM_CONSTANT(plKrautFrondContourMode::InverseSymetric),
PL_END_STATIC_REFLECTED_ENUM

PL_BEGIN_STATIC_REFLECTED_ENUM(plKrautBranchTargetDir2Usage, 1)
  PL_ENUM_CONSTANT(plKrautBranchTargetDir2Usage::Off),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir2Usage::Relative),
  PL_ENUM_CONSTANT(plKrautBranchTargetDir2Usage::Absolute),
PL_END_STATIC_REFLECTED_ENUM

PL_BEGIN_STATIC_REFLECTED_TYPE(plKrautAssetBranchType, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    // === Administrative ===

    PL_MEMBER_PROPERTY("GrowSubBranchType1", m_bGrowSubBranchType1),
    PL_MEMBER_PROPERTY("GrowSubBranchType2", m_bGrowSubBranchType2),
    PL_MEMBER_PROPERTY("GrowSubBranchType3", m_bGrowSubBranchType3),

    // === Branch Type ===

    // General

    PL_MEMBER_PROPERTY("SegmentLength", m_uiSegmentLengthCM)->AddAttributes(new plDefaultValueAttribute(5), new plClampValueAttribute(1, 50), new plSuffixAttribute("cm"), new plGroupAttribute("General")),
    PL_ENUM_MEMBER_PROPERTY("BranchType", plKrautBranchTypeMode, m_BranchTypeMode),
    PL_MEMBER_PROPERTY("BranchlessPartABS", m_fBranchlessPartABS)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0, 10.0f)),
    PL_MEMBER_PROPERTY("BranchlessPartEndABS", m_fBranchlessPartEndABS)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0, 10.0f)),
    PL_MEMBER_PROPERTY("LowerBound", m_uiLowerBound)->AddAttributes(new plDefaultValueAttribute(0), new plClampValueAttribute(0, 100)),
    PL_MEMBER_PROPERTY("UpperBound", m_uiUpperBound)->AddAttributes(new plDefaultValueAttribute(100), new plClampValueAttribute(0, 100)),
    PL_MEMBER_PROPERTY("MinBranchThickness", m_uiMinBranchThicknessInCM)->AddAttributes(new plDefaultValueAttribute(20), new plClampValueAttribute(1, 100), new plSuffixAttribute("cm")),
    PL_MEMBER_PROPERTY("MaxBranchThickness", m_uiMaxBranchThicknessInCM)->AddAttributes(new plDefaultValueAttribute(20), new plClampValueAttribute(1, 100), new plSuffixAttribute("cm")),

    // Spawn Nodes

    PL_MEMBER_PROPERTY("MinBranchesPerNode", m_uiMinBranches)->AddAttributes(new plDefaultValueAttribute(4), new plClampValueAttribute(0, 32), new plGroupAttribute("Spawn Nodes")),
    PL_MEMBER_PROPERTY("MaxBranchesPerNode", m_uiMaxBranches)->AddAttributes(new plDefaultValueAttribute(4), new plClampValueAttribute(0, 32)),
    PL_MEMBER_PROPERTY("NodeSpacingBefore", m_fNodeSpacingBefore)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0, 5.0f)),
    PL_MEMBER_PROPERTY("NodeSpacingAfter", m_fNodeSpacingAfter)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0, 5.0f)),
    PL_MEMBER_PROPERTY("NodeHeight", m_fNodeHeight)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0, 5.0f)),
    // === Growth ===

    // Start Direction
    PL_MEMBER_PROPERTY("MaxRotationalDeviation", m_MaxRotationalDeviation)->AddAttributes(new plDefaultValueAttribute(plAngle()), new plClampValueAttribute(plAngle::MakeFromDegree(0), plAngle::MakeFromDegree(180)), new plGroupAttribute("Start Direction")),
    PL_MEMBER_PROPERTY("BranchAngle", m_BranchAngle)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(90)), new plClampValueAttribute(plAngle::MakeFromDegree(1), plAngle::MakeFromDegree(179))),
    PL_MEMBER_PROPERTY("MaxBranchAngleDeviation", m_MaxBranchAngleDeviation)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(10)), new plClampValueAttribute(plAngle::MakeFromDegree(0), plAngle::MakeFromDegree(90))),

    // Target Direction
    PL_ENUM_MEMBER_PROPERTY("TargetDirection", plKrautBranchTargetDir, m_TargetDirection)->AddAttributes(new plGroupAttribute("Target Direction")),
    PL_MEMBER_PROPERTY("TargetDirRelative", m_bTargetDirRelative),
    PL_ENUM_MEMBER_PROPERTY("TargetDir2Uage", plKrautBranchTargetDir2Usage, m_TargetDir2Uage),
    PL_MEMBER_PROPERTY("TargetDir2Offset", m_fTargetDir2Usage)->AddAttributes(new plDefaultValueAttribute(2.5f), new plClampValueAttribute(0.01f, 5.0f)),
    PL_ENUM_MEMBER_PROPERTY("TargetDirection2", plKrautBranchTargetDir, m_TargetDirection2),
    PL_MEMBER_PROPERTY("MaxTargetDirDeviation", m_MaxTargetDirDeviation)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(20)), new plClampValueAttribute(plAngle::MakeFromDegree(0), plAngle::MakeFromDegree(90))),

    // Growth
    PL_MEMBER_PROPERTY("MinBranchLength", m_uiMinBranchLengthInCM)->AddAttributes(new plDefaultValueAttribute(100), new plClampValueAttribute(1, 10000), new plSuffixAttribute("cm"), new plGroupAttribute("Branch Growth")),
    PL_MEMBER_PROPERTY("MaxBranchLength", m_uiMaxBranchLengthInCM)->AddAttributes(new plDefaultValueAttribute(100), new plClampValueAttribute(1, 10000), new plSuffixAttribute("cm")),
    PL_MEMBER_PROPERTY("MaxBranchLengthParentScale", m_MaxBranchLengthParentScale)->AddAttributes(new plColorAttribute(plColor::Brown), new plCurveExtentsAttribute(0.0, true, 1.0f, true), new plClampValueAttribute(0.0, 1.0), new plDefaultValueAttribute(1.0)),
    PL_MEMBER_PROPERTY("TargetDirDeviation", m_GrowMaxTargetDirDeviation)->AddAttributes(new plDefaultValueAttribute(plAngle()), new plClampValueAttribute(plAngle::MakeFromDegree(0), plAngle::MakeFromDegree(180))),
    PL_MEMBER_PROPERTY("DirChangePerSegment", m_GrowMaxDirChangePerSegment)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(5)), new plClampValueAttribute(plAngle::MakeFromDegree(0), plAngle::MakeFromDegree(90))),
    PL_MEMBER_PROPERTY("OnlyGrowUpAndDown", m_bRestrictGrowthToFrondPlane),

    // Obstacles

    //bool m_bActAsObstacle;
    //bool m_bDoPhysicalSimulation;
    //float m_fPhysicsLookAhead;
    //float m_fPhysicsEvasionAngle;

    // === Appearance ===

    // Branch Mesh

    PL_MEMBER_PROPERTY("EnableMesh", m_bEnableMesh)->AddAttributes(new plDefaultValueAttribute(true), new plGroupAttribute("Branch Mesh")),
    PL_MEMBER_PROPERTY("BranchMaterial", m_sBranchMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PL_MEMBER_PROPERTY("BranchContour", m_BranchContour)->AddAttributes(new plColorAttribute(plColor::Brown), new plCurveExtentsAttribute(0.0f, true, 1.0f, true), new plClampValueAttribute(0.1, 1.0), new plDefaultValueAttribute(1.0)),
    PL_MEMBER_PROPERTY("Roundness", m_fRoundnessFactor)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
    PL_MEMBER_PROPERTY("Flares", m_uiFlares)->AddAttributes(new plDefaultValueAttribute(0), new plClampValueAttribute(0, 16)),
    PL_MEMBER_PROPERTY("FlareWidth", m_fFlareWidth)->AddAttributes(new plDefaultValueAttribute(2.0f), new plClampValueAttribute(0.0f, 10.0f)),
    PL_MEMBER_PROPERTY("FlareWidthCurve", m_FlareWidthCurve)->AddAttributes(new plColorAttribute(plColor::FloralWhite), new plCurveExtentsAttribute(0.0, true, 1.0, true), new plClampValueAttribute(0.0, 1.0), new plDefaultValueAttribute(1.0)),
    PL_MEMBER_PROPERTY("FlareRotation", m_FlareRotation)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(0)), new plClampValueAttribute(plAngle::MakeFromDegree(-720), plAngle::MakeFromDegree(720))),
    PL_MEMBER_PROPERTY("RotateTexCoords", m_bRotateTexCoords)->AddAttributes(new plDefaultValueAttribute(true)),

    // Fronds

    PL_MEMBER_PROPERTY("EnableFronds", m_bEnableFronds)->AddAttributes(new plGroupAttribute("Fronds")),
    PL_MEMBER_PROPERTY("FrondMaterial", m_sFrondMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PL_MEMBER_PROPERTY("TextureRepeat", m_fTextureRepeat)->AddAttributes(new plClampValueAttribute(0.0f, 99.0f)),
    PL_ENUM_MEMBER_PROPERTY("FrondUpOrientation", plKrautLeafOrientation, m_FrondUpOrientation),
    PL_MEMBER_PROPERTY("FrondOrientationDeviation", m_MaxFrondOrientationDeviation)->AddAttributes(new plClampValueAttribute(plAngle::MakeFromDegree(0), plAngle::MakeFromDegree(180))),
    PL_MEMBER_PROPERTY("NumFronds", m_uiNumFronds)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(1, 16)),
    PL_MEMBER_PROPERTY("AlignFrondsOnSurface", m_bAlignFrondsOnSurface),
    PL_MEMBER_PROPERTY("FrondDetail", m_uiFrondDetail)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(0, 32)),
    PL_MEMBER_PROPERTY("FrondContour", m_FrondContour)->AddAttributes(new plColorAttribute(plColor::LawnGreen), new plCurveExtentsAttribute(0.0, true, 1.0, true), new plClampValueAttribute(0.0, 1.0), new plDefaultValueAttribute(1.0)),
    PL_ENUM_MEMBER_PROPERTY("FrondContourMode", plKrautFrondContourMode, m_FrondContourMode),
    PL_MEMBER_PROPERTY("FrondHeight", m_fFrondHeight)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 10.0f)),
    PL_MEMBER_PROPERTY("FrondHeightScale", m_FrondHeight)->AddAttributes(new plColorAttribute(plColor::LawnGreen), new plCurveExtentsAttribute(0.0, true, 1.0, true), new plClampValueAttribute(-1.0, 1.0)),
    PL_MEMBER_PROPERTY("FrondWidth", m_fFrondWidth)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 10.0f)),
    PL_MEMBER_PROPERTY("FrondWidthScale", m_FrondWidth)->AddAttributes(new plColorAttribute(plColor::LawnGreen), new plCurveExtentsAttribute(0.0, true, 1.0, true), new plClampValueAttribute(0.0, 1.0), new plDefaultValueAttribute(1.0)),
    //PL_MEMBER_PROPERTY("FrondVariationColor", m_FrondVariationColor)->AddAttributes(new plDefaultValueAttribute(plColor::White)),

    // Leaves

    PL_MEMBER_PROPERTY("EnableLeaves", m_bEnableLeaves)->AddAttributes(new plGroupAttribute("Leaves")),
    PL_MEMBER_PROPERTY("LeafMaterial", m_sLeafMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PL_MEMBER_PROPERTY("BillboardLeaves", m_bBillboardLeaves)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("LeafSize", m_fLeafSize)->AddAttributes(new plDefaultValueAttribute(0.25f), new plClampValueAttribute(0.01f, 10.0f)),
    PL_MEMBER_PROPERTY("LeafScale", m_LeafScale)->AddAttributes(new plColorAttribute(plColor::LightGreen), new plCurveExtentsAttribute(0.0, true, 1.0, true), new plClampValueAttribute(0.0, 1.0), new plDefaultValueAttribute(1.0)),
    PL_MEMBER_PROPERTY("LeafInterval", m_fLeafInterval)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0.0f, 10.0f)),
    //PL_MEMBER_PROPERTY("LeafVariationColor", m_LeafVariationColor)->AddAttributes(new plDefaultValueAttribute(plColor::White)),

    // Shared

    //plUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
    //plUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeAssetProperties, 1, plRTTIDefaultAllocator<plKrautTreeAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    //PL_MEMBER_PROPERTY("KrautFile", m_sKrautFile)->AddAttributes(new plFileBrowserAttribute("Select Kraut Tree file", "*.tree")),
    //PL_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    //PL_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new plDefaultValueAttribute(0.4f), new plClampValueAttribute(0.0f, 10.0f)),
    PL_MEMBER_PROPERTY("TreeStiffness", m_fTreeStiffness)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(1.0f, 10000.0f)),
    PL_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PL_ARRAY_MEMBER_PROPERTY("Materials", m_Materials)->AddAttributes(new plContainerAttribute(false, false, false)),
    PL_MEMBER_PROPERTY("DisplayRandomSeed", m_uiRandomSeedForDisplay),
    PL_ARRAY_MEMBER_PROPERTY("GoodRandomSeeds", m_GoodRandomSeeds),
    PL_MEMBER_PROPERTY("BT_Trunk1", m_BT_Trunk1),
    //PL_MEMBER_PROPERTY("BT_Trunk2", m_BT_Trunk2),
    //PL_MEMBER_PROPERTY("BT_Trunk3", m_BT_Trunk3),
    PL_MEMBER_PROPERTY("BT_MainBranch1", m_BT_MainBranch1),
    PL_MEMBER_PROPERTY("BT_MainBranch2", m_BT_MainBranch2),
    PL_MEMBER_PROPERTY("BT_MainBranch3", m_BT_MainBranch3),
    PL_MEMBER_PROPERTY("BT_SubBranch1", m_BT_SubBranch1),
    PL_MEMBER_PROPERTY("BT_SubBranch2", m_BT_SubBranch2),
    PL_MEMBER_PROPERTY("BT_SubBranch3", m_BT_SubBranch3),
    PL_MEMBER_PROPERTY("BT_Twig1", m_BT_Twig1),
    PL_MEMBER_PROPERTY("BT_Twig2", m_BT_Twig2),
    PL_MEMBER_PROPERTY("BT_Twig3", m_BT_Twig3),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plKrautTreeAssetProperties::plKrautTreeAssetProperties() = default;
plKrautTreeAssetProperties::~plKrautTreeAssetProperties() = default;

void CopyCurve(Kraut::Curve& dst, const plSingleCurveData& src, plUInt32 uiNumSamples, float fDefaultValue)
{
  if (src.m_ControlPoints.IsEmpty())
  {
    dst.Initialize(1, fDefaultValue, 0.0f, 1.0f);
  }
  else
  {
    plCurve1D c;
    src.ConvertToRuntimeData(c);
    c.SortControlPoints();
    c.CreateLinearApproximation();

    dst.Initialize(uiNumSamples, fDefaultValue, 0.0f, 1.0f);

    const double invSam = 1.0 / (uiNumSamples - 1);
    for (plUInt32 i = 0; i < uiNumSamples; ++i)
    {
      const double val = c.Evaluate(i * invSam);
      dst.m_Values[i] = (float)val;
    }
  }
}

void CopyConfig(Kraut::SpawnNodeDesc& nd, const plKrautAssetBranchType& bt, plDynamicArray<plKrautMaterialDescriptor>& materials, plKrautBranchType branchType)
{
  // === Administrative ===

  //bool m_bVisible;

  nd.m_bAllowSubType[0] = bt.m_bGrowSubBranchType1;
  nd.m_bAllowSubType[1] = bt.m_bGrowSubBranchType2;
  nd.m_bAllowSubType[2] = bt.m_bGrowSubBranchType3;

  nd.m_bEnable[Kraut::BranchGeometryType::Branch] = bt.m_bEnableMesh;
  nd.m_bEnable[Kraut::BranchGeometryType::Frond] = bt.m_bEnableFronds;
  nd.m_bEnable[Kraut::BranchGeometryType::Leaf] = bt.m_bEnableLeaves;

  if (bt.m_bEnableMesh && !bt.m_sBranchMaterial.IsEmpty())
  {
    auto& m = materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = plKrautMaterialType::Branch;
    m.m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(bt.m_sBranchMaterial);
  }

  if (bt.m_bEnableFronds && !bt.m_sFrondMaterial.IsEmpty())
  {
    auto& m = materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = plKrautMaterialType::Frond;
    m.m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(bt.m_sFrondMaterial);
    //m.m_VariationColor = bt.m_FrondVariationColor;// currently done through the material
  }

  if (bt.m_bEnableLeaves && !bt.m_sLeafMaterial.IsEmpty())
  {
    auto& m = materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = plKrautMaterialType::Leaf;
    m.m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(bt.m_sLeafMaterial);
    //m.m_VariationColor = bt.m_LeafVariationColor;// currently done through the material
  }

  // === Branch Type ===

  // General

  nd.m_iSegmentLengthCM = plMath::Clamp<plInt8>(bt.m_uiSegmentLengthCM, 1, 50);
  nd.m_BranchTypeMode = (Kraut::BranchTypeMode::Enum)bt.m_BranchTypeMode.GetValue();
  nd.m_fBranchlessPartABS = bt.m_fBranchlessPartABS;
  nd.m_fBranchlessPartEndABS = bt.m_fBranchlessPartEndABS;
  nd.m_uiLowerBound = plMath::Clamp<plUInt8>(bt.m_uiLowerBound, 0, 100);
  nd.m_uiUpperBound = plMath::Clamp<plUInt8>(bt.m_uiUpperBound, nd.m_uiLowerBound, 100);
  nd.m_uiMinBranchThicknessInCM = plMath::Clamp<plUInt16>(bt.m_uiMinBranchThicknessInCM, 1, 100);
  nd.m_uiMaxBranchThicknessInCM = plMath::Clamp<plUInt16>(bt.m_uiMaxBranchThicknessInCM, nd.m_uiMinBranchThicknessInCM, 100);

  // Spawn Nodes

  nd.m_uiMinBranches = plMath::Clamp<plUInt16>(bt.m_uiMinBranches, 0, 32);
  nd.m_uiMaxBranches = plMath::Clamp<plUInt16>(bt.m_uiMaxBranches, nd.m_uiMinBranches, 32);
  nd.m_fNodeSpacingBefore = bt.m_fNodeSpacingBefore;
  nd.m_fNodeSpacingAfter = bt.m_fNodeSpacingAfter;
  nd.m_fNodeHeight = bt.m_fNodeHeight;


  // === Growth ===

  // Start Direction

  nd.m_fMaxRotationalDeviation = bt.m_MaxRotationalDeviation.GetDegree();
  nd.m_fBranchAngle = bt.m_BranchAngle.GetDegree();
  nd.m_fMaxBranchAngleDeviation = bt.m_MaxBranchAngleDeviation.GetDegree();

  // Target Direction

  nd.m_TargetDirection = (Kraut::BranchTargetDir::Enum)bt.m_TargetDirection.GetValue();
  nd.m_bTargetDirRelative = bt.m_bTargetDirRelative;
  nd.m_TargetDir2Uage = (Kraut::BranchTargetDir2Usage::Enum)bt.m_TargetDir2Uage.GetValue();
  nd.m_fTargetDir2Usage = bt.m_fTargetDir2Usage;
  nd.m_TargetDirection2 = (Kraut::BranchTargetDir::Enum)bt.m_TargetDirection2.GetValue();
  nd.m_fMaxTargetDirDeviation = bt.m_MaxTargetDirDeviation.GetDegree();

  // Growth

  nd.m_uiMinBranchLengthInCM = plMath::Clamp<plUInt16>(bt.m_uiMinBranchLengthInCM, 1, 10000);
  nd.m_uiMaxBranchLengthInCM = plMath::Clamp<plUInt16>(bt.m_uiMaxBranchLengthInCM, nd.m_uiMinBranchLengthInCM, 10000);
  CopyCurve(nd.m_MaxBranchLengthParentScale, bt.m_MaxBranchLengthParentScale, 20, 1.0f);
  nd.m_fGrowMaxTargetDirDeviation = bt.m_GrowMaxTargetDirDeviation.GetDegree();
  nd.m_fGrowMaxDirChangePerSegment = bt.m_GrowMaxDirChangePerSegment.GetDegree();
  nd.m_bRestrictGrowthToFrondPlane = bt.m_bRestrictGrowthToFrondPlane;

  // Obstacles

  //bool m_bActAsObstacle;
  //bool m_bDoPhysicalSimulation;
  //float m_fPhysicsLookAhead;
  //float m_fPhysicsEvasionAngle;


  // === Appearance ===

  // Branch Mesh

  CopyCurve(nd.m_BranchContour, bt.m_BranchContour, 50, 1.0f);
  nd.m_fRoundnessFactor = bt.m_fRoundnessFactor;
  nd.m_uiFlares = bt.m_uiFlares;
  nd.m_fFlareWidth = bt.m_fFlareWidth;
  CopyCurve(nd.m_FlareWidthCurve, bt.m_FlareWidthCurve, 50, 1.0f);
  nd.m_fFlareRotation = bt.m_FlareRotation.GetDegree();
  nd.m_bRotateTexCoords = bt.m_bRotateTexCoords;

  // Fronds

  nd.m_fTextureRepeat = bt.m_fTextureRepeat;
  nd.m_FrondUpOrientation = (Kraut::LeafOrientation::Enum)bt.m_FrondUpOrientation.GetValue();
  nd.m_uiMaxFrondOrientationDeviation = (plUInt8)bt.m_MaxFrondOrientationDeviation.GetDegree();
  nd.m_uiNumFronds = bt.m_uiNumFronds;
  nd.m_bAlignFrondsOnSurface = bt.m_bAlignFrondsOnSurface;
  nd.m_uiFrondDetail = bt.m_uiFrondDetail;
  CopyCurve(nd.m_FrondContour, bt.m_FrondContour, 40, 1.0f);
  nd.m_FrondContourMode = (Kraut::SpawnNodeDesc::FrondContourMode)bt.m_FrondContourMode.GetValue();
  nd.m_fFrondHeight = bt.m_fFrondHeight;
  CopyCurve(nd.m_FrondHeight, bt.m_FrondHeight, 50, 0.0f);
  nd.m_fFrondWidth = bt.m_fFrondWidth;
  CopyCurve(nd.m_FrondWidth, bt.m_FrondWidth, 50, 1.0f);

  // Leaves

  nd.m_bBillboardLeaves = bt.m_bBillboardLeaves;
  nd.m_fLeafSize = bt.m_fLeafSize;
  CopyCurve(nd.m_LeafScale, bt.m_LeafScale, 25, 1.0f);
  nd.m_fLeafInterval = bt.m_fLeafInterval;

  // Shared
  //plUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  //plUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
}
