#ifndef WARP_H
#define WARP_H

#include "itkArray.h"
#include "itkImageRegistrationMethod.h"
#include "itkImageRegistrationMethodv4.h"
#include "itkRecursiveMultiResolutionPyramidImageFilter.h"
#include "itkDemonsRegistrationFilter.h"
#include "itkLBFGSOptimizer.h"
#include "itkLBFGSOptimizerv4.h"
#include "itkMultiResolutionImageRegistrationMethod.h"
#include "itkMultiResolutionPDEDeformableRegistration.h"
#include "itkSyNImageRegistrationMethod.h"
#include "itkTimeVaryingVelocityFieldTransform.h"
#include "itkTimeVaryingVelocityFieldImageRegistrationMethodv4.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkMeanSquaresImageToImageMetricv4.h"
#include "itkCorrelationImageToImageMetricv4.h"
#include "itkGradientDescentOptimizerv4.h"
#include "itkMattesMutualInformationImageToImageMetricv4.h"
#include "itkCompositeTransform.h"
#include "itkDisplacementFieldTransform.h"
#include "itkDisplacementFieldTransformParametersAdaptor.h"
#include "itkTransformToDisplacementFieldFilter.h"
#include "itkInvertDisplacementFieldImageFilter.h"
#include "itkImageMaskSpatialObject.h"
#include "itkWarpImageFilter.h"

#include <vtkDistancePolyDataFilter.h>

#include "utils.h"


class Warper {
 public:
  Warper();

  // Methods
  void SetOriginalLabel(UCharImageType::Pointer image) { this->m_inputMovingImage = image; }
  void SetAtrophyLabel(UCharImageType::Pointer image) { this->m_inputFixedImage = image; }
  void SetRegistrationMask(UCharImageType::Pointer image) { this->m_metricMask = image; }
  void ComputeTransform();

  // Output warps
  void SetWarp(VectorImageType::Pointer warp) { this->m_warp = warp; }
  void SetInverseWarp(VectorImageType::Pointer warp) { this->m_inverseWarp = warp; }
  VectorImageType::Pointer GetWarp() const { return this->m_warp; }
  VectorImageType::Pointer GetInverseWarp() const { return this->m_inverseWarp; }

 private:
  UCharImageType::Pointer m_inputFixedImage;
  UCharImageType::Pointer m_inputMovingImage;
  UCharImageType::Pointer m_metricMask;
  VectorImageType::Pointer m_warp;
  VectorImageType::Pointer m_inverseWarp;
};

//
using DisplacementFieldTransformType = itk::DisplacementFieldTransform<float, nDims>;
using InvertDisplacementFieldFilterType =
  itk::InvertDisplacementFieldImageFilter<VectorImageType, VectorImageType>;

using RegistrationMethodType =
  itk::ImageRegistrationMethodv4<FloatImageType, FloatImageType, DisplacementFieldTransformType>;
using GradientDescentOptimizerType = itk::GradientDescentOptimizerv4Template<float>;
using MSQMetricType =
  itk::MeanSquaresImageToImageMetricv4<FloatImageType, FloatImageType, FloatImageType, float>;
using MaskObjectType = itk::ImageMaskSpatialObject<nDims>;

using ResampleType = itk::ResampleImageFilter<FloatImageType, FloatImageType>;

using PyramidType =
  itk::RecursiveMultiResolutionPyramidImageFilter<FloatImageType, FloatImageType>;
using BaseAdaptorPointer =
  RegistrationMethodType::TransformParametersAdaptorsContainerType::value_type;
using DisplacementFieldTransformParametersAdaptorType =
  itk::DisplacementFieldTransformParametersAdaptor<DisplacementFieldTransformType>;


// Applying the transforms
using WarpImageFilterType = itk::WarpImageFilter<FloatImageType, FloatImageType, VectorImageType>;

std::array<VectorImageType::Pointer, 2> CombineWarps(std::vector<RBSAOutTuple> warpData);

FloatImageType::Pointer WarpImage
(FloatImageType::Pointer image, VectorImageType::Pointer warp);

vtkSmartPointer<vtkPolyData> WarpSurface
(vtkSmartPointer<vtkPolyData> inputSurface, VectorImageType::Pointer warp,
 UCharImageType::Pointer mask, bool convertFromRAS = false, bool checkMask = false);

	  

// Custom observer for registration
template <typename TOptimizer>
class RegistrationObserver : public itk::Command {
 public:
  using Self = RegistrationObserver;
  using SuperClass = itk::Command;
  using Pointer = itk::SmartPointer<Self>;
  itkNewMacro(Self);

  void SetLearningRates(const std::vector<double>& lr) { this->m_learningRates = lr; }
  void SetIterations(const std::vector<unsigned int>& iters) { this->m_iterations = iters; }
  void SetOptimizer(TOptimizer* opt) { this->m_optimizer = opt; }

  void Execute(itk::Object* caller, const itk::EventObject& event) override {
    Execute(const_cast<const itk::Object*>(caller), event);
  }

  void Execute(const itk::Object*, const itk::EventObject& event) override {
    if (!itk::MultiResolutionIterationEvent().CheckEvent(&event)) return;

    unsigned int level = m_currentLevel;
    if (level >= m_learningRates.size()) return;

    m_optimizer->SetLearningRate(m_learningRates[level]);
    m_optimizer->SetNumberOfIterations(m_iterations[level]);

    std::cout << "Starting level " << level
              << " with learning rate " << m_learningRates[level]
              << " and iterations " << m_iterations[level] << std::endl;

    ++m_currentLevel;
  }

 private:
  std::vector<double> m_learningRates;
  std::vector<unsigned int> m_iterations;
  typename TOptimizer::Pointer m_optimizer;
  unsigned int m_currentLevel = 0;
};



#endif
