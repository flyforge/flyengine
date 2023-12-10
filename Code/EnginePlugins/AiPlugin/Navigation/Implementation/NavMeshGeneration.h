#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <Foundation/Threading/TaskSystem.h>

class plPhysicsWorldModuleInterface;

class plNavMeshSectorGenerationTask : public plTask
{
public:
  plAiNavMesh::SectorID m_SectorID = plInvalidIndex;
  plAiNavMesh* m_pWorldNavMesh = nullptr;
  const plPhysicsWorldModuleInterface* m_pPhysics = nullptr;

protected:
  virtual void Execute() override;
};
