#ifndef BLUR_MASK_H
#define BLUR_MASK_H

#include <itkSignedMaurerDistanceMapImageFilter.h>
#include <itkGradientImageFilter.h>
#include <itkGradientRecursiveGaussianImageFilter.h>
#include <itkGradientVectorFlowImageFilter.h>
#include <itkVectorRescaleIntensityImageFilter.h>
#include <itkBinaryFillholeImageFilter.h>

#include "BinaryImageMorphology.h"
#include "utils.h"


const unsigned int nInterpolationPoints = 4;


// Custom typedefs
struct PixelToLineData {
  unsigned int lineId; // line id
  float dist2;         // squared distance to point
  float t;             // vtk param
};

template <unsigned int N_max>
struct ClosestNLines {
  std::array<PixelToLineData, N_max> a{};
  unsigned int N = 0;

  void consider(unsigned int lineId, float dist2, float t) {
    if(N < N_max) {
      // Haven't seen N lines yet, so just add to array and continue
      a[N++] = {lineId, dist2, t};
    }
    else {
      // Array is full -> check if input dist2 is small and swap with largest
      auto maxIt = std::max_element(a.begin(), a.end(),
				    [](auto& x, auto&y) { return x.dist2 < y.dist2; });
      if(dist2 < maxIt->dist2) *maxIt = {lineId, dist2, t};
    }
  }
};


// Main class
class BlurMaskGenerator {
 public:
  BlurMaskGenerator();
  
  // Methods
  void SetStepSize(float stepSize) { this->m_stepSize = stepSize; }
  TPointer<UCharImageType> Generate(TPointer<UCharImageType> labelMask,
				   TPointer<UCharImageType> brainMask,
				   TPointer<UCharImageType> skullStripMask);
  TPointer<VectorImageType> ApplyToWarp(TPointer<VectorImageType> warp,
				       TPointer<UCharImageType> blurMask,
				       TPointer<UCharImageType> labelMask);
    
 private:
  // Parameters
  float m_stepSize = 0.5;
  unsigned int m_nInterpolationPoints = 4;

  // Blur mask point cloud data
  vtkSmartPointer<vtkPoints> m_pointCloud;
  vtkSmartPointer<vtkCellArray> m_pointCloudLines;
};
  
  
// ITK typedefs
using BinaryContourImageFilterType = itk::BinaryContourImageFilter<UCharImageType, UCharImageType>;

using GradientImageFilterType =
  itk::GradientRecursiveGaussianImageFilter<FloatImageType, VectorImageType>;

using SignedMaurerDistanceMapImageFilterType =
  itk::SignedMaurerDistanceMapImageFilter<UCharImageType, FloatImageType>;

using RescaleImageFilterType =
  itk::VectorRescaleIntensityImageFilter<VectorImageType, VectorImageType>;


// Functions
vtkSmartPointer<vtkPolyData> CreateLabelMesh(TPointer<UCharImageType> labelMask,
					     TPointer<FloatImageType> distanceMap);

#endif
