#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <Utilities/FileFormats/OBJLoader.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGizmoHandle, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Visible", m_bVisible),
    PL_MEMBER_PROPERTY("Transformation", m_Transformation),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEngineGizmoHandle, 1, plRTTIDefaultAllocator<plEngineGizmoHandle>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("HandleType", m_iHandleType),
    PL_MEMBER_PROPERTY("HandleMesh", m_sGizmoHandleMesh),
    PL_MEMBER_PROPERTY("Color", m_Color),
    PL_MEMBER_PROPERTY("ConstantSize", m_bConstantSize),
    PL_MEMBER_PROPERTY("AlwaysOnTop", m_bAlwaysOnTop),
    PL_MEMBER_PROPERTY("Visualizer", m_bVisualizer),
    PL_MEMBER_PROPERTY("Ortho", m_bShowInOrtho),
    PL_MEMBER_PROPERTY("Pickable", m_bIsPickable),
    PL_MEMBER_PROPERTY("FaceCam", m_bFaceCamera),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plGizmoHandle::plGizmoHandle()
{
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero(); // make sure it is different from anything valid
}

void plGizmoHandle::SetVisible(bool bVisible)
{
  if (bVisible != m_bVisible)
  {
    m_bVisible = bVisible;
    SetModified(true);
  }
}

void plGizmoHandle::SetTransformation(const plTransform& m)
{
  if (m_Transformation != m)
  {
    m_Transformation = m;
    if (m_bVisible)
      SetModified(true);
  }
}

void plGizmoHandle::SetTransformation(const plMat4& m)
{
  plTransform t = plTransform::MakeFromMat4(m);
  SetTransformation(t);
}

static plMeshBufferResourceHandle CreateMeshBufferResource(plGeometry& inout_geom, const char* szResourceName, const char* szDescription, plGALPrimitiveTopology::Enum topology)
{
  inout_geom.ComputeFaceNormals();
  inout_geom.ComputeSmoothVertexNormals();

  plMeshBufferResourceDescriptor desc;
  desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
  desc.AddStream(plGALVertexAttributeSemantic::Color0, plGALResourceFormat::RGBAUByteNormalized);
  desc.AddStream(plGALVertexAttributeSemantic::Normal, plGALResourceFormat::XYZFloat);
  desc.AllocateStreamsFromGeometry(inout_geom, topology);
  desc.ComputeBounds();

  return plResourceManager::CreateResource<plMeshBufferResource>(szResourceName, std::move(desc), szDescription);
}

