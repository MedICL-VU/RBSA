#include "Warp.h"

Warper::Warper(){}

void Warper::ComputeTransform()
{
  // Initialize
  auto registration = RegistrationMethodType::New();
  
  // Set input data
  FloatImageType::Pointer fixedImage =
    CastImage<UCharImageType, FloatImageType>(this->m_inputFixedImage);
  FloatImageType::Pointer movingImage =
    CastImage<UCharImageType, FloatImageType>(this->m_inputMovingImage);
  
  registration->SetFixedImage(fixedImage);
  registration->SetMovingImage(movingImage);
  
  // Set initial transform
  auto initialField = VectorImageType::New();
  initialField->SetRegions(fixedImage->GetLargestPossibleRegion());
  initialField->SetOrigin(fixedImage->GetOrigin());
  initialField->SetSpacing(fixedImage->GetSpacing());
  initialField->SetDirection(fixedImage->GetDirection());
  initialField->Allocate();
  initialField->FillBuffer(VectorImageType::PixelType(0.0));

  auto transform = DisplacementFieldTransformType::New();
  transform->SetDisplacementField(initialField);

  registration->SetInitialTransform(transform);
  
  // Set metric
  auto registrationMask = MaskObjectType::New();
  registrationMask->SetImage(this->m_inputMovingImage);

  auto metric = MSQMetricType::New();
  metric->SetFixedImageMask(registrationMask);
  metric->SetMovingImageMask(registrationMask);

  registration->SetMetric(metric);
  
  // Configure optimizer w/ multi-level resolution
  auto optimizer = GradientDescentOptimizerType::New();
  optimizer->SetNumberOfIterations(100);
  optimizer->SetLearningRate(1.0);

  auto observer = RegistrationObserver<GradientDescentOptimizerType>::New();
  observer->SetIterations({100, 100, 50, 10});

  unsigned int nLevels = 4;
  registration->SetNumberOfLevels(nLevels);
  registration->SetOptimizer(optimizer);
  registration->AddObserver(itk::MultiResolutionIterationEvent(), observer);

  // Image pyramid business
  itk::Array<int> shrinkFactors, smoothingSigmas;
  shrinkFactors.SetSize(nLevels);
  smoothingSigmas.SetSize(nLevels);

  for(unsigned int n = 0; n < nLevels; n++) {
    smoothingSigmas[n] = nLevels - (n + 1);
    shrinkFactors[n] = std::pow(2, smoothingSigmas[n]);
  }
  registration->SetShrinkFactorsPerLevel(shrinkFactors);
  registration->SetSmoothingSigmasPerLevel(smoothingSigmas);

    
  // Configure for multi-level resolution
  itk::Array<int> startingShrinkFactors;
  startingShrinkFactors.SetSize(nDims);

  for(unsigned int d = 0; d < nDims; d++) {
    startingShrinkFactors[d] = startingShrinkFactors[0];
  }

  auto pyramid = PyramidType::New();
  pyramid->SetInput(fixedImage);
  pyramid->SetNumberOfLevels(nLevels);
  pyramid->SetStartingShrinkFactors(shrinkFactors[0]);
  pyramid->Update();
  
  RegistrationMethodType::TransformParametersAdaptorsContainerType adaptorsContainer;
  for(unsigned int n = 0; n < nLevels; n++) {
    auto levelImage = pyramid->GetOutput(n);

    auto adaptor = DisplacementFieldTransformParametersAdaptorType::New();
    adaptor->SetRequiredSpacing(levelImage->GetSpacing());
    adaptor->SetRequiredSize(levelImage->GetLargestPossibleRegion().GetSize());
    adaptor->SetRequiredDirection(levelImage->GetDirection());
    adaptor->SetRequiredOrigin(levelImage->GetOrigin());
    
    adaptorsContainer.push_back(static_cast<BaseAdaptorPointer>(adaptor));
  }
  registration->SetTransformParametersAdaptorsPerLevel(adaptorsContainer);

  // Run
  registration->Update();
  
  // Get forward warp
  auto warpTransform =
    dynamic_cast<DisplacementFieldTransformType*>(registration->GetModifiableTransform());
  this->m_warp = warpTransform->GetDisplacementField();

  // Get inverse warp
  auto invertFieldFilter = InvertDisplacementFieldFilterType::New();
  invertFieldFilter->SetInput(this->m_warp);
  invertFieldFilter->SetMaximumNumberOfIterations(50);
  invertFieldFilter->SetMeanErrorToleranceThreshold(1e-6);
  invertFieldFilter->Update();
  this->m_inverseWarp = invertFieldFilter->GetOutput();

}


// ------------------ Other functions related to the warping ------------------


