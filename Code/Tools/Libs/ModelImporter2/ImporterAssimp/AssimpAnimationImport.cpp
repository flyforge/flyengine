#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <assimp/anim.h>
#include <assimp/scene.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_animation_utils.h>

namespace plModelImporter2
{
  PL_FORCE_INLINE void ai2ozz(const aiVector3D& in, ozz::math::Float3& ref_out)
  {
    ref_out.x = (float)in.x;
    ref_out.y = (float)in.y;
    ref_out.z = (float)in.z;
  }

  PL_FORCE_INLINE void ai2ozz(const aiQuaternion& in, ozz::math::Quaternion& ref_out)
  {
    ref_out.x = (float)in.x;
    ref_out.y = (float)in.y;
    ref_out.z = (float)in.z;
    ref_out.w = (float)in.w;
  }

  PL_FORCE_INLINE void ozz2pl(const ozz::math::Float3& in, plVec3& ref_vOut)
  {
    ref_vOut.x = (float)in.x;
    ref_vOut.y = (float)in.y;
    ref_vOut.z = (float)in.z;
  }

  PL_FORCE_INLINE void ozz2pl(const ozz::math::Quaternion& in, plQuat& ref_qOut)
  {
    ref_qOut.x = (float)in.x;
    ref_qOut.y = (float)in.y;
    ref_qOut.z = (float)in.z;
    ref_qOut.w = (float)in.w;
  }

