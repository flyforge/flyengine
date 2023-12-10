#include <RendererVulkan/RendererVulkanPCH.h>

PLASMA_STATICLINK_LIBRARY(RendererVulkan)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Context_Implementation_ContextVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Device_Implementation_DeviceVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Device_Implementation_SwapChainVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_BufferVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_FenceVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_QueryVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_RenderTargetViewVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_ResourceViewVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_TextureVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_SharedTextureVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_UnorderedAccessViewVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Shader_Implementation_ShaderVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_Shader_Implementation_VertexDeclarationVulkan);
  PLASMA_STATICLINK_REFERENCE(RendererVulkan_State_Implementation_StateVulkan);
}
