#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkeletonAssetDocument, 10, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static plTransform CalculateTransformationMatrix(const plEditableSkeleton* pProp)
{
  const float us = plMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const plBasisAxis::Enum rightDir = pProp->m_RightDir;
  const plBasisAxis::Enum upDir = pProp->m_UpDir;
  plBasisAxis::Enum forwardDir = plBasisAxis::GetOrthogonalAxis(rightDir, upDir, !pProp->m_bFlipForwardDir);

  plTransform t;
  t.SetIdentity();
  t.m_vScale.Set(us);

  // prevent mirroring in the rotation matrix, because we can't generate a quaternion from that
  if (!pProp->m_bFlipForwardDir)
  {
    switch (forwardDir)
    {
      case plBasisAxis::PositiveX:
        forwardDir = plBasisAxis::NegativeX;
        t.m_vScale.x *= -1;
        break;
      case plBasisAxis::PositiveY:
        forwardDir = plBasisAxis::NegativeY;
        t.m_vScale.y *= -1;
        break;
      case plBasisAxis::PositiveZ:
        forwardDir = plBasisAxis::NegativeZ;
        t.m_vScale.z *= -1;
        break;
      case plBasisAxis::NegativeX:
        forwardDir = plBasisAxis::PositiveX;
        t.m_vScale.x *= -1;
        break;
      case plBasisAxis::NegativeY:
        forwardDir = plBasisAxis::PositiveY;
        t.m_vScale.y *= -1;
        break;
      case plBasisAxis::NegativeZ:
        forwardDir = plBasisAxis::PositiveZ;
        t.m_vScale.z *= -1;
        break;
    }
  }

  plMat3 rot = plBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, 1.0f);
  t.m_qRotation = plQuat::MakeFromMat3(rot);

  return t;
}

plSkeletonAssetDocument::plSkeletonAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plEditableSkeleton>(sDocumentPath, plAssetDocEngineConnection::Simple, true)
{
}

plSkeletonAssetDocument::~plSkeletonAssetDocument() = default;

void plSkeletonAssetDocument::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plEditableSkeletonJoint>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool overrideSurface = e.m_pObject->GetTypeAccessor().GetValue("OverrideSurface").ConvertTo<bool>();
    const bool overrideCollisionLayer = e.m_pObject->GetTypeAccessor().GetValue("OverrideCollisionLayer").ConvertTo<bool>();
    const plSkeletonJointType::Enum jointType = (plSkeletonJointType::Enum)e.m_pObject->GetTypeAccessor().GetValue("JointType").ConvertTo<plInt32>();

    const bool bHasStiffness = jointType == plSkeletonJointType::SwingTwist;
    const bool bHasSwing = jointType == plSkeletonJointType::SwingTwist;
    const bool bHasTwist = jointType == plSkeletonJointType::SwingTwist;

    props["CollisionLayer"].m_Visibility = overrideCollisionLayer ? plPropertyUiState::Default : plPropertyUiState::Invisible;
    props["Surface"].m_Visibility = overrideSurface ? plPropertyUiState::Default : plPropertyUiState::Invisible;

    props["LocalRotation"].m_Visibility = bHasStiffness ? plPropertyUiState::Default : plPropertyUiState::Invisible;
    props["Stiffness"].m_Visibility = bHasStiffness ? plPropertyUiState::Default : plPropertyUiState::Invisible;
    props["SwingLimitY"].m_Visibility = bHasSwing ? plPropertyUiState::Default : plPropertyUiState::Invisible;
    props["SwingLimitZ"].m_Visibility = bHasSwing ? plPropertyUiState::Default : plPropertyUiState::Invisible;
    props["TwistLimitHalfAngle"].m_Visibility = bHasTwist ? plPropertyUiState::Default : plPropertyUiState::Invisible;
    props["TwistLimitCenterAngle"].m_Visibility = bHasTwist ? plPropertyUiState::Default : plPropertyUiState::Invisible;

    return;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plEditableSkeletonBoneShape>())
  {
    auto& props = *e.m_pPropertyStates;

    const plSkeletonJointGeometryType::Enum geomType = (plSkeletonJointGeometryType::Enum)e.m_pObject->GetTypeAccessor().GetValue("Geometry").ConvertTo<plInt32>();

    props["Offset"].m_Visibility = plPropertyUiState::Invisible;
    props["Rotation"].m_Visibility = plPropertyUiState::Invisible;
    props["Length"].m_Visibility = plPropertyUiState::Invisible;
    props["Width"].m_Visibility = plPropertyUiState::Invisible;
    props["Thickness"].m_Visibility = plPropertyUiState::Invisible;

    if (geomType == plSkeletonJointGeometryType::None)
      return;

    props["Length"].m_sNewLabelText = "Length";
    props["Width"].m_sNewLabelText = "Width";
    props["Thickness"].m_sNewLabelText = "Thickness";

    props["Offset"].m_Visibility = plPropertyUiState::Default;
    props["Rotation"].m_Visibility = plPropertyUiState::Default;

    if (geomType == plSkeletonJointGeometryType::Box)
    {
      props["Length"].m_Visibility = plPropertyUiState::Default;
      props["Width"].m_Visibility = plPropertyUiState::Default;
      props["Thickness"].m_Visibility = plPropertyUiState::Default;
    }
    else if (geomType == plSkeletonJointGeometryType::Sphere)
    {
      props["Thickness"].m_Visibility = plPropertyUiState::Default;
      props["Thickness"].m_sNewLabelText = "Radius";
    }
    else if (geomType == plSkeletonJointGeometryType::Capsule)
    {
      props["Length"].m_Visibility = plPropertyUiState::Default;

      props["Thickness"].m_Visibility = plPropertyUiState::Default;
      props["Thickness"].m_sNewLabelText = "Radius";
    }

    return;
  }
}

