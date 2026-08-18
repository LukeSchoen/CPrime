#include "clControls.h"
#include "clWindow.h"
#include "clCamera.h"
#include "clAssets.h"
#include "cl2DDraw.h"
#include "Racer.h"

#define NAME Racer
#define ICON Racer.ico

void Racer()
{
  const bool spinCamera = false;

  float grip = 1.f;

  auto res = clWindow::ScreenResolution(0) - v2I(96, 48);
  //auto res = clWindow::ScreenResolution(0);

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

  clVec2 camCenter = clVec2::Zero();
  float camZoom = 1;

  float carLastDir = 0;

  car.m_pos = clVec2({ -5.62176037f, -17.7291603f });
  car.m_angle = -6.33552265f;

  while (clControls::Update())
  {
    window.Clear(0xff009500);
    
    f32 rotSpeed = 1.0f;
    if (clControls::KeyIsDown(SDL_SCANCODE_A)) car.SetAngle(car.m_angle + clDegreesToRadians(rotSpeed));
    if (clControls::KeyIsDown(SDL_SCANCODE_D)) car.SetAngle(car.m_angle - clDegreesToRadians(rotSpeed));

    if (clControls::KeyIsDown(SDL_SCANCODE_W))
    {
      float carSpeed = 0.18f;
      // Check Wheel Over Grass
      if (1)
      {
        // Todo: check all 4 wheels (spawn grass and slow car appropriately)
        for (i32 w = 0; w < 4; w++)
        {
          clVec2 offset = clVec2(((w & 1) * 2 - 1) * 0.7f, (((w >> 1) & 1) * 2 - 1) * 0.35f) * carSize * 0.5f;
          offset = clRotateVector(offset, car.m_angle);

          // Spawn Grass At Wheels If We Go Off-Track
          clVec2 sample = car.m_pos + offset;
          if ((track.SampleImage(sample) & 0xff000000) == 0)
          {
            carSpeed *= 0.2f;
            clVector2<clVec4> piece;
            piece[0] = clVec4(sample, clVec2(car.m_angle, 1.0f));
            piece[1] = clVec4(cpcRandomUnitVector2<float>() * cpcRandFloat(), 0, 0);
            grasses.PushBack(piece);
          }
        }
      }

      car.SetPos(car.m_pos + clDirectionFromAngle(carLastDir) * carSpeed * grip);
      if (clControls::KeyIsDown(SDL_SCANCODE_SPACE))
      {
        skids.PushBack(clVec4(car.m_pos, clVec2(car.m_angle, 1.0f)));
        grip *= 0.99f;
      }
      else
      {
        grip = (grip * 49 + 1) / 50.f;
        carLastDir = car.m_angle;
      }
    }
    
    // Update Camera
    const i32 blendRate = 30;
    camCenter = (camCenter * (blendRate - 1) + car.m_pos) / blendRate;
}

