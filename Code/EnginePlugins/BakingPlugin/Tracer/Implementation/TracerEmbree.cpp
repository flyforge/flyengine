#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

#include <embree3/rtcore.h>

namespace
{
  static RTCDevice s_rtcDevice;
  static plHashTable<plHashedString, RTCScene, plHashHelper<plHashedString>, plStaticsAllocatorWrapper> s_rtcMeshCache;

  const char* rtcErrorCodeToString[] = {
    "RTC_NO_ERROR",
    "RTC_UNKNOWN_ERROR",
    "RTC_INVALID_ARGUMENT",
    "RTC_INVALID_OPERATION",
    "RTC_OUT_OF_MEMORY",
    "RTC_UNSUPPORTED_CPU",
    "RTC_CANCELLED"};

  const char* GetStringFromRTCErrorCode(RTCError code)
  {
    return (code >= 0 && code < PL_ARRAY_SIZE(rtcErrorCodeToString)) ? rtcErrorCodeToString[code] : "RTC invalid error code";
  }

  static void ErrorCallback(void* userPtr, RTCError code, const char* str)
  {
    plLog::Error("Embree: {}: {}", GetStringFromRTCErrorCode(code), str);
  }

  static plResult InitDevice()
  {
    if (s_rtcDevice == nullptr)
    {
      if (s_rtcDevice = rtcNewDevice("threads=1"))
      {
        plLog::Info("Created new Embree Device (Version {})", RTC_VERSION_STRING);

        rtcSetDeviceErrorFunction(s_rtcDevice, &ErrorCallback, nullptr);

        bool bRay4Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY4_SUPPORTED);
        bool bRay8Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY8_SUPPORTED);
        bool bRay16Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY16_SUPPORTED);
        bool bRayStreamSupported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_RAY_STREAM_SUPPORTED);

        plLog::Info("Supported ray packets: Ray4:{}, Ray8:{}, Ray16:{}, RayStream:{}", bRay4Supported, bRay8Supported, bRay16Supported, bRayStreamSupported);
      }
      else
      {
        plLog::Error("Failed to create Embree Device. Error: {}", GetStringFromRTCErrorCode(rtcGetDeviceError(nullptr)));
        return PL_FAILURE;
      }
    }

    return PL_SUCCESS;
  }

  static void DeinitDevice()
  {
    for (auto it : s_rtcMeshCache)
    {
      rtcReleaseScene(it.Value());
    }
    s_rtcMeshCache.Clear();

    rtcReleaseDevice(s_rtcDevice);
    s_rtcDevice = nullptr;
  }

  static RTCScene GetOrCreateMesh(const plCpuMeshResourceHandle& hMeshResource)
  {
    plHashedString sResourceId;
    sResourceId.Assign(hMeshResource.GetResourceID());

    RTCScene scene = nullptr;
    if (s_rtcMeshCache.TryGetValue(sResourceId, scene))
    {
      return scene;
    }

    plResourceLock<plCpuMeshResource> pCpuMesh(hMeshResource, plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != plResourceAcquireResult::Final)
    {
      plLog::Warning("Failed to retrieve CPU mesh '{}'", sResourceId);
      return nullptr;
    }

    RTCGeometry triangleMesh = rtcNewGeometry(s_rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
    {
      const auto& mbDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

      const plVec3* pPositions = nullptr;
      const plUInt8* pNormals = nullptr;
      plGALResourceFormat::Enum normalFormat = plGALResourceFormat::Invalid;
      plUInt32 uiElementStride = 0;
      if (plMeshBufferUtils::GetPositionAndNormalStream(mbDesc, pPositions, pNormals, normalFormat, uiElementStride).Failed())
      {
        return nullptr;
      }

      plVec3* rtcPositions = static_cast<plVec3*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(plVec3), mbDesc.GetVertexCount()));

      rtcSetGeometryVertexAttributeCount(triangleMesh, 1);
      plVec3* rtcNormals = static_cast<plVec3*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, sizeof(plVec3), mbDesc.GetVertexCount()));

      // write out all vertices
      plVec3 vNormal;
      for (plUInt32 i = 0; i < mbDesc.GetVertexCount(); ++i)
      {
        plMeshBufferUtils::DecodeNormal(plMakeArrayPtr(pNormals, sizeof(plVec3)), normalFormat, vNormal).IgnoreResult();

        rtcPositions[i] = *pPositions;
        rtcNormals[i] = vNormal;

        pPositions = plMemoryUtils::AddByteOffset(pPositions, uiElementStride);
        pNormals = plMemoryUtils::AddByteOffset(pNormals, uiElementStride);
      }

      plVec3U32* rtcIndices = static_cast<plVec3U32*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(plVec3U32), mbDesc.GetPrimitiveCount()));

      bool flip = false;
      if (mbDesc.Uses32BitIndices())
      {
        const plUInt32* pTypedIndices = reinterpret_cast<const plUInt32*>(mbDesc.GetIndexBufferData().GetPtr());

        for (plUInt32 p = 0; p < mbDesc.GetPrimitiveCount(); ++p)
        {
          rtcIndices[p].x = pTypedIndices[p * 3 + (flip ? 2 : 0)];
          rtcIndices[p].y = pTypedIndices[p * 3 + 1];
          rtcIndices[p].z = pTypedIndices[p * 3 + (flip ? 0 : 2)];
        }
      }
      else
      {
        const plUInt16* pTypedIndices = reinterpret_cast<const plUInt16*>(mbDesc.GetIndexBufferData().GetPtr());

        for (plUInt32 p = 0; p < mbDesc.GetPrimitiveCount(); ++p)
        {
          rtcIndices[p].x = pTypedIndices[p * 3 + (flip ? 2 : 0)];
          rtcIndices[p].y = pTypedIndices[p * 3 + 1];
          rtcIndices[p].z = pTypedIndices[p * 3 + (flip ? 0 : 2)];
        }
      }

      rtcCommitGeometry(triangleMesh);
    }

    scene = rtcNewScene(s_rtcDevice);
    {
      PL_VERIFY(rtcAttachGeometry(scene, triangleMesh) == 0, "Geometry id must be 0");
      rtcReleaseGeometry(triangleMesh);

      rtcCommitScene(scene);
    }

    s_rtcMeshCache.Insert(sResourceId, scene);
    return scene;
  }

} // namespace

