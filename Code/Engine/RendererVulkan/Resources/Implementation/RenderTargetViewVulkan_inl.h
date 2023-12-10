

PLASMA_ALWAYS_INLINE vk::ImageView plGALRenderTargetViewVulkan::GetImageView() const
{
  return m_imageView;
}

PLASMA_ALWAYS_INLINE bool plGALRenderTargetViewVulkan::IsFullRange() const
{
  return m_bfullRange;
}

PLASMA_ALWAYS_INLINE vk::ImageSubresourceRange plGALRenderTargetViewVulkan::GetRange() const
{
  return m_range;
}
