#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>

struct plGALTextureCreationDescription;
class plStreamWriter;

/// \brief Passed to plRenderPipelinePass::InitRenderPipelinePass to inform about
/// existing connections on each input / output pin index.
struct plRenderPipelinePassConnection
{
  plRenderPipelinePassConnection() { m_pOutput = nullptr; }

  plGALTextureCreationDescription m_Desc;
  plGALTextureHandle m_TextureHandle;
  const plRenderPipelineNodePin* m_pOutput;                  ///< The output pin that this connection spawns from.
  plHybridArray<const plRenderPipelineNodePin*, 4> m_Inputs; ///< The various input pins this connection is connected to.
};

class PL_RENDERERCORE_DLL plRenderPipelinePass : public plRenderPipelineNode
{
  PL_ADD_DYNAMIC_REFLECTION(plRenderPipelinePass, plRenderPipelineNode);
  PL_DISALLOW_COPY_AND_ASSIGN(plRenderPipelinePass);

public:
  plRenderPipelinePass(const char* szName, bool bIsStereoAware = false);
  ~plRenderPipelinePass();

  /// \brief Sets the name of the pass.
  void SetName(const char* szName);

  /// \brief returns the name of the pass.
  const char* GetName() const;

  /// \brief True if the render pipeline pass can handle stereo cameras correctly.
  bool IsStereoAware() const { return m_bIsStereoAware; }

  /// \brief For a given input pin configuration, provide the output configuration of this node.
  /// Outputs is already resized to the number of output pins.
  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) = 0;

  /// \brief After GetRenderTargetDescriptions was called successfully for each pass, this function is called
  /// with the inputs and outputs for review. Disconnected pins have a nullptr value in the passed in arrays.
  /// This is the time to create additional resources that are not covered by the pins automatically, e.g. a picking texture or eye
  /// adaptation buffer.
  virtual void InitRenderPipelinePass(const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs);

  /// \brief Render into outputs. Both inputs and outputs are passed in with actual texture handles.
  /// Disconnected pins have a nullptr value in the passed in arrays. You can now create views and render target setups on the fly and
  /// fill the output targets with data.
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) = 0;

  virtual void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs);

  /// \brief Allows for the pass to write data back using plView::SetRenderPassReadBackProperty. E.g. picking results etc.
  virtual void ReadBackProperties(plView* pView);

  virtual plResult Serialize(plStreamWriter& inout_stream) const;
  virtual plResult Deserialize(plStreamReader& inout_stream);

  void RenderDataWithCategory(const plRenderViewContext& renderViewContext, plRenderData::Category category, plRenderDataBatch::Filter filter = plRenderDataBatch::Filter());

  PL_ALWAYS_INLINE plRenderPipeline* GetPipeline() { return m_pPipeline; }
  PL_ALWAYS_INLINE const plRenderPipeline* GetPipeline() const { return m_pPipeline; }

private:
  friend class plRenderPipeline;

  bool m_bActive = true;

  const bool m_bIsStereoAware;
  plHashedString m_sName;

  plRenderPipeline* m_pPipeline = nullptr;
};
