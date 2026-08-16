#include "cpcMesh.h"

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

void cpcMesh::Validate()
{
  for (i64 triID = 0; triID < m_triangles.Size(); ++triID)
  {
    cpcMeshTriangle *tri = m_triangles.AtPtr(triID);
    for (i64 v = 0; v < 3; ++v)
    {
      cpcMeshVertex *vert = cpcMeshTriangleVertex(tri, v);
      if (vert->color >= m_colors.Size()) vert->color = -1;
      if (vert->normal >= m_normals.Size()) vert->normal = -1;
      if (vert->position >= m_positions.Size()) vert->position = -1;
      if (vert->texCoords >= m_texCoords.Size()) vert->texCoords = -1;
    }
    if (tri->material >= m_materials.Size())
      tri->material = -1;
  }
}

void cpcMesh::DefaultMissingAttributes()
{
  i64 missingMatID = -1;
  i64 missingPosID = -1;
  i64 missingTexUVID = -1;
  i64 defaultColorID = -1;

  for (i64 faceID = 0; faceID < m_triangles.Size(); ++faceID)
  {
    cpcMeshTriangle *face = m_triangles.AtPtr(faceID);
    for (i64 i = 0; i < 3; ++i)
    {
      cpcMeshVertex *vert = cpcMeshTriangleVertex(face, i);
      if (vert->position == -1)
      {
        if (missingPosID == -1)
        {
          cpcVec3 pos = cpcVec3Make(0, 0, 0);
          m_positions.PushBack(pos);
          missingPosID = m_positions.Size() - 1;
        }
        vert->position = missingPosID;
      }
    }
  }

  for (i64 faceID = 0; faceID < m_triangles.Size(); ++faceID)
  {
    cpcMeshTriangle *face = m_triangles.AtPtr(faceID);
    for (i64 i = 0; i < 3; ++i)
    {
      cpcMeshVertex *vert = cpcMeshTriangleVertex(face, i);
      if (vert->color == -1)
      {
        if (defaultColorID == -1)
        {
          m_colors.PushBack(cpcVec4One());
          defaultColorID = m_colors.Size() - 1;
        }
        vert->color = defaultColorID;
      }
      if (vert->normal == -1)
      {
        cpcVec3 *p0 = m_positions.AtPtr(face->v0.position);
        cpcVec3 *p1 = m_positions.AtPtr(face->v1.position);
        cpcVec3 *p2 = m_positions.AtPtr(face->v2.position);
        cpcVec3 edgeA = cpcVec3Make(p1->x - p0->x, p1->y - p0->y, p1->z - p0->z);
        cpcVec3 edgeB = cpcVec3Make(p2->x - p0->x, p2->y - p0->y, p2->z - p0->z);
        cpcVec3 normal = cpcNormalize(cpcCross(edgeA, edgeB));
        m_normals.PushBack(normal);
        vert->normal = m_normals.Size() - 1;
      }
      if (vert->texCoords == -1)
      {
        if (missingTexUVID == -1)
        {
          m_texCoords.PushBack(cpcVec2Make(0, 0));
          missingTexUVID = m_texCoords.Size() - 1;
        }
        vert->texCoords = missingTexUVID;
      }
    }
    if (face->material == -1)
    {
      if (missingMatID == -1)
      {
        cpcMeshMaterial material = cpcMeshMaterialMake();
        m_materials.PushBack(material);
        missingMatID = m_materials.Size() - 1;
      }
      face->material = missingMatID;
    }
  }
}

void cpcMesh::InitCube(float size, cpcVec3 color)
{
  m_immediateKind = CPC_MESH_IMMEDIATE_CUBE;
  m_immediateSize = size;
  m_immediateGridHalfExtent = 0;
  m_immediateColor = color;
}

void cpcMesh::InitGrid(int halfExtent, cpcVec3 color)
{
  m_immediateKind = CPC_MESH_IMMEDIATE_GRID;
  m_immediateSize = 0.0f;
  m_immediateGridHalfExtent = halfExtent;
  m_immediateColor = color;
}

void cpcMesh::DrawImmediate()
{
  (void)m_immediateKind;
}
