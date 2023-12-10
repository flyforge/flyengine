#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

PLASMA_IMPLEMENT_SINGLETON(plAudioSystemAllocator);
PLASMA_IMPLEMENT_SINGLETON(plAudioMiddlewareAllocator);

plAudioSystemAllocator::plAudioSystemAllocator()
  : plAllocator("AudioSystemAllocator")
  , m_SingletonRegistrar(this)
{
}

plAudioMiddlewareAllocator::plAudioMiddlewareAllocator(plAudioSystemAllocator* pParentAllocator)
  : plAllocator("AudioMiddlewareAllocator", pParentAllocator)
  , m_SingletonRegistrar(this)
{
}

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioSystemAllocator);
