#include "clControls.h"
#include "clWindow.h"
#include "clCamera.h"
#include "clAssets.h"
#include "cl2DDraw.h"
#include "Racer.h"

void ProbeRacerPos()
{
  clVec2 camCenter = clVec2::Zero();
  clAABB2<float> viewArea(camCenter);
  viewArea.GrowUniform(1.0f);
  clVec3D pos(clVec2D((viewArea.max + viewArea.min) / 2), (double)viewArea.Width() + 1.75);
}