std::array<VectorImageType::Pointer, 2> CombineWarps(std::vector<RBSAOutTuple> warpData)
{
  VectorImageType::PixelType zeroVec = itk::NumericTraits<VectorImageType::PixelType>::ZeroValue();
  
  // Initialize new vector image
  VectorImageType::Pointer reference = std::get<1>(warpData.at(0));
  VectorImageType::Pointer outWarp = DuplicateImage<VectorImageType>(reference);
  outWarp->FillBuffer(zeroVec);
  VectorImageType::Pointer outInverseWarp = DuplicateImage<VectorImageType>(outWarp);
  
  // Combine inputs
  std::vector<VectorImageType::PixelType> disps, inverseDisps;
  ImageRegionIteratorWithIndexType<VectorImageType> iterator
    (outWarp, outWarp->GetLargestPossibleRegion());
  iterator.GoToBegin();

  while(!iterator.IsAtEnd()) {
    const VectorImageType::IndexType& index = iterator.GetIndex();

    // Get values from input warp if mask is nonzero
    for(const auto& data : warpData) {
      UCharImageType::Pointer mask = std::get<0>(data);
      VectorImageType::Pointer warp = std::get<1>(data);
      VectorImageType::Pointer inverseWarp = std::get<2>(data);
      
      if(mask->GetPixel(index) == 1) {
	disps.emplace_back(warp->GetPixel(index));
	inverseDisps.emplace_back(inverseWarp->GetPixel(index));
      }
    }

    // Set combined warp value
    if(!disps.empty()) {
      VectorImageType::PixelType pixel;
      pixel.Fill(0);

      for(const auto& disp : disps) {
        for(unsigned int d = 0; d < nDims; d++) {
          pixel[d] += disp[d];
        }
      }
      pixel /= static_cast<double>(disps.size());
      outWarp->SetPixel(index, pixel);
    }

    // Set combined inverseWarp value
    if(!inverseDisps.empty()) {
      VectorImageType::PixelType pixel;
      pixel.Fill(0);
      
      for(const auto& disp : inverseDisps) {
	for(unsigned int d = 0; d < nDims; d++) {
          pixel[d] += disp[d];
        }
      }
      pixel /= static_cast<double>(inverseDisps.size());
      outInverseWarp->SetPixel(index, pixel);
    }
    
    // Reset
    disps.clear();
    inverseDisps.clear();
    ++iterator;
  }

  // Output
  return {outWarp, outInverseWarp};
}


FloatImageType::Pointer WarpImage
(FloatImageType::Pointer image, VectorImageType::Pointer warp)
{
  auto interpolator = LinearInterpolateType<FloatImageType>::New();
  interpolator->SetInputImage(image);

  auto warpFilter = WarpImageFilterType::New();  
  warpFilter->SetInput(image);
  warpFilter->SetOutputDirection(image->GetDirection());
  warpFilter->SetOutputOrigin(image->GetOrigin());
  warpFilter->SetOutputSpacing(image->GetSpacing());
  warpFilter->SetOutputSize(image->GetLargestPossibleRegion().GetSize());
  warpFilter->SetInterpolator(interpolator);
  warpFilter->SetDisplacementField(warp);
  warpFilter->Update();

  return warpFilter->GetOutput();  
}


vtkSmartPointer<vtkPolyData> WarpSurface
(vtkSmartPointer<vtkPolyData> inputSurface, VectorImageType::Pointer warp,
 UCharImageType::Pointer mask, bool convertFromRAS, bool checkMask)
{
  auto surface = vtkSmartPointer<vtkPolyData>::New();
  surface->DeepCopy(inputSurface);

  const unsigned int& nPoints = surface->GetNumberOfPoints();

  // Deform the surface
  auto interpolator = LinearInterpolateType<VectorImageType>::New();
  interpolator->SetInputImage(warp);
  
  for(unsigned int p = 0; p < nPoints; p++) {
    double p0[nDims];
    surface->GetPoint(p, p0);
    
    // Check if point is inside the mask
    if(checkMask) {
      const IndexType& index = TransformNDimsDoubleToIndex<UCharImageType>(mask, p0, convertFromRAS);
      if(!mask->GetLargestPossibleRegion().IsInside(index)) {
	std::cerr << "[WarpSurfaces():] huh, index (" << index << ") is not inside :(\n";
      }
      if(mask->GetPixel(index) != 1) continue;
    }
    
    // Get field value at surface point and deform
    const ContinuousIndexType& cIndex =
      TransformNDimsDoubleToContinuousIndex<VectorImageType>(warp, p0, convertFromRAS);
    const VectorImageType::PixelType& disp = interpolator->EvaluateAtContinuousIndex(cIndex);
    
    double p1[nDims] = {p0[0], p0[1], p0[2]};
    for(unsigned int d = 0; d < nDims; d++) {
      p1[d] += (convertFromRAS) ? (rasShift[d] * disp[d]) : disp[d];
    }
    surface->GetPoints()->SetPoint(p, p1);
  }
  surface->BuildLinks();

  return surface;
}
