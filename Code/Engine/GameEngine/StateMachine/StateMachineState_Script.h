#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/StateMachine/StateMachine.h>

/// \brief A state machine state implementation that can be scripted using e.g. visual scripting.
class PLASMA_GAMEENGINE_DLL plStateMachineState_Script : public plStateMachineState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineState_Script, plStateMachineState);

public:
  plStateMachineState_Script(plStringView sName = plStringView());
  ~plStateMachineState_Script();

  virtual void OnEnter(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pFromState) const override;
  virtual void OnExit(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pToState) const override;
  virtual void Update(plStateMachineInstance& ref_instance, void* pInstanceData, plTime deltaTime) const override;

  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) override;

  void SetScriptClassFile(const char* szFile); // [ property ]
  const char* GetScriptClassFile() const;      // [ property ]

  // Exposed Parameters
  const plRangeView<const char*, plUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const plVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, plVariant& out_value) const;

private:
  plArrayMap<plHashedString, plVariant> m_Parameters;

  plString m_sScriptClassFile;
};
