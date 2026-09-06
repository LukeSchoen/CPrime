#include "clAssimp.h"
#include "clFile.h"
#include <stdio.h>
static bool close(double a,double b){return a-b<0.00001&&b-a<0.00001;}
int main(int argc,char**argv){
 if(argc!=2)return 1;
 clString folder=argv[1];clPath input=folder+"/triangle.obj";
 clMesh mesh=clAssimp::Import(input);
 if(mesh.m_triangles.Size()!=1||mesh.m_positions.Size()!=3||mesh.m_texCoords.Size()!=3)return 2;
 const auto& triangle=mesh.m_triangles[0];const auto& material=mesh.m_materials[triangle.material];
 if(material.name!="surface"||!close(material.diffuseColor.x,.25)||!close(material.diffuseColor.y,.5)||!close(material.diffuseColor.z,.75))return 3;
 if(!clFile::Exists(material.diffuseMapPath))return 4;
 if(!close(mesh.m_texCoords[0].x,.25)||!close(mesh.m_texCoords[0].y,.25))return 5;
 mesh.Validate();mesh.DefaultMissingAttributes();
 mesh.m_materials[triangle.material].diffuseMapPath.Clear();
 clPath output=folder+"/roundtrip.dae";
 if(!clAssimp::Export(output,mesh))return 6;
 clMesh restored=clAssimp::Import(output);
 if(restored.m_triangles.Size()!=1||restored.m_positions.Size()!=3)return 7;
 double x=0,y=0;for(const auto& p:restored.m_positions){x+=p.x;y+=p.y;}
 if(!close(x,2)||!close(y,3))return 8;
 const auto& restoredMaterial=restored.m_materials[restored.m_triangles[0].material];
 if(!close(restoredMaterial.diffuseColor.x,.25)||!close(restoredMaterial.diffuseColor.y,.5)||!close(restoredMaterial.diffuseColor.z,.75))return 9;
 if(clAssimp::Import(folder+"/missing.obj").m_triangles.Size()!=0)return 10;
 if(clAssimp::Export(folder+"/missing/subfolder/out.dae",mesh))return 11;
 puts("Assimp C API import/export roundtrip passed");return 0;
}
