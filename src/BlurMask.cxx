#include "BlurMask.h"
#include "ParcellateSurface.h"
#include "utils.h"


//----------------------------------------------------------------------------------------------------

vtkSmartPointer<vtkPolyData> CreateLabelMesh(TPointer<UCharImageType> labelMask,
					     TPointer<FloatImageType> distanceMap)
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
  TIndex<UCharImageType> idx;

  auto interpolator = LinearInterpolateType<FloatImageType>::New();
  interpolator->SetInputImage(distanceMap);

  for(unsigned int p = 0; p < mesh->GetNumberOfPoints(); p++) {
    double p0[nDims], p1[nDims];
    mesh->GetPoint(p, p0);
        
    ContinuousIndexType cIdx =
      TransformNDimsDoubleToContinuousIndex<FloatImageType>(distanceMap, p0);
    TIndex<FloatImageType> idx = TransformNDimsDoubleToIndex<FloatImageType>(distanceMap, p0);

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


TPointer<UCharImageType> LabelCSFEdgeMap(TPointer<UCharImageType> labelMask,
					 TPointer<UCharImageType> brainMask)
{
  // Get contour map of brainMask
  TPointer<UCharImageType> fullEdgeMap;
  {
    auto filter = BinaryContourImageFilterType::New();
    filter->SetInput(brainMask);
    filter->SetForegroundValue(0);
    filter->SetBackgroundValue(1);
    filter->Update();
    
    fullEdgeMap = filter->GetOutput();
    BinaryThresholdImageInPlace(fullEdgeMap, 0, 0);
  }
  
  // Isolate voxels of fullEdgeMap that touch labelMask
  BinaryBallStructuringElementType kernel;
  kernel.SetRadius(kernelRadius);
  kernel.CreateStructuringElement();

  TPointer<UCharImageType> mask1 = DilateImage(fullEdgeMap, kernel);
  MultiplyImagesInPlace<UCharImageType>(mask1, labelMask);

  // Isolate voxels of labelMask that touch fullEdgeMap
  TPointer<UCharImageType> mask2 = DilateImage(labelMask, kernel);
  MultiplyImagesInPlace<UCharImageType>(mask2, fullEdgeMap);

  // Combine and return
  TPointer<UCharImageType> labelEdgeMask = AddImages<UCharImageType>(mask1, mask2);
  return labelEdgeMask;
}
  


/*
  Class specific
*/

BlurMaskGenerator::BlurMaskGenerator()
{
  this->m_stepSize = 0.5;

  // Initialize/reset vtk data
  if(this->m_pointCloud) {
    this->m_pointCloud->Initialize();
  }
  else {
    this->m_pointCloud = vtkSmartPointer<vtkPoints>::New();
  }

  if(this->m_pointCloudLines) {
    this->m_pointCloudLines->Initialize();
  }
  else {
    this->m_pointCloudLines = vtkSmartPointer<vtkCellArray>::New();
  }
}
  

TPointer<UCharImageType> BlurMaskGenerator::Generate(TPointer<UCharImageType> labelMask,
						     TPointer<UCharImageType> brainMask,
						     TPointer<UCharImageType> skullStripMask)
{
  // Get signed distance map of brainmask
  TPointer<FloatImageType> distanceMap;
  {
    auto filter = SignedMaurerDistanceMapImageFilterType::New();
    filter->SetInput(brainMask);
    filter->Update();

    distanceMap = filter->GetOutput();
    distanceMap->DisconnectPipeline();
  }

  // Get gradient of signed distance
  TPointer<VectorImageType> distanceGrad;
  {
    auto filter = GradientImageFilterType::New();
    filter->SetInput(distanceMap);
    filter->Update();
    
    distanceGrad = filter->GetOutput();
    distanceGrad->DisconnectPipeline();
  }

  // Initialize the blur mask w/ the CSF voxels bordering the target label mask
  TPointer<UCharImageType> blurMask = LabelCSFEdgeMap(labelMask, brainMask);
  
  // Convert edge map to vtkPolyData (to get a set of points and normals on edge)
  vtkSmartPointer<vtkPolyData> labelMesh = CreateLabelMesh(labelMask, distanceMap);
  vtkSmartPointer<vtkUnsignedCharArray> isEdge =
    vtkUnsignedCharArray::SafeDownCast(labelMesh->GetPointData()->GetArray("Is edge"));
  vtkSmartPointer<vtkDataArray> normals = labelMesh->GetPointData()->GetNormals();

  // Initialize pointCloud (points) and pointCloudLines (cell array)
  unsigned int nEdgePoints = 0;
  for(unsigned int p = 0; p < labelMesh->GetNumberOfPoints(); p++) {
    if(isEdge->GetValue(p) > 0) {
      nEdgePoints++;
    }
  }
  this->m_pointCloud->Allocate(nEdgePoints);
  this->m_pointCloudLines->AllocateEstimate(nEdgePoints, 2);

  // Set up iterators
  auto brainMaskInterpolator = LinearInterpolateType<UCharImageType>::New();
  brainMaskInterpolator->SetInputImage(brainMask);

  auto distanceMapInterpolator = LinearInterpolateType<FloatImageType>::New();
  distanceMapInterpolator->SetInputImage(distanceMap);

  auto distanceGradInterpolator = LinearInterpolateType<VectorImageType>::New();
  distanceGradInterpolator->SetInputImage(distanceGrad);

  auto skullStripMaskInterpolator = LinearInterpolateType<UCharImageType>::New();
  skullStripMaskInterpolator->SetInputImage(skullStripMask);

  // Fill the CSF around the label w/ an ordered point cloud
  unsigned int pointId = 0, lineId = 0;
  
  for(unsigned int p = 0; p < labelMesh->GetNumberOfPoints(); p++) {
    if(isEdge->GetValue(p) == 1) {
      // Initialize
      double x0[nDims], direction[nDims], xt[nDims], x1[nDims];
      labelMesh->GetPoint(p, x0);
      labelMesh->GetPoint(p, xt);
      normals->GetTuple(p, direction);
      
      ContinuousIndexType cIdxt =
	TransformNDimsDoubleToContinuousIndex<UCharImageType>(blurMask, xt);
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
	  TransformNDimsDoubleToContinuousIndex<UCharImageType>(blurMask, x1);
        float Dx1 = distanceMapInterpolator->EvaluateAtContinuousIndex(cIdx1);

	// Are we too close to adjacent tissue?
	if(Dxt >= Dx1) {
	  atMedialBoundary = true;
	  continue;
	}

	// Still inside skull strip mask?
	if(!skullStripMask->GetLargestPossibleRegion().IsInside(cIdx1)
	   || skullStripMaskInterpolator->EvaluateAtContinuousIndex(cIdx1) < 0.5) {
	  insideSkullStrip = false;
	  continue;
	}

	// Update mask
	TIndex<UCharImageType> idx =
	  TransformNDimsDoubleToIndex<UCharImageType>(blurMask, x1);
	blurMask->SetPixel(idx, 1);

	// Reset for next iteration
	for(unsigned int d = 0; d < nDims; d++) {
	  xt[d] = x1[d];
	  cIdxt[d] = cIdx1[d];
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
  MultiplyImagesInPlace<UCharImageType>(blurMask, skullStripMask);
  AddImagesInPlace<UCharImageType>(blurMask, labelMask);
  BinaryThresholdImageInPlace(blurMask);
  
  return blurMask;
}


// Apply blur mask to warp
TPointer<VectorImageType> BlurMaskGenerator::ApplyToWarp(TPointer<VectorImageType> warp,
							 TPointer<UCharImageType> blurMask,
							 TPointer<UCharImageType> labelMask)
{
  // Take only CSF voxels of blur mask
  TPointer<UCharImageType> CSFMask = SubtractImages<UCharImageType>(blurMask, labelMask);
  TRegion<UCharImageType> CSFMaskRegion = CSFMask->GetLargestPossibleRegion();
    
  // Iterate over point cloud lines to get endpoint coordinates and warp value at p0
  const unsigned int& nLines = this->m_pointCloudLines->GetNumberOfCells();

  std::vector<std::array<double, nDims>> linePoint0(nLines), linePoint1(nLines);
  std::vector<TPixel<VectorImageType>> warpValuesAtP0s(nLines);
  {
    auto interpolator = LinearInterpolateType<VectorImageType>::New();
    interpolator->SetInputImage(warp);

    auto iterator = vtk::TakeSmartPointer(this->m_pointCloudLines->NewIterator());
    vtkNew<vtkIdList> linePointIds;
    linePointIds->Allocate(2);

    for(iterator->GoToFirstCell(); !iterator->IsDoneWithTraversal(); iterator->GoToNextCell())
      {
        linePointIds->Reset();
        iterator->GetCurrentCell(linePointIds);

	// Cache endpoints of line
        unsigned int lineId = iterator->GetCurrentCellId();
        this->m_pointCloud->GetPoint(linePointIds->GetId(0), linePoint0[lineId].data());
        this->m_pointCloud->GetPoint(linePointIds->GetId(1), linePoint1[lineId].data());

	// Get field value at p0
	ContinuousIndexType cIdx =
	  TransformNDimsDoubleToContinuousIndex<VectorImageType>(warp, linePoint0[lineId].data());
	warpValuesAtP0s.at(lineId) = interpolator->EvaluateAtContinuousIndex(cIdx);
      }
  }
  
  // Set each pixel value in masked warp
  TPointer<VectorImageType> warpMasked = MaskImage<VectorImageType>(warp, labelMask);
  {
    ImageRegionIteratorWithIndexType<UCharImageType> iterator(CSFMask, CSFMaskRegion);
    iterator.GoToBegin();
    
    for(iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator) {
      if(iterator.Get() != 1) continue;

      // Get index as phyiscal point
      const TIndex<UCharImageType>& index = iterator.GetIndex();
      double indexPoint[nDims];
      TransformIndexToNDimsDouble<UCharImageType>(labelMask, index, indexPoint);
      
      // Check all lines and keep the best N
      ClosestNLines<nInterpolationPoints> closestLines;
      
      for(unsigned int lineId = 0; lineId < nLines; lineId++) {
	double t;
	const double* p0 = linePoint0.at(lineId).data();
	const double* p1 = linePoint1.at(lineId).data();
	const double dist2 = vtkLine::DistanceToLine(indexPoint, p0, p1, t, nullptr);
	
	closestLines.consider(lineId, static_cast<float>(dist2), static_cast<float>(t));
      }

      // Calculate interpolation weights corresponding to each line
      const unsigned int nInterpolationLines = std::min(closestLines.N, nInterpolationPoints);
      std::vector<float> weights(nInterpolationLines);
      float w_norm = 0.f;
      
      for(unsigned int i = 0; i < nInterpolationLines; i++) {
	const float w = 1.f / (eps + std::sqrt(closestLines.a[i].dist2));	
	weights.at(i) = w * std::abs(1 - closestLines.a[i].t);
	w_norm += w;
      }

      // Get new pixel value 
      TPixel<VectorImageType> v;
      v.Fill(0.0);

      for(unsigned int i = 0; i < nInterpolationLines; i++) {
	const TPixel<VectorImageType>& v_i = warpValuesAtP0s.at(closestLines.a[i].lineId);
	const float w = weights.at(i) / w_norm;

	for(unsigned int d = 0; d < nDims; d++) {  
	  v[d] += v_i[d] * w;
	}
      }
      warpMasked->SetPixel(index, v);
    }
  }

  warpMasked->DisconnectPipeline();
  return warpMasked;
}
