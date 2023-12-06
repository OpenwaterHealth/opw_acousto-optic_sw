#include <cmath>
#include <cstring>
#include <cstdlib>

#include "system/component/inc/fftt.h"
#include "system/component/inc/frame_draw.h"
#include "system/component/inc/invertroi.h"
#include "system/component/inc/roi.h"
#include "system/component/inc/rcam.h"
#include "system/component/inc/syncnode.h"

#include "fftw3.h"

#pragma comment(lib, "sfml-graphics-s.lib")
#pragma comment(lib, "sfml-window-s.lib")
#pragma comment(lib, "sfml-system-s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "freetype.lib")

#pragma comment(lib, "libfftw3-3.lib")

#include "windows.h"

static const int WIN_X = 1280;
static const int WIN_Y = 720;

static const int DIM_X = 5;
static const int DIM_Y = 5;

static void PrintArr(const uint16_t* arr, int x, int y) {
  for (int j = 0; j < y; ++j) {
    for (int i = 0; i < x; ++i) {
      printf("%-3d ", arr[j * DIM_X + i]);
    }
    printf("\n");
  }
  printf("\n");
}

static void PrintArr(const double* arr, int x, int y) {
  for (int j = 0; j < y; ++j) {
    for (int i = 0; i < x; ++i) {
      printf("%-3.0lf ", arr[j * DIM_X + i]);
    }
    printf("\n");
  }
  printf("\n");
}

static void ScaleArr(double* arr, int x, int y, double scale) {
  for (int i = 0; i < x * y; ++i) {
    arr[i] /= scale;
  }
}

int main() {
  srand(GetTickCount());

  Frame fr(DIM_X, DIM_Y);
  FFTT fftt(&fr);
  InvertROI iroi(&fr);
  SyncNode sn;

  iroi.AddProducer(&fftt);
  sn.AddProducer(&iroi);

  iroi.Set(0.0, 0.0, 2.0);

  fftt.resize(20);

  for (int i = 0; i < DIM_X * DIM_Y; ++i) {
    fr.data[i] = rand() % 1000;
  }

  PrintArr(fr.data, DIM_X, DIM_Y);

  fftt.Consume((void*)&fr);
  Frame* fr_out = (Frame*)sn.Wait();

  PrintArr(InvertROI::GetTag(fr_out)->ifft, DIM_X, DIM_Y);
  printf("Mean: %.2e\n", InvertROI::GetTag(fr_out)->mean);
  printf("Stddev: %.2e\n", InvertROI::GetTag(fr_out)->stddev);
  fr_out = (Frame*)sn.Get();


  system("pause");

  return 0;
}
