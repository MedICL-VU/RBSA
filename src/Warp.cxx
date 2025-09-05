#include "Warp.h"

//--------------------------------------------------------------------------------------------------

TPointer<VectorImageType> RegisterLabelMasks(TPointer<UCharImageType> movingInput,
					    TPointer<UCharImageType> fixedInput)
{
  // Initialize
  auto registration = RegistrationMethodType::New();

  // Set input data
  TPointer<FloatImageType> fixedImage = CastImage<UCharImageType, FloatImageType>(fixedInput);
  TPointer<FloatImageType> movingImage = CastImage<UCharImageType, FloatImageType>(movingInput);
  registration->SetFixedImage(fixedImage);
  registration->SetMovingImage(movingImage);

  // Set initial transform
  auto initialField = VectorImageType::New();
  initialField->SetRegions(fixedImage->GetLargestPossibleRegion());
  initialField->SetOrigin(fixedImage->GetOrigin());
  initialField->SetSpacing(fixedImage->GetSpacing());
  initialField->SetDirection(fixedImage->GetDirection());
  initialField->Allocate();
  initialField->FillBuffer(TPixel<VectorImageType>(0.0));

  auto transform = DisplacementFieldTransformType::New();
  transform->SetDisplacementField(initialField);

  registration->SetInitialTransform(transform);

  // Set metric
  auto registrationMask = MaskObjectType::New();
  registrationMask->SetImage(movingInput);

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

  // Get warp
  auto warpTransform =
    dynamic_cast<DisplacementFieldTransformType*>(registration->GetModifiableTransform());

  TPointer<VectorImageType> warp = warpTransform->GetDisplacementField();
  warp->DisconnectPipeline();
  return warp;
}


TPointer<FloatImageType> WarpImage(TPointer<FloatImageType> image,
				  TPointer<VectorImageType> warp)
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


inline bool isInsideMask(std::set<unsigned int> pointNeighborIds,
			 vtkSmartPointer<vtkPolyData> surface,
			 TPointer<UCharImageType> mask,
			 bool isRAS)
{
  for(const auto& q : pointNeighborIds) {
    double q0[nDims];
    surface->GetPoint(q, q0);
    
    const TIndex<UCharImageType>& idx = TransformNDimsDoubleToIndex<UCharImageType>(mask, q0, isRAS);
    if(!mask->GetLargestPossibleRegion().IsInside(idx)) {
      std::cerr << "[Warp::IsInsideMask()]: huh, index (" << idx << ") is not inside :(\n";
    }
    if(mask->GetPixel(idx) != 1) return true;
  }
  return false;
}


vtkSmartPointer<vtkPolyData> WarpSurface(vtkSmartPointer<vtkPolyData> inputSurface,
					 TPointer<VectorImageType> warp,
					 TPointer<UCharImageType> mask,
					 bool isRAS,
					 bool checkMask)
{
  // Initialize
  auto surface = vtkSmartPointer<vtkPolyData>::New();
  surface->DeepCopy(inputSurface);

  const unsigned int& nPoints = surface->GetNumberOfPoints();
  
  // Deform the surface
  auto interpolator = LinearInterpolateType<VectorImageType>::New();
  interpolator->SetInputImage(warp);

  vtkNew<vtkIdList> cellPointIds, pointCellIds;
  std::set<unsigned int> pointNeighborIds;
  
  for(unsigned int p = 0; p < nPoints; p++) {
    double p0[nDims];
    surface->GetPoint(p, p0);

    // Check if point (or any neighbors) are inside the mask
    if(checkMask) {
      if(pointNeighborIds.size() > 0) pointNeighborIds.clear();
      pointCellIds->Reset();
      surface->GetPointCells(p, pointCellIds);

      for(unsigned int i = 0; i < pointCellIds->GetNumberOfIds(); i++) {
	cellPointIds->Reset();
	surface->GetCellPoints(pointCellIds->GetId(i), cellPointIds);

	for(unsigned int j = 0; j < cellPointIds->GetNumberOfIds(); j++) {
	  pointNeighborIds.emplace(cellPointIds->GetId(j));
	}
      }

      if(isInsideMask(pointNeighborIds, surface, mask, isRAS)) continue;
    }
    
    // Get field value at surface point and deform
    const ContinuousIndexType& cIndex =
      TransformNDimsDoubleToContinuousIndex<VectorImageType>(warp, p0, isRAS);
    const TPixel<VectorImageType>& disp = interpolator->EvaluateAtContinuousIndex(cIndex);
    
    double p1[nDims] = {p0[0], p0[1], p0[2]};
    for(unsigned int d = 0; d < nDims; d++) {
      p1[d] -= (isRAS) ? (rasShift[d] * disp[d]) : disp[d];
    }
    surface->GetPoints()->SetPoint(p, p1);
  }
  surface->BuildLinks();

  return surface;
}
