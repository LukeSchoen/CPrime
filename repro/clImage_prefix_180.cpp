#include "clLZ4.h"
#include "clDDA2.h"
//#include "clAVIF.h"
#include "windows.h"
#include "clColor.h"
#include "clImage.h"
#include "vnImagine.h"
#include "clImageRaw.h"
#include "clFileDump.h"


#include "clHistogram.h"
#include "clFileStream.h"
#include "clDepthImage.h"
#include "clFileHelper.h"
#include "clCannyFilter.h"
#include "clInterpolate.h"
#include "clReachabilityFinder.h"
#include "clLoop.h"
#include "clThirdPartyDir.h"

#pragma comment(lib, clThirdPartyPath "webp/lib/libwebp.lib")
#pragma comment(lib, clThirdPartyPath "libtiff/lib/libtiff.lib")
#pragma comment(lib, clThirdPartyPath "Imagine/lib/Imagine.lib")

#define FAST

clImage::clImage(const ui32 *pData, const clVec2I &size) : clArray2<ui32>(pData, size) {}
clImage::clImage(const clArray2<ui32> &data) : clArray2<ui32>(data) {}
clImage::clImage(const clPath &inputFile) { LoadImageFile(inputFile); }
clImage::clImage(clArray2<ui32> &&data) : clArray2<ui32>(data) {}

bool clImage::LoadImage(const void *pFileData, const i64 &fileLength, const bool &jpegNicestHint)
{
  i64 width, height;
  ui32 *pNewImageData = nullptr;
  if (!clImageRawLoadLZIFromMemory(pFileData, &pNewImageData, fileLength, &width, &height))
    //if (!clImageRawLoadAVIFFromMemory(pFileData, &pNewImageData, fileLength, &width, &height))
      if (!clImageRawLoadDDSFromMemory(pFileData, &pNewImageData, fileLength, &width, &height))
        if (!clImageRawLoadPNMFromMemory(pFileData, &pNewImageData, fileLength, &width, &height))
          if (!clImageRawLoadTiffFromMemory(pFileData, &pNewImageData, fileLength, &width, &height))
            if (!clImageRawLoadWebpFromMemory(pFileData, &pNewImageData, fileLength, &width, &height))
              //if (!clImageRawLoadGifFromMemory(pFileData, &pNewImageData, fileLength, &width, &height))
              //if (!clImageRawLoadJpegFromMemoryFast(pFileData, &pNewImageData, fileLength, &width, &height, jpegNicestHint))
              if (!clImageRawLoadFromMemory(pFileData, &pNewImageData, fileLength, &width, &height))
                return false;

  // Copy
  //*this = clArray2<ui32>(pNewImageData, clVec2I((int)width, (int)height));
  //clFree(pNewImageData);
  // Move
  m_data.SetData(pNewImageData, width * height, width * height);
  m_size.x = width;
  m_size.y = height;

  return true;
}

bool clImage::LoadImageFile(const clPath &inputFile)
{
  clFileReader reader(64 * 1024);
  if (!reader.Open(inputFile))
  {
    //clRelFail(("Failed To Load Image: " + inputFile.Path()).c_str());
    return false;
  }
  return LoadImage(reader.Read(reader.Length(), nullptr), reader.Length());
}

bool clImage::SupportedExtension(const clString &_ext)
{
  clString ext = _ext.ToLower();
  if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "hdr" || ext == "tga" || ext == "bmp" || ext == "psd" || ext == "gif" || ext == "pic"
    || ext == "pgm" || ext == "ppm" || ext == "pnm" || ext == "tif" || ext == "tiff" || ext == "dds" || ext == "gif" || ext == "webp" || ext == "avif"
    || ext == "lzi")
  {
    return true;
  }
  return false;
}

bool clImage::SaveFileLZI(const clPath &outputLZI) const
{
  i64 estimatedMaxSize = m_size.Area() * sizeof(ui32) + 1024;
  clFileDump stream(outputLZI, estimatedMaxSize, estimatedMaxSize);
  stream.Write("LZI", 3);
  stream.Write(&m_size.x, 4);
  stream.Write(&m_size.y, 4);
  auto data = clLZ4::Compress(m_data.Data(), m_size.Area() * sizeof(ui32));
  stream.Write(data.Data(), data.Size());
  return true;
}

bool clImage::SaveFilePNG(const clPath &outputPNG) const
{
  i64 len = 0;
  ui8 *pImgData = nullptr;
  if (!clImageRawSavePNGToMemory(Data(), &pImgData, Size().x, Size().y, &len)) return false;
  clFileHelper::Write(outputPNG, pImgData, (i64)len);
  free(pImgData);
  return true;
}
bool clImage::SaveFileWebP(const clPath &outputWebP, const float &quality) const { return clImageRawSaveWebpToFile(outputWebP.Path().c_str(), Data(), Size().x, Size().y, quality); }