  plResult ImporterAssimp::ImportAnimations()
  {
    auto* pAnimOut = m_Options.m_pAnimationOutput;

    if (pAnimOut == nullptr)
      return PL_SUCCESS;

    if (m_pScene->mNumAnimations == 0)
      return PL_FAILURE;

    pAnimOut->m_bAdditive = m_Options.m_bAdditiveAnimation;

    m_OutputAnimationNames.SetCount(m_pScene->mNumAnimations);
    for (plUInt32 animIdx = 0; animIdx < m_pScene->mNumAnimations; ++animIdx)
    {
      m_OutputAnimationNames[animIdx] = m_pScene->mAnimations[animIdx]->mName.C_Str();
    }

    if (m_Options.m_sAnimationToImport.IsEmpty())
      m_Options.m_sAnimationToImport = m_pScene->mAnimations[0]->mName.C_Str();

    plHashedString hs;

    ozz::animation::offline::RawAnimation orgRawAnim, sampledRawAnim;
    ozz::animation::offline::RawAnimation* pFinalRawAnim = &orgRawAnim;

    for (plUInt32 animIdx = 0; animIdx < m_pScene->mNumAnimations; ++animIdx)
    {
      const aiAnimation* pAnim = m_pScene->mAnimations[animIdx];

      if (m_Options.m_sAnimationToImport != pAnim->mName.C_Str())
        continue;

      if (plMath::IsZero(pAnim->mDuration, plMath::DefaultEpsilon<double>()))
        return PL_FAILURE;

      const plUInt32 uiMaxKeyframes = (plUInt32)plMath::Max(1.0, pAnim->mDuration);

      if (m_Options.m_uiNumAnimKeyframes == 0)
      {
        m_Options.m_uiNumAnimKeyframes = uiMaxKeyframes;
      }

      // Usually assimp should give us the correct number of ticks per second here,
      // e.g. for GLTF files it should be 1000.
      // However, sometimes this 'breaks' (usually someone changes assimp).
      // If that happens again in the future, we may need to add a custom property to override the value.
      const double fOneDivTicksPerSec = 1.0 / pAnim->mTicksPerSecond;

      const plUInt32 uiNumChannels = pAnim->mNumChannels;

      orgRawAnim.tracks.resize(uiNumChannels);
      orgRawAnim.duration = (float)(pAnim->mDuration * fOneDivTicksPerSec);

      for (plUInt32 channelIdx = 0; channelIdx < uiNumChannels; ++channelIdx)
      {
        const auto pChannel = pAnim->mChannels[channelIdx];

        orgRawAnim.tracks[channelIdx].translations.resize(pChannel->mNumPositionKeys);
        orgRawAnim.tracks[channelIdx].rotations.resize(pChannel->mNumRotationKeys);
        orgRawAnim.tracks[channelIdx].scales.resize(pChannel->mNumScalingKeys);

        for (plUInt32 i = 0; i < pChannel->mNumPositionKeys; ++i)
        {
          orgRawAnim.tracks[channelIdx].translations[i].time = (float)(pChannel->mPositionKeys[i].mTime * fOneDivTicksPerSec);
          ai2ozz(pChannel->mPositionKeys[i].mValue, orgRawAnim.tracks[channelIdx].translations[i].value);
        }
        for (plUInt32 i = 0; i < pChannel->mNumRotationKeys; ++i)
        {
          orgRawAnim.tracks[channelIdx].rotations[i].time = (float)(pChannel->mRotationKeys[i].mTime * fOneDivTicksPerSec);
          ai2ozz(pChannel->mRotationKeys[i].mValue, orgRawAnim.tracks[channelIdx].rotations[i].value);
        }
        for (plUInt32 i = 0; i < pChannel->mNumScalingKeys; ++i)
        {
          orgRawAnim.tracks[channelIdx].scales[i].time = (float)(pChannel->mScalingKeys[i].mTime * fOneDivTicksPerSec);
          ai2ozz(pChannel->mScalingKeys[i].mValue, orgRawAnim.tracks[channelIdx].scales[i].value);
        }

        if (m_Options.m_bAdditiveAnimation)
        {
          auto refPos = orgRawAnim.tracks[channelIdx].translations[0].value;
          auto refRot = Conjugate(orgRawAnim.tracks[channelIdx].rotations[0].value);
          auto refScale = orgRawAnim.tracks[channelIdx].scales[0].value;
          refScale.x = 1.0f / refScale.x;
          refScale.y = 1.0f / refScale.y;
          refScale.z = 1.0f / refScale.z;

          for (plUInt32 i = 0; i < pChannel->mNumPositionKeys; ++i)
          {
            auto& val = orgRawAnim.tracks[channelIdx].translations[i].value;
            val.x -= refPos.x;
            val.y -= refPos.y;
            val.z -= refPos.z;
          }

          for (plUInt32 i = 0; i < pChannel->mNumRotationKeys; ++i)
          {
            auto& val = orgRawAnim.tracks[channelIdx].rotations[i].value;
            val = refRot * val;
          }

          for (plUInt32 i = 0; i < pChannel->mNumScalingKeys; ++i)
          {
            auto& val = orgRawAnim.tracks[channelIdx].scales[i].value;
            val.x *= refScale.x;
            val.y *= refScale.y;
            val.z *= refScale.z;
          }
        }
      }

      if (m_Options.m_uiFirstAnimKeyframe > 0 || m_Options.m_uiNumAnimKeyframes < uiMaxKeyframes)
      {
        m_Options.m_uiFirstAnimKeyframe = plMath::Min(m_Options.m_uiFirstAnimKeyframe, uiMaxKeyframes - 1);
        const plUInt32 uiLastKeyframeExcl = plMath::Min(m_Options.m_uiFirstAnimKeyframe + m_Options.m_uiNumAnimKeyframes, uiMaxKeyframes);
        const plUInt32 uiNumKeyframes = uiLastKeyframeExcl - m_Options.m_uiFirstAnimKeyframe;

        const double fLowerTimestamp = m_Options.m_uiFirstAnimKeyframe * fOneDivTicksPerSec;
        const double fDuration = uiNumKeyframes * fOneDivTicksPerSec;

        sampledRawAnim.duration = (float)fDuration;
        sampledRawAnim.tracks.resize(uiNumChannels);

        ozz::math::Transform tmpTransform;

        for (plUInt32 channelIdx = 0; channelIdx < uiNumChannels; ++channelIdx)
        {
          auto& track = sampledRawAnim.tracks[channelIdx];

          track.translations.resize(uiNumKeyframes);
          track.rotations.resize(uiNumKeyframes);
          track.scales.resize(uiNumKeyframes);

          for (plUInt32 kf = 0; kf < uiNumKeyframes; ++kf)
          {
            const float fCurTime = (float)fOneDivTicksPerSec * kf;
            ;
            ozz::animation::offline::SampleTrack(orgRawAnim.tracks[channelIdx], (float)(fLowerTimestamp + fOneDivTicksPerSec * kf), &tmpTransform);

            track.translations[kf].time = fCurTime;
            track.translations[kf].value = tmpTransform.translation;

            track.rotations[kf].time = fCurTime;
            track.rotations[kf].value = tmpTransform.rotation;

            track.scales[kf].time = fCurTime;
            track.scales[kf].value = tmpTransform.scale;
          }
        }

        pFinalRawAnim = &sampledRawAnim;
      }

      // TODO: optimize the animation

      for (plUInt32 channelIdx = 0; channelIdx < uiNumChannels; ++channelIdx)
      {
        const auto& channel = pAnim->mChannels[channelIdx];
        hs.Assign(channel->mNodeName.C_Str());

        pAnimOut->CreateJoint(hs, (plUInt16)pFinalRawAnim->tracks[channelIdx].translations.size(), (plUInt16)pFinalRawAnim->tracks[channelIdx].rotations.size(), (plUInt16)pFinalRawAnim->tracks[channelIdx].scales.size());
      }

      pAnimOut->AllocateJointTransforms();

      double fMaxTimestamp = 1.0 / 60.0; // minimum duration

      for (plUInt32 channelIdx = 0; channelIdx < uiNumChannels; ++channelIdx)
      {
        const char* szChannelName = pAnim->mChannels[channelIdx]->mNodeName.C_Str();
        const auto& track = pFinalRawAnim->tracks[channelIdx];

        const auto* pJointInfo = pAnimOut->GetJointInfo(plTempHashedString(szChannelName));

        // positions
        {
          auto keys = pAnimOut->GetPositionKeyframes(*pJointInfo);

          for (plUInt32 kf = 0; kf < keys.GetCount(); ++kf)
          {
            keys[kf].m_fTimeInSec = track.translations[kf].time;
            ozz2pl(track.translations[kf].value, keys[kf].m_Value);
          }

          fMaxTimestamp = plMath::Max<double>(fMaxTimestamp, keys[keys.GetCount() - 1].m_fTimeInSec);
        }

        // rotations
        {
          auto keys = pAnimOut->GetRotationKeyframes(*pJointInfo);

          for (plUInt32 kf = 0; kf < keys.GetCount(); ++kf)
          {
            keys[kf].m_fTimeInSec = track.rotations[kf].time;
            ozz2pl(track.rotations[kf].value, keys[kf].m_Value);
          }

          fMaxTimestamp = plMath::Max<double>(fMaxTimestamp, keys[keys.GetCount() - 1].m_fTimeInSec);
        }

        // scales
        {
          auto keys = pAnimOut->GetScaleKeyframes(*pJointInfo);

          for (plUInt32 kf = 0; kf < keys.GetCount(); ++kf)
          {
            keys[kf].m_fTimeInSec = track.scales[kf].time;
            ozz2pl(track.scales[kf].value, keys[kf].m_Value);
          }

          fMaxTimestamp = plMath::Max<double>(fMaxTimestamp, keys[keys.GetCount() - 1].m_fTimeInSec);
        }
      }

      pAnimOut->SetDuration(plTime::MakeFromSeconds(fMaxTimestamp));

      return PL_SUCCESS;
    }

    return PL_FAILURE;
  }
} // namespace plModelImporter2