struct plTracerEmbree::Data
{
  ~Data()
  {
    ClearScene();
  }

  void ClearScene()
  {
    m_rtcInstancedGeometry.Clear();

    if (m_rtcScene != nullptr)
    {
      rtcReleaseScene(m_rtcScene);
      m_rtcScene = nullptr;
    }
  }

  RTCScene m_rtcScene = nullptr;

  struct InstancedGeometry
  {
    RTCGeometry m_mesh;
    plSimdVec4f m_normalTransform0;
    plSimdVec4f m_normalTransform1;
    plSimdVec4f m_normalTransform2;
  };

  plDynamicArray<InstancedGeometry, plAlignedAllocatorWrapper> m_rtcInstancedGeometry;
};

plTracerEmbree::plTracerEmbree()
{
  m_pData = PL_DEFAULT_NEW(Data);
}

plTracerEmbree::~plTracerEmbree() = default;

plResult plTracerEmbree::BuildScene(const plBakingScene& scene)
{
  PL_SUCCEED_OR_RETURN(InitDevice());

  m_pData->ClearScene();
  m_pData->m_rtcScene = rtcNewScene(s_rtcDevice);

  for (auto& meshObject : scene.GetMeshObjects())
  {
    RTCScene mesh = GetOrCreateMesh(meshObject.m_hMeshResource);
    if (mesh == nullptr)
    {
      continue;
    }

    plMat4 transform = meshObject.m_GlobalTransform.GetAsMat4();

    RTCGeometry instance = rtcNewGeometry(s_rtcDevice, RTC_GEOMETRY_TYPE_INSTANCE);
    {
      rtcSetGeometryInstancedScene(instance, mesh);
      rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, &transform);

      rtcCommitGeometry(instance);
    }

    plUInt32 uiInstanceID = rtcAttachGeometry(m_pData->m_rtcScene, instance);
    rtcReleaseGeometry(instance);

    plMat3 normalTransform = transform.GetRotationalPart().GetInverse(0.0f).GetTranspose();

    PL_ASSERT_DEBUG(uiInstanceID == m_pData->m_rtcInstancedGeometry.GetCount(), "");
    auto& instancedGeometry = m_pData->m_rtcInstancedGeometry.ExpandAndGetRef();
    instancedGeometry.m_mesh = rtcGetGeometry(mesh, 0);
    instancedGeometry.m_normalTransform0 = plSimdConversion::ToVec3(normalTransform.GetColumn(0));
    instancedGeometry.m_normalTransform1 = plSimdConversion::ToVec3(normalTransform.GetColumn(1));
    instancedGeometry.m_normalTransform2 = plSimdConversion::ToVec3(normalTransform.GetColumn(2));
  }

  rtcCommitScene(m_pData->m_rtcScene);

  return PL_SUCCESS;
}

