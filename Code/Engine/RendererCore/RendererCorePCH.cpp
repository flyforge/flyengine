#include <RendererCore/RendererCorePCH.h>

PLASMA_STATICLINK_LIBRARY(RendererCore)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_BlackboardAnimNodes);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_BoneWeightsAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_CombinePosesAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_DebugAnimNodes);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_EventAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_LocalToModelPoseAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_LogicAnimNodes);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_MathAnimNodes);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_MixClips1DAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_MixClips2DAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_ModelPoseOutputAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_PlayClipAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_PlaySequenceAnimNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraph);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphPins);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_AnimPoseGenerator);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_AnimationClipResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_AnimationPose);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_EditableSkeleton);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_OzzUtils);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_Skeleton);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_SkeletonBuilder);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_SkeletonComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_SkeletonPoseComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_SkeletonResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakedProbesComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakedProbesVolumeComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakedProbesWorldModule);
  PLASMA_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakingInterface);
  PLASMA_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakingUtils);
  PLASMA_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_ProbeTreeSectorResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_AlwaysVisibleComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_BeamComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_CameraComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_FogComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_OccluderComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_RenderComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_RenderTargetActivatorComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_RopeRenderComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_SkyBoxComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_SpriteComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Components_Implementation_SpriteRenderer);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Debug_Implementation_DebugRenderer);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Debug_Implementation_DebugTextComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Debug_Implementation_Inconsolata);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Debug_Implementation_SimpleASCIIFont);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Decals_Implementation_DecalAtlasResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Decals_Implementation_DecalComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Decals_Implementation_DecalResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_GPUResourcePool_Implementation_GPUResourcePool);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_AmbientLightComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_BoxReflectionProbeComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ClusteredDataExtractor);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ClusteredDataProvider);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_DirectionalLightComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_LightComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_PointLightComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionPool);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionPoolData);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionProbeComponentBase);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionProbeData);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionProbeMapping);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionProbeUpdater);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ShadowPool);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SimplifiedDataExtractor);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SimplifiedDataProvider);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SkyLightComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SphereReflectionProbeComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SpotLightComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Material_Implementation_MaterialResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_CpuMeshResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_CustomMeshComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_InstancedMeshComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshBufferResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshBufferUtils);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshComponentBase);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshRenderer);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshResourceDescriptor);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_SkinnedMeshComponent);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_SkinnedMeshRenderer);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_ExtractedRenderData);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Extractor);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_FrameDataProvider);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_InstanceDataProvider);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_AOPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_AntialiasingPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_BloomPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_BlurPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_CopyTexturePass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_DepthOnlyPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_ForwardRenderPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_LSAOPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_MsaaResolvePass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_MsaaUpscalePass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_OpaqueForwardRenderPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SelectionHighlightPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SimpleRenderPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SkyRenderPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SourcePass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_StereoTestPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_TargetPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_TonemapPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderData);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipeline);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipelineNode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipelinePass);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipelineResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_SortingFunctions);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_View);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_ViewRenderMode);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Rasterizer_Implementation_RasterizerObject);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Rasterizer_Implementation_RasterizerView);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Rasterizer_Thirdparty_Occluder);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Rasterizer_Thirdparty_Rasterizer);
  PLASMA_STATICLINK_REFERENCE(RendererCore_RenderContext_Implementation_RenderContext);
  PLASMA_STATICLINK_REFERENCE(RendererCore_RenderWorld_Implementation_RenderWorld);
  PLASMA_STATICLINK_REFERENCE(RendererCore_ShaderCompiler_Implementation_PermutationGenerator);
  PLASMA_STATICLINK_REFERENCE(RendererCore_ShaderCompiler_Implementation_ShaderCompiler);
  PLASMA_STATICLINK_REFERENCE(RendererCore_ShaderCompiler_Implementation_ShaderManager);
  PLASMA_STATICLINK_REFERENCE(RendererCore_ShaderCompiler_Implementation_ShaderParser);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ConstantBufferStorage);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_Helper);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderPermutationBinary);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderPermutationResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderStageBinary);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderStateDescriptor);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Textures_Texture2DResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Textures_Texture3DResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Textures_TextureCubeResource);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Textures_TextureLoader);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Textures_TextureUtils);
  PLASMA_STATICLINK_REFERENCE(RendererCore_Utils_Implementation_WorldGeoExtractionUtil);
}