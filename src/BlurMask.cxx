#include "BlurMask.h"
#include "ParcellateSurface.h"
#include "utils.h"


/*struct IndexHasher {
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
  };*/



UCharImageType::Pointer BinaryContourImage(UCharImageType::Pointer image)
{
  auto filter = BinaryContourImageFilterType::New();
  filter->SetInput(image);
  filter->SetForegroundValue(0);
  filter->SetBackgroundValue(1);
  filter->Update();

  return filter->GetOutput();
}


UCharImageType::Pointer BinaryFillHoles(UCharImageType::Pointer image)
{
  auto filter = BinaryFillholeImageFilterType::New();
  filter->SetInput(image);
  filter->SetForegroundValue(1);
  filter->Update();

  return filter->GetOutput();
}



vtkSmartPointer<vtkPolyData> CreateLabelMesh
(UCharImageType::Pointer labelMask, FloatImageType::Pointer distanceMap)
{
  auto mesh = vtkSmartPointer<vtkPolyData>::New();
  BinaryITKImageToVTKMesh(labelMask, mesh);
  vtkSmartPointer<vtkDataArray> normals = mesh->GetPointData()->GetNormals();

  // Determine which vertices of mesh lie on the CSF boundary
  vtkNew<vtkUnsignedCharArray> isEdge;
  isEdge->SetNumberOfComponents(1);
  isEdge->SetNumberOfValues(mesh->GetNumberOfPoints());
  isEdge->SetName("Is edge");
  isEdge->Fill(0);

  itk::Point<double, nDims> x0;
  ContinuousIndexType cIdx;
  UCharImageType::IndexType idx;

  auto interpolator = LinearInterpolateType<FloatImageType>::New();
  interpolator->SetInputImage(distanceMap);

  for(unsigned int p = 0; p < mesh->GetNumberOfPoints(); p++) {
    double p0[nDims], p1[nDims];
    mesh->GetPoint(p, p0);
        
    ContinuousIndexType cIdx =
      TransformNDimsDoubleToContinuousIndex<FloatImageType>(distanceMap, p0);
    FloatImageType::IndexType idx = TransformNDimsDoubleToIndex<FloatImageType>(distanceMap, p0);

    if(distanceMap->GetLargestPossibleRegion().IsInside(idx)) {
      unsigned char x = (interpolator->EvaluateAtContinuousIndex(cIdx) >= 0) ? 1 : 0; 
      isEdge->SetValue(p, x);
    }
    else {
      std::cerr << "huh, index is not inside\n";
    }
  }
  mesh->GetPointData()->AddArray(isEdge);
  mesh->BuildLinks();

  return mesh;
}


UCharImageType::Pointer LabelCSFEdgeMap
(UCharImageType::Pointer labelMask, UCharImageType::Pointer brainMask)
{
  // Get contour map of brainMask
  UCharImageType::Pointer fullEdgeMap = BinaryContourImage(brainMask);
  fullEdgeMap = BinaryThresholdImage<UCharImageType>(fullEdgeMap, 0, 0, 0, 1);

  // Isolate voxels of fullEdgeMap that touch labelMask
  BinaryBallStructuringElementType kernel;
  kernel.SetRadius(kernelRadius);
  kernel.CreateStructuringElement();

  UCharImageType::Pointer mask1 = DilateImage(fullEdgeMap, kernel);
  mask1 = MultiplyImages<UCharImageType>(mask1, labelMask);

  // Isolate voxels of labelMask that touch fullEdgeMap
  UCharImageType::Pointer mask2 = DilateImage(labelMask, kernel);
  mask2 = MultiplyImages<UCharImageType>(mask2, fullEdgeMap);

  // Combine and return
  UCharImageType::Pointer labelEdgeMask = AddImages<UCharImageType>(mask1, mask2);

  return labelEdgeMask;
}
  

float GetMaximumImageValue(FloatImageType::Pointer image)
{
  auto filter = MinimumMaximumImageCalculatorType::New();
  filter->SetImage(image);
  filter->Compute();

  return filter->GetMaximum();
}


VectorImageType::Pointer Gradient(FloatImageType::Pointer image)
{
  auto filter = GradientImageFilterType::New();
  filter->SetInput(image);
  filter->Update();

  return filter->GetOutput();
}


void NormalizeITKVector(VectorImageType::PixelType &vec)
{
  float sum = 0;
  for(unsigned int d = 0; d < nDims; d++) {
    sum += vec[d];
  }
  for(unsigned int d = 0; d < nDims; d++) {
    vec[d] /= sum;
  }
}
			       

