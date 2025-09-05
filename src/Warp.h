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



// Type defs
using DisplacementFieldTransformType = itk::DisplacementFieldTransform<float, nDims>;

using RegistrationMethodType =
  itk::ImageRegistrationMethodv4<FloatImageType, FloatImageType, DisplacementFieldTransformType>;

using GradientDescentOptimizerType = itk::GradientDescentOptimizerv4Template<float>;

using MaskObjectType = itk::ImageMaskSpatialObject<nDims>;
using MSQMetricType =
  itk::MeanSquaresImageToImageMetricv4<FloatImageType, FloatImageType, FloatImageType, float>;

using BaseAdaptorPointer =
  RegistrationMethodType::TransformParametersAdaptorsContainerType::value_type;

using DisplacementFieldTransformParametersAdaptorType =
  itk::DisplacementFieldTransformParametersAdaptor<DisplacementFieldTransformType>;

using PyramidType =
  itk::RecursiveMultiResolutionPyramidImageFilter<FloatImageType, FloatImageType>;

using WarpImageFilterType = itk::WarpImageFilter<FloatImageType, FloatImageType, VectorImageType>;


// Functions
TPointer<VectorImageType> RegisterLabelMasks
(TPointer<UCharImageType> fixedInput, TPointer<UCharImageType> movingInput);

TPointer<FloatImageType> WarpImage
(TPointer<FloatImageType> image, TPointer<VectorImageType> warp);

vtkSmartPointer<vtkPolyData> WarpSurface
(vtkSmartPointer<vtkPolyData> inputSurface, TPointer<VectorImageType> warp,
 TPointer<UCharImageType> mask, bool isRAS = false, bool checkMask = false);


#endif
