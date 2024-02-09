#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

PL_IMPLEMENT_SINGLETON(plAudioSystemAllocator);
PL_IMPLEMENT_SINGLETON(plAudioMiddlewareAllocator);

plAudioSystemAllocator::plAudioSystemAllocator()
  : plAlignedHeapAllocator("AudioSystemAllocator")
  , m_SingletonRegistrar(this)
{
}

plAudioMiddlewareAllocator::plAudioMiddlewareAllocator(plAudioSystemAllocator* pParentAllocator)
  : plAlignedHeapAllocator("AudioMiddlewareAllocator", pParentAllocator)
  , m_SingletonRegistrar(this)
{
}

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioSystemAllocator);
