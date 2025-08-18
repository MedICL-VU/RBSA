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
  void SetCorticalParcellation(IntImageType::Pointer image) { this->m_parc = image; }
  void SetWarp(VectorImageType::Pointer warp) { this->m_warp = warp; }

  void SetLeftHemiImage(UCharImageType::Pointer image) {
    this->m_lMaskGM = BinaryThresholdImage<UCharImageType>(image, 1, 2, 0, 1);
    this->m_lMaskWM = BinaryThresholdImage<UCharImageType>(image, 1, 1, 0, 1);
  }
  void SetRightHemiImage(UCharImageType::Pointer image) {
    this->m_rMaskGM = BinaryThresholdImage<UCharImageType>(image, 1, 2, 0, 1);
    this->m_rMaskWM = BinaryThresholdImage<UCharImageType>(image, 1, 1, 0, 1);
  }

  
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
  IntImageType::Pointer m_parc;
  VectorImageType::Pointer m_warp;


  UCharImageType::Pointer m_lMaskGM, m_lMaskWM, m_rMaskGM, m_rMaskWM;  
  vtkSmartPointer<vtkPolyData> m_lGM0, m_lWM0, m_lGM1, m_lWM1, m_rGM0, m_rWM0, m_rGM1, m_rWM1;
};


void CalculateSurfaceDistance
(vtkSmartPointer<vtkPolyData> surface0, vtkSmartPointer<vtkPolyData> surface1);

float GetMeanDistanceWithinLabel(vtkSmartPointer<vtkPolyData> surface, const int& label);

#endif
