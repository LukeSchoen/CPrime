#include "cpcMesh.h"
#include "cpcGL.h"

enum
{
  CPC_MESH_IMMEDIATE_NONE = 0,
  CPC_MESH_IMMEDIATE_CUBE = 1,
  CPC_MESH_IMMEDIATE_GRID = 2
};

cpcMesh::cpcMesh()
{
  m_immediateKind = CPC_MESH_IMMEDIATE_NONE;
  m_immediateSize = 0.0f;
  m_immediateGridHalfExtent = 0;
  m_immediateColor = cpcVec3Make(1, 1, 1);
}

void cpcMesh::DrawImmediate()
{
  (void)m_immediateKind;
}
