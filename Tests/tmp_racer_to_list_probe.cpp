#include "clControls.h"
#include "clWindow.h"
#include "clCamera.h"
#include "clAssets.h"
#include "cl2DDraw.h"
#include "Racer.h"

void ProbeRacerToList()
{
  const bool spinCamera = false;
  float grip = 1.f;
  auto res = clWindow::ScreenResolution(0) - v2I(96, 48);
  clWindow window("Racer", 1, false, res, clWCF_VSync | clWCF_HighDPIAware);
  clControls::MouseSetLock(1);
  const float carSize = 1.f;
  cl2DDraw car; car.Load(clImage(clAssets::AssetsPath() + "/Games/RC/Car.png"), clVec2(0.5), carSize);
  cl2DDraw skid; skid.Load(clImage(clAssets::AssetsPath() + "/Games/RC/skid.png"), clVec2(0.5), carSize);
  ui32 darkGreen = 0xff004400;
  cl2DDraw grass; grass.Load(clImage(&darkGreen, clVec2I::One()), clVec2(0.5), carSize * 0.05f);
  clImage img(clAssets::AssetsPath() + "/Games/RC/Track.png");
  cl2DDraw track; track.Load(img, clVec2(0.5), 64);
  clList<clVec4> skids;
  clList<clVector2<clVec4>> grasses;
  (void)spinCamera;
  (void)grip;
  (void)window;
  (void)car;
  (void)skid;
  (void)grass;
  (void)track;
  (void)skids;
  (void)grasses;
}