PL_DEFINE_AS_POD_TYPE(RTCRayHit);

void plTracerEmbree::TraceRays(plArrayPtr<const Ray> rays, plArrayPtr<Hit> hits)
{
  const plUInt32 uiNumRays = rays.GetCount();

  plHybridArray<RTCRayHit, 256, plAlignedAllocatorWrapper> rtcRayHits;
  rtcRayHits.SetCountUninitialized(uiNumRays);

  for (plUInt32 i = 0; i < uiNumRays; ++i)
  {
    auto& ray = rays[i];
    auto& rtcRayHit = rtcRayHits[i];

    rtcRayHit.ray.org_x = ray.m_vStartPos.x;
    rtcRayHit.ray.org_y = ray.m_vStartPos.y;
    rtcRayHit.ray.org_z = ray.m_vStartPos.z;
    rtcRayHit.ray.tnear = 0.0f;

    rtcRayHit.ray.dir_x = ray.m_vDir.x;
    rtcRayHit.ray.dir_y = ray.m_vDir.y;
    rtcRayHit.ray.dir_z = ray.m_vDir.z;
    rtcRayHit.ray.time = 0.0f;

    rtcRayHit.ray.tfar = ray.m_fDistance;
    rtcRayHit.ray.mask = 0;
    rtcRayHit.ray.id = i;
    rtcRayHit.ray.flags = 0;

    rtcRayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
  }

  RTCIntersectContext context;
  rtcInitIntersectContext(&context);

  rtcIntersect1M(m_pData->m_rtcScene, &context, rtcRayHits.GetData(), uiNumRays, sizeof(RTCRayHit));

  for (plUInt32 i = 0; i < uiNumRays; ++i)
  {
    auto& rtcRayHit = rtcRayHits[i];
    auto& ray = rays[i];
    auto& hit = hits[i];

    if (rtcRayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
      auto& instancedGeometry = m_pData->m_rtcInstancedGeometry[rtcRayHit.hit.instID[0]];

      plSimdVec4f objectSpaceNormal;
      rtcInterpolate0(instancedGeometry.m_mesh, rtcRayHit.hit.primID, rtcRayHit.hit.u, rtcRayHit.hit.v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, reinterpret_cast<float*>(&objectSpaceNormal), 3);

      plSimdVec4f worldSpaceNormal = instancedGeometry.m_normalTransform0 * objectSpaceNormal.x();
      worldSpaceNormal += instancedGeometry.m_normalTransform1 * objectSpaceNormal.y();
      worldSpaceNormal += instancedGeometry.m_normalTransform2 * objectSpaceNormal.z();

      hit.m_vNormal = plSimdConversion::ToVec3(worldSpaceNormal.GetNormalized<3>());
      hit.m_fDistance = rtcRayHit.ray.tfar;
      hit.m_vPosition = ray.m_vStartPos + ray.m_vDir * hit.m_fDistance;
    }
    else
    {
      hit.m_vPosition.SetZero();
      hit.m_vNormal.SetZero();
      hit.m_fDistance = -1.0f;
    }
  }
}
