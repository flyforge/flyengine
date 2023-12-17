#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>


class PLASMA_CORE_DLL plECSComponent : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plECSComponent, plReflectedClass);

public:

  plECSComponent() = default;
  virtual ~plECSComponent() = default;

  /// \brief Override this to save the current state of the component to the given stream.
  virtual void SerializeComponent(plWorldWriter& inout_stream) const = 0;

  /// \brief Override this to load the current state of the component from the given stream.
  ///
  /// The active state will be automatically serialized. The 'initialized' state is not serialized, all components
  /// will be initialized after creation, even if they were already in an initialized state when they were serialized.
  virtual void DeserializeComponent(plWorldReader& inout_stream) = 0;

};