FloatImageType::Pointer SignedDistanceTransform(UCharImageType::Pointer image) {
  auto filter = SignedMaurerDistanceMapImageFilterType::New();
  filter->SetInput(image);
  filter->Update();

  return filter->GetOutput();
}



/*
  Class specific
*/

BlurMaskGenerator::BlurMaskGenerator()
{
  this->m_pointCloud = vtkSmartPointer<vtkPoints>::New();
  this->m_pointCloudLineMidpoints = vtkSmartPointer<vtkPolyData>::New();

  this->m_pointCloudPointLineIds = vtkSmartPointer<vtkUnsignedIntArray>::New();
  this->m_pointCloudPointLineIds->SetNumberOfComponents(1);

  this->m_pointCloudLineInterpolationWeights = vtkSmartPointer<vtkFloatArray>::New();
  this->m_pointCloudLineInterpolationWeights->SetNumberOfComponents(1);

  this->m_pointCloudLines = vtkSmartPointer<vtkCellArray>::New();
  this->m_stepSize = 0.5;
  this->m_nInterpolationPoints = 4;
}
  

void BlurMaskGenerator::Generate()
//(UCharImageType::Pointer labelMask, UCharImageType::Pointer brainMask,
// UCharImageType::Pointer skullStripMask)
{
  // Check for inputs
  if(!this->m_labelMask) {
    throw std::runtime_error("BlurMaskGenerator missing required input (LabelMask)");
  }
  if(!this->m_brainMask) {
    throw std::runtime_error("BlurMaskGenerator missing required input (BrainMask)");
  }
  if(!this->m_skullStripMask) {
    throw std::runtime_error("BlurMaskGenerator missing required input (SkullStripMask)");
  }

  // Get signed distance transform/gradient and edges from brainMask
  FloatImageType::Pointer distanceMap = SignedDistanceTransform(this->m_brainMask);
  VectorImageType::Pointer distanceGrad = Gradient(distanceMap);
  this->m_blurMask = LabelCSFEdgeMap(this->m_labelMask, this->m_brainMask);
  
  // Convert edge map to vtkPolyData (to get a set of points and normals on edge)
  this->m_labelMesh = CreateLabelMesh(this->m_labelMask, distanceMap);
  vtkSmartPointer<vtkUnsignedCharArray> isEdge =
    vtkUnsignedCharArray::SafeDownCast(this->m_labelMesh->GetPointData()->GetArray("Is edge"));
  vtkSmartPointer<vtkDataArray> normals = this->m_labelMesh->GetPointData()->GetNormals();

  // Initialize pointCloud (points) and pointCloudLines (cell array)
  unsigned int nEdgePoints = 0;
  for(unsigned int p = 0; p < this->m_labelMesh->GetNumberOfPoints(); p++) {
    if(isEdge->GetValue(p) > 0) {
      nEdgePoints++;
    }
  }
  this->m_pointCloud->Allocate(nEdgePoints);
  this->m_pointCloudLines->AllocateEstimate(nEdgePoints, 2);

  // Fill the CSF around the label w/ an ordered point cloud
  auto brainMaskInterpolator = LinearInterpolateType<UCharImageType>::New();
  brainMaskInterpolator->SetInputImage(this->m_brainMask);
  auto distanceMapInterpolator = LinearInterpolateType<FloatImageType>::New();
  distanceMapInterpolator->SetInputImage(distanceMap);
  auto distanceGradInterpolator = LinearInterpolateType<VectorImageType>::New();
  distanceGradInterpolator->SetInputImage(distanceGrad);
  auto skullStripMaskInterpolator = LinearInterpolateType<UCharImageType>::New();
  skullStripMaskInterpolator->SetInputImage(this->m_skullStripMask);

  unsigned int pointId = 0, lineId = 0;
  double x0[nDims], direction[nDims], xt[nDims], x1[nDims];
  
  for(unsigned int p = 0; p < this->m_labelMesh->GetNumberOfPoints(); p++) {
    if(isEdge->GetValue(p) == 1) {
      // Initialize
      this->m_labelMesh->GetPoint(p, x0);
      this->m_labelMesh->GetPoint(p, xt);
      normals->GetTuple(p, direction);
      
      ContinuousIndexType cIdxt =
	TransformNDimsDoubleToContinuousIndex<UCharImageType>(this->m_blurMask, xt);
      float Dxt = distanceMapInterpolator->EvaluateAtContinuousIndex(cIdxt);
      
      // Traverse along normal from p0 to edge of skullStripMask
      bool atMedialBoundary = false;
      bool insideBrain = false;
      bool insideSkullStrip = true;
      bool isValidLine = false;
      
      while(!atMedialBoundary && !insideBrain && insideSkullStrip) {
	// Get data at next point
	for(unsigned int d = 0; d < nDims; d++) {
	  x1[d] = xt[d] + this->m_stepSize * direction[d];
	}
	
	// Still travelling down the distance map gradient?
	ContinuousIndexType cIdx1 =
	  TransformNDimsDoubleToContinuousIndex<UCharImageType>(this->m_blurMask, x1);
        float Dx1 = distanceMapInterpolator->EvaluateAtContinuousIndex(cIdx1);

	// Are we too close to adjacent tissue?
	if(Dxt >= Dx1) {
	  atMedialBoundary = true;
	  continue;
	}

	// Still inside skull strip mask?
	if(!this->m_skullStripMask->GetLargestPossibleRegion().IsInside(cIdx1)
	   || skullStripMaskInterpolator->EvaluateAtContinuousIndex(cIdx1) < 0.5) {
	  insideSkullStrip = false;
	  continue;
	}

	// Update mask
	UCharImageType::IndexType idx =
	  TransformNDimsDoubleToIndex<UCharImageType>(this->m_blurMask, x1);
	this->m_blurMask->SetPixel(idx, 1);

	// Reset for next iteration
	for(unsigned int d = 0; d < nDims; d++) {
	  xt[d] = x1[d];
	  cIdxt[d] = cIdx1[1];
	}
	Dxt = Dx1;	
	isValidLine = true;
      }

      // Add new line to pointCloudLines
      if(isValidLine) {
	this->m_pointCloud->InsertNextPoint(x0);
	this->m_pointCloud->InsertNextPoint(x1);
	
	vtkNew<vtkLine> line;
	line->GetPointIds()->SetId(0, pointId);
	line->GetPointIds()->SetId(1, pointId + 1);
	this->m_pointCloudLines->InsertNextCell(line);
	
	pointId += 2;
	lineId += 1;
      }
    }
  }
    
  // Get final mask
  this->m_blurMask = MultiplyImages<UCharImageType>(this->m_blurMask, this->m_skullStripMask);

  this->m_outputMask = AddImages<UCharImageType>(this->m_labelMask, this->m_blurMask);
  this->m_outputMask = BinaryThresholdImage<UCharImageType>
    (this->m_outputMask, 1, std::numeric_limits<unsigned char>::max(), 0, 1);
}