static plMeshBufferResourceHandle CreateMeshBufferArrow()
{
  const char* szResourceName = "{B9DC6776-38D8-4C1F-994F-225E69E71283}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.02f;
  const float fLength = 1.0f;

  plGeometry::GeoOptions opt;
  opt.m_Transform = plMat4::MakeRotationY(plAngle::MakeFromDegree(90));

  plGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(plVec3(fLength * 0.5f, 0, 0));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Arrow", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferPiston()
{
  const char* szResourceName = "{E2B59B8F-8F61-48C0-AE37-CF31107BA2CE}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.02f;
  const float fLength = 1.0f;

  plGeometry::GeoOptions opt;
  opt.m_Transform = plMat4::MakeRotationY(plAngle::MakeFromDegree(90));

  plGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(plVec3(fLength * 0.5f, 0, 0));
  geom.AddBox(plVec3(fThickness * 5.0f), false, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Piston", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferHalfPiston()
{
  const char* szResourceName = "{BA17D025-B280-4940-8DFD-5486B0E4B41B}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.04f;
  const float fLength = 1.0f;

  plGeometry::GeoOptions opt;
  opt.m_Transform = plMat4::MakeRotationY(plAngle::MakeFromDegree(90));
  opt.m_Transform.SetTranslationVector(plVec3(fLength * 0.5f, 0, 0));

  plGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(plVec3(fLength, 0, 0));
  geom.AddBox(plVec3(fThickness * 5.0f), false, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_HalfPiston", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferRect()
{
  const char* szResourceName = "{75597E89-CDEE-4C90-A377-9441F64B9DB2}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  // weird size because of translate gizmo, should be fixed through scaling there instead
  const float fLength = 2.0f / 3.0f;

  plGeometry geom;
  geom.AddRectXY(plVec2(fLength));

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Rect", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferLineRect()
{
  const char* szResourceName = "{A1EA52B0-DA73-4176-B50D-3470DDB053F8}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plMat4 m;
  m.SetIdentity();

  plGeometry geom;

  const plVec2 halfSize(1.0f);

  geom.AddVertex(plVec3(-halfSize.x, -halfSize.y, 0), plVec3(0, 0, 1), plVec2(0, 1), plColor::White, 0, m);
  geom.AddVertex(plVec3(halfSize.x, -halfSize.y, 0), plVec3(0, 0, 1), plVec2(0, 0), plColor::White, 0, m);
  geom.AddVertex(plVec3(halfSize.x, halfSize.y, 0), plVec3(0, 0, 1), plVec2(1, 0), plColor::White, 0, m);
  geom.AddVertex(plVec3(-halfSize.x, halfSize.y, 0), plVec3(0, 0, 1), plVec2(1, 1), plColor::White, 0, m);

  geom.AddLine(0, 1);
  geom.AddLine(1, 2);
  geom.AddLine(2, 3);
  geom.AddLine(3, 0);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineRect", plGALPrimitiveTopology::Lines);
}

static plMeshBufferResourceHandle CreateMeshBufferRing()
{
  const char* szResourceName = "{EA8677E3-F623-4FD8-BFAB-349CE1BEB3CA}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fInnerRadius = 1.3f;
  const float fOuterRadius = fInnerRadius + 0.1f;

  plMat4 m;
  m.SetIdentity();

  plGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, 32, 8, false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Ring", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferBox()
{
  const char* szResourceName = "{F14D4CD3-8F21-442B-B07F-3567DBD58A3F}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plGeometry geom;
  geom.AddBox(plVec3(1.0f), false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Box", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferLineBox()
{
  const char* szResourceName = "{55DF000E-EE88-4BDC-8A7B-FA496941064E}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plGeometry geom;
  geom.AddLineBox(plVec3(1.0f));

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineBox", plGALPrimitiveTopology::Lines);
}

static plMeshBufferResourceHandle CreateMeshBufferSphere()
{
  const char* szResourceName = "{A88779B0-4728-4411-A9D7-532AFE6F4704}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plGeometry geom;
  geom.AddGeodesicSphere(1.0f, 2);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Sphere", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferCylinderZ()
{
  const char* szResourceName = "{3BBE2251-0DE4-4B71-979E-A407D8F5CB59}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plGeometry geom;
  geom.AddCylinderOnePiece(1.0f, 1.0f, 0.5f, 0.5f, 16);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_CylinderZ", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferHalfSphereZ()
{
  const char* szResourceName = "{05BDED8B-96C1-4F2E-8F1B-5C07B3C28D22}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plGeometry geom;
  geom.AddHalfSphere(1.0f, 16, 8, false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_HalfSphereZ", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferBoxFaces()
{
  const char* szResourceName = "{BD925A8E-480D-41A6-8F62-0AC5F72DA4F6}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plGeometry geom;
  plGeometry::GeoOptions opt;
  opt.m_Transform = plMat4::MakeTranslation(plVec3(0, 0, 0.5f));

  geom.AddRectXY(plVec2(0.5f), 1, 1, opt);

  opt.m_Transform = plMat4::MakeRotationY(plAngle::MakeFromDegree(180.0));
  opt.m_Transform.SetTranslationVector(plVec3(0, 0, -0.5f));
  geom.AddRectXY(plVec2(0.5f), 1, 1, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxFaces", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferBoxEdges()
{
  const char* szResourceName = "{FE700F28-514E-4193-A0F6-4351E0BAC222}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plMat4 rot;

  plGeometry geom;
  plGeometry::GeoOptions opt;

  for (plUInt32 i = 0; i < 4; ++i)
  {
    rot = plMat4::MakeRotationY(plAngle::MakeFromDegree(90.0f * i));

    opt.m_Transform = plMat4::MakeTranslation(plVec3(0.5f - 0.125f, 0, 0.5f));
    opt.m_Transform = rot * opt.m_Transform;
    geom.AddRectXY(plVec2(0.25f, 0.5f), 1, 1, opt);

    opt.m_Transform = plMat4::MakeTranslation(plVec3(-0.5f + 0.125f, 0, 0.5f));
    geom.AddRectXY(plVec2(0.25f, 0.5f), 1, 1, opt);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxEdges", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferBoxCorners()
{
  const char* szResourceName = "{FBDB6A82-D4B0-447F-815B-228D340451CB}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plMat4 rot[6];
  rot[0].SetIdentity();
  rot[1] = plMat4::MakeRotationX(plAngle::MakeFromDegree(90));
  rot[2] = plMat4::MakeRotationX(plAngle::MakeFromDegree(180));
  rot[3] = plMat4::MakeRotationX(plAngle::MakeFromDegree(270));
  rot[4] = plMat4::MakeRotationY(plAngle::MakeFromDegree(90));
  rot[5] = plMat4::MakeRotationY(plAngle::MakeFromDegree(-90));

  plGeometry geom;
  plGeometry::GeoOptions opt;

  for (plUInt32 i = 0; i < 6; ++i)
  {
    opt.m_Transform = plMat4::MakeTranslation(plVec3(0.5f - 0.125f, 0.5f - 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRectXY(plVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = plMat4::MakeTranslation(plVec3(0.5f - 0.125f, -0.5f + 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRectXY(plVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = plMat4::MakeTranslation(plVec3(-0.5f + 0.125f, 0.5f - 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRectXY(plVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = plMat4::MakeTranslation(plVec3(-0.5f + 0.125f, -0.5f + 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRectXY(plVec2(0.25f, 0.25f), 1, 1, opt);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxCorners", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferCone()
{
  const char* szResourceName = "{BED97C9E-4E7A-486C-9372-1FB1A5FAE786}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plGeometry::GeoOptions opt;
  opt.m_Transform = plMat4::MakeRotationY(plAngle::MakeFromDegree(270.0f));
  opt.m_Transform.SetTranslationVector(plVec3(1.0f, 0, 0));

  plGeometry geom;
  geom.AddCone(1.0f, 1.0f, false, 16, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Cone", plGALPrimitiveTopology::Triangles);
}

static plMeshBufferResourceHandle CreateMeshBufferFrustum()
{
  const char* szResourceName = "{61A7BE38-797D-4BFC-AED6-33CE4F4C6FF6}";

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plMat4 m;
  m.SetIdentity();

  plGeometry geom;

  geom.AddVertex(plVec3(0, 0, 0), plVec3(0, 0, 1), plVec2(0), plColor::White, 0, m);

  geom.AddVertex(plVec3(1.0f, -1.0f, 1.0f), plVec3(0, 0, 1), plVec2(0), plColor::White, 0, m);
  geom.AddVertex(plVec3(1.0f, 1.0f, 1.0f), plVec3(0, 0, 1), plVec2(0), plColor::White, 0, m);
  geom.AddVertex(plVec3(1.0f, -1.0f, -1.0f), plVec3(0, 0, 1), plVec2(0), plColor::White, 0, m);
  geom.AddVertex(plVec3(1.0f, 1.0f, -1.0f), plVec3(0, 0, 1), plVec2(0), plColor::White, 0, m);

  geom.AddLine(0, 1);
  geom.AddLine(0, 2);
  geom.AddLine(0, 3);
  geom.AddLine(0, 4);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Frustum", plGALPrimitiveTopology::Lines);
}

static plMeshBufferResourceHandle CreateMeshBufferFromFile(const char* szFile)
{
  const char* szResourceName = szFile;

  plMeshBufferResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plOBJLoader obj;
  obj.LoadOBJ(szFile, true).AssertSuccess("Couldn't load gizmo model '{}'", szFile);

  plMat4 m;
  m.SetIdentity();

  plGeometry geom;
  for (plUInt32 v = 0; v < obj.m_Positions.GetCount(); ++v)
  {
    geom.AddVertex(obj.m_Positions[v], plVec3::MakeZero(), plVec2::MakeZero(), plColor::White);
  }

  plStaticArray<plUInt32, 3> triangle;
  triangle.SetCount(3);
  for (plUInt32 f = 0; f < obj.m_Faces.GetCount(); ++f)
  {
    triangle[0] = obj.m_Faces[f].m_Vertices[0].m_uiPositionID;
    triangle[1] = obj.m_Faces[f].m_Vertices[1].m_uiPositionID;
    triangle[2] = obj.m_Faces[f].m_Vertices[2].m_uiPositionID;

    geom.AddPolygon(triangle, false);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_FromFile", plGALPrimitiveTopology::Triangles);
}

static plMeshResourceHandle CreateMeshResource(const char* szMeshResourceName, plMeshBufferResourceHandle hMeshBuffer, const char* szMaterial)
{
  const plStringBuilder sIdentifier(szMeshResourceName, "-with-", szMaterial);

  plMeshResourceHandle hMesh = plResourceManager::GetExistingResource<plMeshResource>(sIdentifier);

  if (hMesh.IsValid())
    return hMesh;

  plResourceLock<plMeshBufferResource> pMeshBuffer(hMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);

  plMeshResourceDescriptor md;
  md.UseExistingMeshBuffer(hMeshBuffer);
  md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
  md.SetMaterial(0, szMaterial);
  md.ComputeBounds();

  return plResourceManager::GetOrCreateResource<plMeshResource>(sIdentifier, std::move(md), pMeshBuffer->GetResourceDescription());
}

plEngineGizmoHandle::plEngineGizmoHandle() = default;

plEngineGizmoHandle::~plEngineGizmoHandle()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

void plEngineGizmoHandle::ConfigureHandle(plGizmo* pParentGizmo, plEngineGizmoHandleType type, const plColor& col, plBitflags<plGizmoFlags> flags, const char* szCustomMesh)
{
  SetParentGizmo(pParentGizmo);

  m_iHandleType = (int)type;
  m_sGizmoHandleMesh = szCustomMesh;
  m_Color = col;

  m_bConstantSize = flags.IsSet(plGizmoFlags::ConstantSize);
  m_bAlwaysOnTop = flags.IsSet(plGizmoFlags::OnTop);
  m_bVisualizer = flags.IsSet(plGizmoFlags::Visualizer);
  m_bShowInOrtho = flags.IsSet(plGizmoFlags::ShowInOrtho);
  m_bIsPickable = flags.IsSet(plGizmoFlags::Pickable);
  m_bFaceCamera = flags.IsSet(plGizmoFlags::FaceCamera);
}

bool plEngineGizmoHandle::SetupForEngine(plWorld* pWorld, plUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;

  if (!m_hGameObject.IsInvalidated())
    return false;

  plMeshBufferResourceHandle hMeshBuffer;
  const char* szMeshGuid = "";

  switch (m_iHandleType)
  {
    case plEngineGizmoHandleType::Arrow:
    {
      hMeshBuffer = CreateMeshBufferArrow();
      szMeshGuid = "{9D02CF27-7A15-44EA-A372-C417AF2A8E9B}";
    }
    break;
    case plEngineGizmoHandleType::Rect:
    {
      hMeshBuffer = CreateMeshBufferRect();
      szMeshGuid = "{3DF4DDDA-F598-4A37-9691-D4C3677905A8}";
    }
    break;
    case plEngineGizmoHandleType::LineRect:
    {
      hMeshBuffer = CreateMeshBufferLineRect();
      szMeshGuid = "{96129543-897C-4DEE-922D-931BC91C5725}";
    }
    break;
    case plEngineGizmoHandleType::Ring:
    {
      hMeshBuffer = CreateMeshBufferRing();
      szMeshGuid = "{629AD0C6-C81B-4850-A5BC-41494DC0BF95}";
    }
    break;
    case plEngineGizmoHandleType::Box:
    {
      hMeshBuffer = CreateMeshBufferBox();
      szMeshGuid = "{13A59253-4A98-4638-8B94-5AA370E929A7}";
    }
    break;
    case plEngineGizmoHandleType::Piston:
    {
      hMeshBuffer = CreateMeshBufferPiston();
      szMeshGuid = "{44A4FE37-6AE3-44C1-897D-E8B95AE53EF6}";
    }
    break;
    case plEngineGizmoHandleType::HalfPiston:
    {
      hMeshBuffer = CreateMeshBufferHalfPiston();
      szMeshGuid = "{64A45DD0-D7F9-4D1D-9F68-782FA3274200}";
    }
    break;
    case plEngineGizmoHandleType::Sphere:
    {
      hMeshBuffer = CreateMeshBufferSphere();
      szMeshGuid = "{FC322E80-5EB0-452F-9D8E-9E65FCFDA652}";
    }
    break;
    case plEngineGizmoHandleType::CylinderZ:
    {
      hMeshBuffer = CreateMeshBufferCylinderZ();
      szMeshGuid = "{893384EA-2F43-4265-AF75-662E2C81C167}";
    }
    break;
    case plEngineGizmoHandleType::HalfSphereZ:
    {
      hMeshBuffer = CreateMeshBufferHalfSphereZ();
      szMeshGuid = "{0FC9B680-7B6B-40B6-97BD-CBFFA47F0EFF}";
    }
    break;
    case plEngineGizmoHandleType::BoxCorners:
    {
      hMeshBuffer = CreateMeshBufferBoxCorners();
      szMeshGuid = "{89CCC389-11D5-43F4-9C18-C634EE3154B9}";
    }
    break;
    case plEngineGizmoHandleType::BoxEdges:
    {
      hMeshBuffer = CreateMeshBufferBoxEdges();
      szMeshGuid = "{21508253-2E74-44CE-9399-523214BE7C3D}";
    }
    break;
    case plEngineGizmoHandleType::BoxFaces:
    {
      hMeshBuffer = CreateMeshBufferBoxFaces();
      szMeshGuid = "{FD1A3C29-F8F0-42B0-BBB0-D0A2B28A65A0}";
    }
    break;
    case plEngineGizmoHandleType::LineBox:
    {
      hMeshBuffer = CreateMeshBufferLineBox();
      szMeshGuid = "{4B136D72-BF43-4C4B-96D7-51C5028A7006}";
    }
    break;
    case plEngineGizmoHandleType::Cone:
    {
      hMeshBuffer = CreateMeshBufferCone();
      szMeshGuid = "{9A48962D-127A-445C-899A-A054D6AD8A9A}";
    }
    break;
    case plEngineGizmoHandleType::Frustum:
    {
      szMeshGuid = "{22EC5D48-E8BE-410B-8EAD-51B7775BA058}";
      hMeshBuffer = CreateMeshBufferFrustum();
    }
    break;
    case plEngineGizmoHandleType::FromFile:
    {
      szMeshGuid = m_sGizmoHandleMesh;
      hMeshBuffer = CreateMeshBufferFromFile(m_sGizmoHandleMesh);
    }
    break;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  plStringBuilder sName;
  sName.SetFormat("Gizmo{0}", m_iHandleType);

  plGameObjectDesc god;
  god.m_LocalPosition = m_Transformation.m_vPosition;
  god.m_LocalRotation = m_Transformation.m_qRotation;
  god.m_LocalScaling = m_Transformation.m_vScale;
  god.m_sName.Assign(sName.GetData());
  god.m_bDynamic = true;

  plGameObject* pObject;
  m_hGameObject = pWorld->CreateObject(god, pObject);

  if (!m_bShowInOrtho)
  {
    const plTag& tagNoOrtho = plTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    pObject->SetTag(tagNoOrtho);
  }

  {
    const plTag& tagEditor = plTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    pObject->SetTag(tagEditor);
  }

  plGizmoComponent::CreateComponent(pObject, m_pGizmoComponent);


  plMeshResourceHandle hMesh;

  if (m_bVisualizer)
  {
    hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/Visualizer.plMaterial");
  }
  else if (m_bConstantSize)
  {
    if (m_bFaceCamera)
    {
      hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandleConstantSizeCamFacing.plMaterial");
    }
    else
    {
      hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandleConstantSize.plMaterial");
    }
  }
  else
  {
    hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandle.plMaterial");
  }

  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->m_bIsPickable = m_bIsPickable;
  m_pGizmoComponent->SetMesh(hMesh);

  m_pGizmoComponent->SetUniqueID(uiNextComponentPickingID);

  return true;
}

void plEngineGizmoHandle::UpdateForEngine(plWorld* pWorld)
{
  if (m_hGameObject.IsInvalidated())
    return;

  plGameObject* pObject;
  if (!pWorld->TryGetObject(m_hGameObject, pObject))
    return;

  pObject->SetLocalPosition(m_Transformation.m_vPosition);
  pObject->SetLocalRotation(m_Transformation.m_qRotation);
  pObject->SetLocalScaling(m_Transformation.m_vScale);

  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->SetActiveFlag(m_bVisible);
}

void plEngineGizmoHandle::SetColor(const plColor& col)
{
  m_Color = col;
  SetModified();
}
