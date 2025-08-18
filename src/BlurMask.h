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


// Custom type defs
struct IndexHasher {
  std::size_t operator()(const itk::Index<nDims>& idx) const {
    std::size_t h1 = std::hash<long>()(idx[0]);
    std::size_t h2 = std::hash<long>()(idx[1]);
    std::size_t h3 = std::hash<long>()(idx[2]);

    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

struct IndexEqual {
  bool operator()(const itk::Index<nDims>& a, const itk::Index<nDims>& b) const {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
  }
};

using LineMetaData = std::tuple<unsigned int, double, double>;
using InterpolationMetadata =
  std::unordered_map<itk::Index<nDims>, std::vector<LineMetaData>, IndexHasher, IndexEqual>;


// Main class
class BlurMaskGenerator {
 public:
  BlurMaskGenerator();

  // Inputs
  void SetLabelMask(UCharImageType::Pointer mask) { this->m_labelMask = mask; }
  void SetBrainMask(UCharImageType::Pointer mask) { this->m_brainMask = mask; }
  void SetSkullStripMask(UCharImageType::Pointer mask) { this->m_skullStripMask = mask; }
  void SetStepSize(float stepSize) { this->m_stepSize = stepSize; }

  // Methods
  void Generate();
  void BuildInterpolationMetaData(UCharImageType::Pointer referenceBlurMask,
				  UCharImageType::Pointer referenceLabelMask);
  VectorImageType::Pointer ApplyToWarp(VectorImageType::Pointer warp);
    
  // Outputs
  UCharImageType::Pointer GetMask() const { return this->m_outputMask; }
  vtkSmartPointer<vtkPolyData> GetLabelMesh() const { return this->m_labelMesh; }
  vtkSmartPointer<vtkPoints> GetPoints() const { return this->m_pointCloud; }
  std::vector<vtkSmartPointer<vtkIdList>> GetPointIds() const {
    return this->m_pointCloudLinePointIds; }
  
 private:
  // Blur mask generation images
  UCharImageType::Pointer m_labelMask;
  UCharImageType::Pointer m_brainMask;
  UCharImageType::Pointer m_skullStripMask;
  UCharImageType::Pointer m_blurMask;
  UCharImageType::Pointer m_outputMask;

  // Blur mask interpolation data (images probably have different sizes/resolutions)
  UCharImageType::Pointer m_blurMaskToApply;
  UCharImageType::Pointer m_labelMaskToApply;
  InterpolationMetadata m_interpolationMetaData;

  // Blur mask point cloud data
  vtkSmartPointer<vtkPolyData> m_labelMesh;
  vtkSmartPointer<vtkPoints> m_pointCloud;
  vtkSmartPointer<vtkPolyData> m_pointCloudLineMidpoints;
  
  std::vector<vtkSmartPointer<vtkIdList>> m_pointCloudLinePointIds;
  vtkSmartPointer<vtkUnsignedIntArray> m_pointCloudPointLineIds;
  vtkSmartPointer<vtkFloatArray> m_pointCloudLineInterpolationWeights;

  vtkSmartPointer<vtkCellArray> m_pointCloudLines;

  float m_stepSize = 0.5;
  unsigned int m_nInterpolationPoints = 4;
};

  
  
// ITK typedefs
using BinaryContourImageFilterType = itk::BinaryContourImageFilter<UCharImageType, UCharImageType>;
UCharImageType::Pointer BinaryContourImage(UCharImageType::Pointer image);

using BinaryFillholeImageFilterType = itk::BinaryFillholeImageFilter<UCharImageType>;
UCharImageType::Pointer BinaryFillHoles(UCharImageType::Pointer image);

using GradientImageFilterType =
  itk::GradientRecursiveGaussianImageFilter<FloatImageType, VectorImageType>;

using GVFImageFilterType =
  itk::GradientVectorFlowImageFilter<VectorImageType, VectorImageType, float>;

using MinimumMaximumImageCalculatorType = itk::MinimumMaximumImageCalculator<FloatImageType>;
float GetMaximumImageValue(FloatImageType::Pointer image);

using SignedMaurerDistanceMapImageFilterType =
    itk::SignedMaurerDistanceMapImageFilter<UCharImageType, FloatImageType>;
FloatImageType::Pointer SignedDistanceTransform(IntImageType::Pointer image);

using RescaleImageFilterType =
  itk::VectorRescaleIntensityImageFilter<VectorImageType, VectorImageType>;


// Functions
vtkSmartPointer<vtkPolyData> CreateLabelMesh
(UCharImageType::Pointer labelMask, FloatImageType::Pointer distanceMap);

unsigned int GetClosestPointFromIdList
(double p0[nDims], vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkIdList> pointIdList,
 double (&closestPointDist2));

#endif