bool clImage::SaveFilePNM(const clPath &outputPNM) const { return clImageRawSavePNMToFile(outputPNM.Path().c_str(), Data(), Size().x, Size().y); }
bool clImage::SaveFilePGM(const clPath &outputPGM) const { return clImageRawSavePGMToFile(outputPGM.Path().c_str(), Data(), Size().x, Size().y); }
bool clImage::SaveFileBMP(const clPath &outputBMP) const { return clImage(SwapRGB().FlipY()).SaveFileBMPRaw(outputBMP); }
bool clImage::SaveFileTGA(const clPath &outputTGA) const { i64 outputSize = 0; return clImageRawSaveTGAToFile(outputTGA.Path().c_str(), Data(), Size().x, Size().y, &outputSize); }

//bool clImage::SaveFileBMP(const clPath &outputBMP) const { return clImageRawSaveBMPToFile(outputBMP.Path().c_str(), Data(), Size().x, Size().y); }
bool clImage::SaveFileTIF(const clPath &outputTIF) const { return clImageRawSaveTiffToFile(outputTIF.Path().c_str(), Data(), Size().x, Size().y); }
bool clImage::SaveFileBMPRaw(const clPath &outputBMP) const { return clImageRawSaveBMPToFileFast(outputBMP.Path().c_str(), Data(), Size().x, Size().y); }
//bool clImage::SaveFileAVIF(const clPath &outputAVIF) const { auto data = clAVIF::Encode(*this); return clFileHelper::Write(outputAVIF, data.Data(), data.Size()); }
bool clImage::SaveFileJPG(const clPath &outputJPG, const JPGQuality &quality) const { return clImageRawSaveJPGToFile(outputJPG.Path().c_str(), Data(), Size().x, Size().y, quality); }

clList<char> clImage::SaveMemoryPNG() const
{
  clList<char> ret;
  i64 len;
  ui8 *pImgData = nullptr;
  if (!clImageRawSavePNGToMemory(Data(), &pImgData, Size().x, Size().y, &len))
    return ret;
  ret.PushBack((char *)pImgData, len);
  free(pImgData);
  return ret;
}

clImage clImage::Resize(const clVec2I &newSize, const bool &interpolate /*= false*/) const
{
  if (newSize == Size() || (newSize.x == -1 && newSize.y == -1)) return *this;

  if (newSize.x == -1) return Resize(clVec2I(Size().x / ((float)Size().y / newSize.y), newSize.y), interpolate);
  if (newSize.y == -1) return Resize(clVec2I(newSize.x, Size().x / ((float)Size().x / newSize.x)), interpolate);

  if (interpolate)
  {
    bool widthVerySmall = newSize.x < Size().x / 2;
    bool heightVerySmall = newSize.y < Size().y / 2;
    if (widthVerySmall || heightVerySmall)
      return Resize(clVec2I(Size().x / (widthVerySmall ? 2 : 1), Size().y / (heightVerySmall ? 2 : 1)), interpolate).Resize(newSize, interpolate);

    CVImage cvInput;
    CVImage cvOutput;
    vnCreateImage(VN_IMAGE_FORMAT_R8G8B8A8, Size().x, Size().y, &cvInput);
    i64 pitch = cvInput.RowPitch();
    if (!pitch) pitch = Size().x * 4;
    for (int y = 0; y < Size().y; y++)
      memcpy(cvInput.QueryData() + y * pitch, (ui8 *)(Data() + y * Size().x), Size().x * 4);
    vnResizeImage(cvInput, VN_IMAGE_KERNEL_GAUSSIAN, newSize.x, newSize.y, 0, &cvOutput);
    clImage ret(nullptr, newSize);
    pitch = cvOutput.RowPitch();
    if (!pitch) pitch = newSize.x * 4;
    for (int y = 0; y < ret.Size().y; y++)
      memcpy((ui8 *)(ret.Data() + y * ret.Size().x), cvOutput.QueryData() + y * pitch, newSize.x * 4);
    return ret;
  }
  else
  { // Nearest
    clImage ret(nullptr, newSize);
    clVec2 iRatio = clVec2(Size()) / clVec2(newSize);
    for (i64 y = 0; y < newSize.y; ++y)
      for (i64 x = 0; x < newSize.x; ++x)
      {
        clVec2 oldPos = iRatio * clVec2((float)x, (float)y);
        ret.m_data[x + y * ret.Size().x] = m_data[(i64)oldPos.x + (i64)oldPos.y * Size().x];
      }
    return ret;
  }
}

clImage clImage::Resize(const i64 &maxSideLen, const bool &interpolate /*= false*/) const
{
  float longSide = (float)clMaxComponent(Size());
  if (longSide > maxSideLen)
  {
    float sizeMultiplier = longSide / maxSideLen;
    return Resize(clVec2I(clMax(i32(Size().x / sizeMultiplier), 1), clMax(i32(Size().y / sizeMultiplier), 1)), interpolate);
  }
  return *this;
}
