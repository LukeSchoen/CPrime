#include "clControls.h"
#include "clWindow.h"
#include "clCamera.h"
#include "clAssets.h"
#include "cl2DDraw.h"
#include "Racer.h"

void ProbeRacerCamera()
{
  clVec2 camCenter = clVec2::Zero();
  clVec2 carPos = clVec2::One();
  clAABB2<float> viewArea(camCenter);
  viewArea.GrowUniform(1.0f);
  clWindow window("Racer", 1, false, clWindow::ScreenResolution(0), clWCF_VSync);
  clFPSCamera camera(clVec3D(clVec2D((viewArea.max + viewArea.min) / 2), (double)viewArea.Width() + 1.75), { 0, clDegreesToRadians(270) }, clDegreesToRadians(45), window.Aspect());
  (void)carPos;
}