void BlurMaskGenerator::BuildInterpolationMetaData
(UCharImageType::Pointer referenceBlurMask, UCharImageType::Pointer referenceLabelMask)
{
  // Blur mask to apply to warps (probably higher res than original)
  this->m_labelMaskToApply = referenceLabelMask;
  this->m_blurMaskToApply =
    SubtractImages<UCharImageType>(referenceBlurMask, this->m_labelMaskToApply);

  // Set up iterators
  auto pointCloudLineIterator = vtk::TakeSmartPointer(this->m_pointCloudLines->NewIterator());
  vtkNew<vtkIdList> linePointIds;
  linePointIds->Allocate(2);

  ImageRegionIteratorWithIndexType<UCharImageType>
    maskIterator(this->m_blurMaskToApply, this->m_blurMaskToApply->GetLargestPossibleRegion());
  
  // Precompute line distances for each voxel in this->m_blurMaskToApply
  const unsigned int& nLines = this->m_pointCloudLines->GetNumberOfCells();
  const unsigned int N = std::min(this->m_nInterpolationPoints, nLines);

  InterpolationMetadata interpolationMetaData;

  for(maskIterator.GoToBegin(); !maskIterator.IsAtEnd(); ++maskIterator) {
    if(maskIterator.Get() != 1) continue;

    // Get index as phyiscal point
    UCharImageType::IndexType index = maskIterator.GetIndex();
    double indexPoint[nDims];
    TransformIndexToNDimsDouble<UCharImageType>(referenceLabelMask, index, indexPoint);

    // Get closest distance from indexPoint to each line in pointCloudLines
    std::vector<LineMetaData> pointCloudLineMetaData;
    pointCloudLineMetaData.reserve(nLines);
    pointCloudLineIterator->GoToFirstCell();
    
    while(!pointCloudLineIterator->IsDoneWithTraversal()) {
      linePointIds->Reset();
      pointCloudLineIterator->GetCurrentCell(linePointIds);
      unsigned int lineId = pointCloudLineIterator->GetCurrentCellId();

      double p0[nDims], p1[nDims];
      this->m_pointCloud->GetPoint(linePointIds->GetId(0), p0);
      this->m_pointCloud->GetPoint(linePointIds->GetId(1), p1);
      
      double t;
      double dist2 = vtkLine::DistanceToLine(indexPoint, p0, p1, t, nullptr);
      pointCloudLineMetaData.emplace_back(lineId, dist2, t);

      pointCloudLineIterator->GoToNextCell();
    }

    // Sort interpolationPointData by smallest to largest dist2 and keep smallest
    std::partial_sort(pointCloudLineMetaData.begin(),
		      pointCloudLineMetaData.begin() + N,
		      pointCloudLineMetaData.end(),
		      [](const auto& a, const auto& b) { return std::get<1>(a) < std::get<1>(b); });
    pointCloudLineMetaData.resize(N);
    
    // Normalize the distances for each voxel
    double totalDist = 0.0;

    for(unsigned int n = 0; n < N; n++) {
      double& dist = std::get<1>(pointCloudLineMetaData.at(n));      
      dist = std::sqrt(dist);
      totalDist += dist;
    }

    if(totalDist > 0) {
      for(unsigned int n = 0; n < this->m_nInterpolationPoints; n++) {
	std::get<1>(pointCloudLineMetaData.at(n)) /= totalDist;
      }
    }

    // Add to meta data
    this->m_interpolationMetaData[index] = std::move(pointCloudLineMetaData);
  }
}


