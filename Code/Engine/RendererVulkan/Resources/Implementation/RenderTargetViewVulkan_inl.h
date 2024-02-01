

PL_ALWAYS_INLINE vk::ImageView plGALRenderTargetViewVulkan::GetImageView() const
{
  return m_imageView;
}

PL_ALWAYS_INLINE bool plGALRenderTargetViewVulkan::IsFullRange() const
{
  return m_bfullRange;
}

PL_ALWAYS_INLINE vk::ImageSubresourceRange plGALRenderTargetViewVulkan::GetRange() const
{
  return m_range;
}