plStatus plSkeletonAssetDocument::WriteResource(plStreamWriter& inout_stream, const plEditableSkeleton& skeleton) const
{
  plSkeletonResourceDescriptor desc;
  desc.m_RootTransform = CalculateTransformationMatrix(&skeleton);
  skeleton.FillResourceDescriptor(desc);

  PL_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return plStatus(PL_SUCCESS);
}

void plSkeletonAssetDocument::SetRenderBones(bool bEnable)
{
  if (m_bRenderBones == bEnable)
    return;

  m_bRenderBones = bEnable;

  plSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void plSkeletonAssetDocument::SetRenderColliders(bool bEnable)
{
  if (m_bRenderColliders == bEnable)
    return;

  m_bRenderColliders = bEnable;

  plSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void plSkeletonAssetDocument::SetRenderJoints(bool bEnable)
{
  if (m_bRenderJoints == bEnable)
    return;

  m_bRenderJoints = bEnable;

  plSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void plSkeletonAssetDocument::SetRenderSwingLimits(bool bEnable)
{
  if (m_bRenderSwingLimits == bEnable)
    return;

  m_bRenderSwingLimits = bEnable;

  plSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void plSkeletonAssetDocument::SetRenderTwistLimits(bool bEnable)
{
  if (m_bRenderTwistLimits == bEnable)
    return;

  m_bRenderTwistLimits = bEnable;

  plSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void plSkeletonAssetDocument::SetRenderPreviewMesh(bool bEnable)
{
  if (m_bRenderPreviewMesh == bEnable)
    return;

  m_bRenderPreviewMesh = bEnable;

  plSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void plSkeletonAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // expose all the bones as parameters
  // such that we can create components that modify these bones

  auto* desc = GetProperties();
  plExposedParameters* pExposedParams = PL_DEFAULT_NEW(plExposedParameters);


  {
    plExposedBone bone;
    bone.m_sName = "<root-transform>";
    bone.m_Transform = CalculateTransformationMatrix(desc);

    plExposedParameter* param = PL_DEFAULT_NEW(plExposedParameter);
    param->m_sName = "<root-transform>";
    param->m_DefaultValue.CopyTypedObject(&bone, plGetStaticRTTI<plExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);
  }

  auto Traverse = [&](plEditableSkeletonJoint* pJoint, const char* szParent, auto recurse) -> void {
    plExposedBone bone;
    bone.m_sName = pJoint->GetName();
    bone.m_sParent = szParent;
    bone.m_Transform = pJoint->m_LocalTransform;

    plExposedParameter* param = PL_DEFAULT_NEW(plExposedParameter);
    param->m_sName = pJoint->GetName();
    param->m_DefaultValue.CopyTypedObject(&bone, plGetStaticRTTI<plExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);

    for (auto pChild : pJoint->m_Children)
    {
      recurse(pChild, pJoint->GetName(), recurse);
    }
  };

  for (auto ptr : desc->m_Children)
  {
    Traverse(ptr, "", Traverse);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

plTransformStatus plSkeletonAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  {
    m_bIsTransforming = true;
    PL_SCOPE_EXIT(m_bIsTransforming = false);

    plProgressRange range("Transforming Asset", 3, false);

    plEditableSkeleton* pProp = GetProperties();

    plStringBuilder sAbsFilename = pProp->m_sSourceFile;

    if (sAbsFilename.IsEmpty())
    {
      range.BeginNextStep("Writing Result");
      PL_SUCCEED_OR_RETURN(WriteResource(stream, *GetProperties()));
    }
    else
    {
      if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
      {
        return plStatus(plFmt("Couldn't make path absolute: '{0};", sAbsFilename));
      }

      plUniquePtr<plModelImporter2::Importer> pImporter = plModelImporter2::RequestImporterForFileType(sAbsFilename);
      if (pImporter == nullptr)
        return plStatus("No known importer for this file type.");

      range.BeginNextStep("Importing Source File");

      plEditableSkeleton newSkeleton;

      plModelImporter2::ImportOptions opt;
      opt.m_sSourceFile = sAbsFilename;
      opt.m_pSkeletonOutput = &newSkeleton;

      if (pImporter->Import(opt).Failed())
        return plStatus("Model importer was unable to read this asset.");

      range.BeginNextStep("Importing Skeleton Data");

      // synchronize the old data (collision geometry etc.) with the new hierarchy
      const plEditableSkeleton* pFinalSkeleton = MergeWithNewSkeleton(newSkeleton);

      range.BeginNextStep("Writing Result");
      PL_SUCCEED_OR_RETURN(WriteResource(stream, *pFinalSkeleton));

      // merge the new data with the actual asset document
      ApplyNativePropertyChangesToObjectManager(true);
    }
  }

  plSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plSkeletonAssetEvent::Transformed;
  m_Events.Broadcast(e);

  return plStatus(PL_SUCCESS);
}

plTransformStatus plSkeletonAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  // the preview mesh is an editor side only option, so the thumbnail context doesn't know anything about this
  // until we explicitly tell it about the mesh
  // without sending this here, thumbnails wouldn't look as desired, for assets transformed in the background
  if (!GetProperties()->m_sPreviewMesh.IsEmpty())
  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload = GetProperties()->m_sPreviewMesh;
    SendMessageToEngine(&msg);
  }

  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

const plEditableSkeleton* plSkeletonAssetDocument::MergeWithNewSkeleton(plEditableSkeleton& newSkeleton)
{
  plEditableSkeleton* pOldSkeleton = GetProperties();
  plMap<plString, const plEditableSkeletonJoint*> prevJoints;

  // map all old joints by name
  {
    auto TraverseJoints = [&prevJoints](const auto& self, plEditableSkeletonJoint* pJoint) -> void {
      prevJoints[pJoint->GetName()] = pJoint;

      for (plEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild);
      }
    };

    for (plEditableSkeletonJoint* pChild : pOldSkeleton->m_Children)
    {
      TraverseJoints(TraverseJoints, pChild);
    }
  }

  // copy old properties to new skeleton
  {
    auto TraverseJoints = [&prevJoints](const auto& self, plEditableSkeletonJoint* pJoint, const plTransform& root, plTransform origin) -> void {
      auto it = prevJoints.Find(pJoint->GetName());
      if (it.IsValid())
      {
        pJoint->CopyPropertiesFrom(it.Value());
      }

      // use the parent rotation as the gizmo base rotation
      plMat4 modelTransform, fullTransform;
      modelTransform = origin.GetAsMat4();
      plMsgAnimationPoseUpdated::ComputeFullBoneTransform(root.GetAsMat4(), modelTransform, fullTransform, pJoint->m_qGizmoOffsetRotationRO);

      origin = plTransform::MakeGlobalTransform(origin, pJoint->m_LocalTransform);
      pJoint->m_vGizmoOffsetPositionRO = root.TransformPosition(origin.m_vPosition);

      for (plEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild, root, origin);
      }
    };

    for (plEditableSkeletonJoint* pChild : newSkeleton.m_Children)
    {
      TraverseJoints(TraverseJoints, pChild, CalculateTransformationMatrix(pOldSkeleton), plTransform::MakeIdentity());
    }
  }

  // get rid of all old joints
  pOldSkeleton->ClearJoints();

  // move the new top level joints over to our own skeleton
  pOldSkeleton->m_Children = newSkeleton.m_Children;
  newSkeleton.m_Children.Clear(); // prevent this skeleton from deallocating the joints

  return pOldSkeleton;
}


//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkeletonAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plSkeletonAssetDocumentGenerator>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSkeletonAssetDocumentGenerator::plSkeletonAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

plSkeletonAssetDocumentGenerator::~plSkeletonAssetDocumentGenerator() = default;

void plSkeletonAssetDocumentGenerator::GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::Undecided;
    info.m_sName = "SkeletonImport";
    info.m_sIcon = ":/AssetIcons/Skeleton.svg";
  }
}

plStatus plSkeletonAssetDocumentGenerator::Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments)
{
  plStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  plOSFile::FindFreeFilename(sOutFile);

  auto pApp = plQtEditorApp::GetSingleton();

  plStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  plDocument* pDoc = pApp->CreateDocument(sOutFile, plDocumentFlags::None);
  if (pDoc == nullptr)
    return plStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  plSkeletonAssetDocument* pAssetDoc = plDynamicCast<plSkeletonAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("File", sInputFileRel.GetView());

  return plStatus(PL_SUCCESS);
}