// Apply blur mask to warp
VectorImageType::Pointer BlurMaskGenerator::ApplyToWarp(VectorImageType::Pointer warp)
{
  // Iterate over pointCloudLines to interpolate field value at line origin
  const unsigned int& nLines = this->m_pointCloudLines->GetNumberOfCells();
  const unsigned int N = std::min(this->m_nInterpolationPoints, nLines);

  std::unordered_map<unsigned int, VectorImageType::PixelType> interpolationValues;
  interpolationValues.reserve(nLines);
  
  auto warpInterpolator = LinearInterpolateType<VectorImageType>::New();
  warpInterpolator->SetInputImage(warp);

  auto pointCloudLineIterator = vtk::TakeSmartPointer(this->m_pointCloudLines->NewIterator());
  pointCloudLineIterator->GoToFirstCell();

  vtkNew<vtkIdList> linePointIds;
  linePointIds->Allocate(2);

  while(!pointCloudLineIterator->IsDoneWithTraversal()) {
    linePointIds->Reset();
    pointCloudLineIterator->GetCurrentCell(linePointIds);
    unsigned int lineId = pointCloudLineIterator->GetCurrentCellId();

    double p0[nDims];
    this->m_pointCloud->GetPoint(linePointIds->GetId(0), p0);

    ContinuousIndexType cIdx = TransformNDimsDoubleToContinuousIndex<VectorImageType>(warp, p0);
    interpolationValues[lineId] = warpInterpolator->EvaluateAtContinuousIndex(cIdx);

    pointCloudLineIterator->GoToNextCell();
  }

  // Mask field to just the label region and get CSF blur mask
  VectorImageType::Pointer warpMasked = MaskImage<VectorImageType>(warp, this->m_labelMaskToApply);

  // Set field values within blur mask
  ImageRegionIteratorWithIndexType<UCharImageType>
    maskIterator(this->m_blurMaskToApply, this->m_blurMaskToApply->GetLargestPossibleRegion());
  maskIterator.GoToBegin();
  
  for(maskIterator.GoToBegin(); !maskIterator.IsAtEnd(); ++maskIterator) {
    if(maskIterator.Get() != 1) continue;

    auto index = maskIterator.GetIndex();
    const auto& indexMetaData = this->m_interpolationMetaData.at(index);

    VectorImageType::PixelType maskedValue;
    maskedValue.Fill(0.0);

    for(const auto& lineData : indexMetaData) {
      unsigned int lineId = std::get<0>(lineData);
      double weight = std::get<1>(lineData);
      double t = std::get<2>(lineData);
      
      VectorImageType::PixelType interpValue = interpolationValues.at(lineId);
      for(unsigned int d = 0; d < nDims; d++) {
	maskedValue[d] += interpValue[d] * abs(1 - t) * weight;
      }
    }
    warpMasked->SetPixel(index, maskedValue);
  }

  return warpMasked;
}
