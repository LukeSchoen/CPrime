#include "clControls.h"
#include "clWindow.h"
#include "clCamera.h"
#include "clAssets.h"
#include "cl2DDraw.h"
#include "Racer.h"

void ProbeRacerCameraSplit()
{
  clVec2 camCenter = clVec2::Zero();
  clAABB2<float> viewArea(camCenter);
  viewArea.GrowUniform(1.0f);
  clWindow window("Racer", 1, false, clWindow::ScreenResolution(0), clWCF_VSync);

  clVec3D pos(clVec2D((viewArea.max + viewArea.min) / 2), (double)viewArea.Width() + 1.75);
  clVec3D rot = { 0, clDegreesToRadians(270), 0 };
  double fov = clDegreesToRadians(45);
  double aspect = window.Aspect();
  clFPSCamera camera(pos, rot, fov, aspect);
}
