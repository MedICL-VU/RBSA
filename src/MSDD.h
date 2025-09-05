#ifndef MSDD_H
#define MSDD_H

#include <vtkDistancePolyDataFilter.h>

#include "ParcellateSurface.h"
#include "Warp.h"
#include "utils.h"

class MSDD {
 public:
  MSDD();

  // Inputs
  void SetTargetLabels(const std::vector<int>& labels) { this->m_labels = labels; }
  void SetCorticalParcellation(TPointer<IntImageType> image) { this->m_parc = image; }
  void SetWarp(TPointer<VectorImageType> warp) { this->m_warp = warp; }
  void SetWarpMask(TPointer<UCharImageType> mask) { this->m_warpMask = mask; }

  void MakeHemiTemplates(TPointer<IntImageType> mask,
			 const std::vector<int>& lGMs,
			 const std::vector<int>& lWMs,
			 const std::vector<int>& rGMs,
			 const std::vector<int>& rWMs);

  // Main methods
  void CreateCustomParcellation();
  void GetLabelHemis();
  void GenerateSurfaces();
  void Calculate();
  void Write(const std::string& filename);
  
 private:
  std::vector<int> m_labels;
  std::tuple<std::vector<int>, std::vector<int>> m_hemiLabels;
  std::vector<float> m_MSDDs;
  
  int m_defaultPixelValue;
  TPointer<IntImageType> m_parc;
  TPointer<VectorImageType> m_warp;
  TPointer<UCharImageType> m_warpMask;
  
  TPointer<UCharImageType> m_lMaskGM, m_lMaskWM, m_rMaskGM, m_rMaskWM;  
  vtkSmartPointer<vtkPolyData> m_lGM0, m_lWM0, m_lGM1, m_lWM1, m_rGM0, m_rWM0, m_rGM1, m_rWM1;
};


void CalculateSurfaceDistance(vtkSmartPointer<vtkPolyData> surface0,
			      vtkSmartPointer<vtkPolyData> surface1);

float GetMeanDistanceWithinLabel(vtkSmartPointer<vtkPolyData> surface, const int& label);

TPointer<UCharImageType> MakeBinaryMask(TPointer<IntImageType> ref,
					const std::vector<int>& labels,
					TPointer<UCharImageType> mask = nullptr);

#endif
