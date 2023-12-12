#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>

class plDocumentNodeManager;

class plVisualShaderCodeGenerator
{
public:
  plVisualShaderCodeGenerator();

  plStatus GenerateVisualShader(const plDocumentNodeManager* pNodeMaanger, plStringBuilder& out_sCheckPerms);

  const char* GetFinalShaderCode() const { return m_sFinalShaderCode; }

  void DetermineConfigFileDependencies(const plDocumentNodeManager* pNodeManager, plSet<plString>& out_cfgFiles);

private:
  struct NodeState
  {
    NodeState()
    {
      m_uiNodeId = 0;
      m_bCodeGenerated = false;
      m_bInProgress = false;
    }

    plUInt16 m_uiNodeId;
    bool m_bCodeGenerated;
    bool m_bInProgress;
  };

  struct OutputPinState
  {
    OutputPinState() { m_bCodeGenerated = false; }

    bool m_bCodeGenerated;
    plString m_sCodeAtPin;
  };


  plStatus GatherAllNodes(const plDocumentObject* pRootObj);
  plUInt16 DeterminePinId(const plDocumentObject* pOwner, const plPin& pin) const;
  plStatus GenerateNode(const plDocumentObject* pNode);
  plStatus GenerateInputPinCode(plArrayPtr<const plUniquePtr<const plPin>> pins);
  plStatus CheckPropertyValues(const plDocumentObject* pNode, const plVisualShaderNodeDescriptor* pDesc);
  plStatus InsertPropertyValues(const plDocumentObject* pNode, const plVisualShaderNodeDescriptor* pDesc, plStringBuilder& sString);
  plStatus GenerateOutputPinCode(const plDocumentObject* pOwnerNode, const plPin& pinSource);

  plStatus ReplaceInputPinsByCode(const plDocumentObject* pOwnerNode, const plVisualShaderNodeDescriptor* pNodeDesc, plStringBuilder& sInlineCode, plStringBuilder& sCodeForPlacingDefines);
  void SetPinDefines(const plDocumentObject* pOwnerNode, plStringBuilder& sInlineCode);
  static void AppendStringIfUnique(plStringBuilder& inout_String, const char* szAppend);

  const plDocumentObject* m_pMainNode;
  const plVisualShaderTypeRegistry* m_pTypeRegistry;
  const plDocumentNodeManager* m_pNodeManager;
  const plRTTI* m_pNodeBaseRtti;
  plMap<const plDocumentObject*, NodeState> m_Nodes;
  plMap<const plPin*, OutputPinState> m_OutputPins;
  plMap<plInt8, plSet<plString>> m_UsedUniqueValues;

  plStringBuilder m_sShaderPixelDefines;
  plStringBuilder m_sShaderPixelIncludes;
  plStringBuilder m_sShaderPixelConstants;
  plStringBuilder m_sShaderPixelSamplers;
  plStringBuilder m_sShaderPixelBody;
  plStringBuilder m_sShaderVertexDefines;
  plStringBuilder m_sShaderVertex;
  plStringBuilder m_sShaderGeometryDefines;
  plStringBuilder m_sShaderGeometry;
  plStringBuilder m_sShaderMaterialParam;
  plStringBuilder m_sShaderMaterialCB;
  plStringBuilder m_sShaderRenderConfig;
  plStringBuilder m_sShaderRenderState;
  plStringBuilder m_sShaderPermutations;
  plStringBuilder m_sFinalShaderCode;
};
