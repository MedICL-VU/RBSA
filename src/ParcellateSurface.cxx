#include "ParcellateSurface.h"


//--------------------------------------------------------------------------------------------------

template <typename TImage>
typename TImage::Pointer MakeTargetLabelParcellation
(typename TImage::Pointer input, std::vector<typename TImage::PixelType>& validLabels,
 typename TImage::PixelType defaultPixelValue)
{
  // Make sure default pixel value is not a target label value
  if(IsInside<TImage>(defaultPixelValue, validLabels)) {
    defaultPixelValue = *std::max_element(validLabels.begin(), validLabels.end());
  }
  validLabels.emplace_back(defaultPixelValue);

  // Convert all pixels in image to defaultPixelValue (1) except the target labels
  typename TImage::Pointer output = DuplicateImage<TImage>(input);
  ImageRegionIteratorWithIndexType<TImage> iterator(output, output->GetLargestPossibleRegion());
  iterator.GoToBegin();

  while(!iterator.IsAtEnd()) {
    const typename TImage::PixelType& pixel = iterator.Get();
    if(pixel != 0 && !IsInside<TImage>(pixel, validLabels)) {
      iterator.Set(defaultPixelValue);
    }
    ++iterator;
  }
  return output;
}

template IntImageType::Pointer MakeTargetLabelParcellation<IntImageType>
(IntImageType::Pointer input, std::vector<IntImageType::PixelType>& validLabels,
 IntImageType::PixelType defaultPixelValue);


//
template <typename TImage>
typename TImage::PixelType MajorityVote
(const std::vector<typename TImage::PixelType>& votes,
 const std::vector<typename TImage::PixelType>& validLabels)
{
  unsigned int nVotes = votes.size();
  std::vector<typename TImage::PixelType> candidates;
  std::vector<unsigned int> count;

  unsigned int nCandidates = 0;
  count.resize(nVotes, 0);

  // Tally up all possible labels
  for(unsigned int i = 0; i < nVotes; i++) {
    typename TImage::PixelType label = votes.at(i);
    if(!IsInside<TImage>(label, validLabels)) continue;

    bool found = false;
    for(unsigned int j = 0; j < nCandidates; j++) {
      if(label == candidates.at(j)) {
        found = true;
        count.at(j)++;
      }
    }
    if(!found) {
      candidates.push_back(label);
      count.at(nCandidates)++;
      nCandidates++;
    }
  }

  // Select most likely
  unsigned int maxVote = 0;
  typename TImage::PixelType choice = 0;

  for(unsigned int i = 0; i < nCandidates; i++) {
    if(count.at(i) > maxVote) {
      maxVote = count.at(i);
      choice = candidates.at(i);
    }
  }
  return choice;
}

template IntImageType::PixelType MajorityVote<IntImageType>
(const std::vector<IntImageType::PixelType>& votes,
 const std::vector<IntImageType::PixelType>& validLabels);

template FloatImageType::PixelType MajorityVote<FloatImageType>
(const std::vector<FloatImageType::PixelType>& votes,
 const std::vector<FloatImageType::PixelType>& validLabels);

template UCharImageType::PixelType MajorityVote<UCharImageType>
(const std::vector<UCharImageType::PixelType>& votes,
 const std::vector<UCharImageType::PixelType>& validLabels);


//

template <typename TImage, typename TArray>
vtkSmartPointer<TArray> AssignLabels
(vtkSmartPointer<vtkPolyData> surface, typename TImage::Pointer image,
 const std::string& labelArrayName, const std::vector<typename TImage::PixelType>& validLabels,
 bool convertFromRAS)
{
  unsigned int nPoints = surface->GetNumberOfPoints();

  GeneratePolyDataNormals(surface);
  vtkSmartPointer<vtkDataArray> surfaceNormals = surface->GetPointData()->GetArray("Normals");

  vtkNew<TArray> labels;
  labels->SetNumberOfComponents(1);
  labels->SetNumberOfValues(nPoints);
  labels->SetName(labelArrayName.c_str());
  labels->Fill(0.0);
  
  for(unsigned int p = 0; p < nPoints; p++) {
    double p0[nDims];
    surface->GetPoint(p, p0);

    typename TImage::IndexType index =
      TransformNDimsDoubleToIndex<TImage>(image, p0, convertFromRAS);
    
    if(image->GetLargestPossibleRegion().IsInside(index)) {
      typename TImage::PixelType pixel = image->GetPixel(index);
      
      // Is this pixel a valid label?
      if(IsInside<TImage>(pixel, validLabels)) {
	labels->SetValue(p, pixel);
      }
      else {
	// Nope... travel inside along normal to find pixel (surface probably overestimated)
	double normal[nDims];
	surfaceNormals->GetTuple(p, normal);

	const auto& spacing = image->GetSpacing();
	const double step = std::min({spacing[0], spacing[1], spacing[2]}); 

	double p1[nDims] = {p0[0], p0[1], p0[2]};
	
	for(unsigned int n = 0; n < 3; n++) {
	  // Travel inwards by approx. 1 voxel
	  for(unsigned int d = 0; d < nDims; d++) {
	    const double dir = (convertFromRAS) ? (rasShift[d] * normal[d]) : normal[d];
	    p1[d] -= step * dir;
	  }
	  
	  // Check again
	  index = TransformNDimsDoubleToIndex<TImage>(image, p1, convertFromRAS);
	  if(!image->GetLargestPossibleRegion().IsInside(index)) continue;
	  
	  pixel = image->GetPixel(index);
	  if(IsInside<TImage>(pixel, validLabels)) {
	    labels->SetValue(p, pixel);
	    break;
	  }
	}
      }
    }
    else {
      std::cout << "[AssignLabels():] not inside. huh.\n";
    }
  }

  return labels;
}

template vtkSmartPointer<vtkIntArray> AssignLabels<IntImageType, vtkIntArray>
(vtkSmartPointer<vtkPolyData> surface, IntImageType::Pointer image,
 const std::string& labelArrayName, const std::vector<IntImageType::PixelType>& validLabels,
 bool convertFromRAS);

template vtkSmartPointer<vtkFloatArray> AssignLabels<FloatImageType, vtkFloatArray>
(vtkSmartPointer<vtkPolyData> surface, FloatImageType::Pointer image,
 const std::string& labelArrayName, const std::vector<FloatImageType::PixelType>& validLabels,
 bool convertFromRAS);

template vtkSmartPointer<vtkUnsignedCharArray> AssignLabels<UCharImageType, vtkUnsignedCharArray>
(vtkSmartPointer<vtkPolyData> surface, UCharImageType::Pointer image,
 const std::string& labelArrayName, const std::vector<UCharImageType::PixelType>& validLabels,
 bool convertFromRAS);



//

template <typename TImage, typename TArray>
unsigned int FillHoles
(vtkSmartPointer<vtkPolyData> surface, vtkSmartPointer<TArray> labels,
 const std::vector<typename TImage::PixelType>& validLabels, bool convertFromRAS)
{
  unsigned int nPoints = labels->GetNumberOfTuples();
  unsigned int nFail = 0;  
  
  for(unsigned int p = 0; p < nPoints; p++) {
    // Get pixel value
    auto label = labels->GetValue(p);
    if(IsInside<TImage>(label, validLabels)) continue;
    
    // Not a valid label... let's see if any one-hop neighbors have a valid label to steal
    std::vector<typename TImage::PixelType> candidates;
    vtkNew<vtkIdList> visitedNeighbors;
    
    vtkNew<vtkIdList> pointCellIds;
    surface->GetPointCells(p, pointCellIds);

    for(unsigned int i = 0; i < pointCellIds->GetNumberOfIds(); i++) {
      vtkNew<vtkIdList> cellPointIds;
      surface->GetCellPoints(pointCellIds->GetId(i), cellPointIds);
      
      for(unsigned j = 0; j < cellPointIds->GetNumberOfIds(); j++) {
	unsigned int q = cellPointIds->GetId(j);
	if(q == p || visitedNeighbors->IsId(q) != -1) continue;

	// New neighbor -> is it a candidate?
	visitedNeighbors->InsertNextId(q);
	const auto candidate = labels->GetValue(q);
	if(IsInside<TImage>(candidate, validLabels)) {
	  candidates.push_back(candidate);
	}
      }
    }

    // Now we vote on which candidate is the best    
    if(!candidates.empty()) {
      auto selected = MajorityVote<TImage>(candidates, validLabels);
      labels->SetValue(p, selected);

      if(!IsInside<TImage>(selected, validLabels))
	nFail++;
    }
    else {
      nFail++;
    }
  }
  
  return nFail;
}

template unsigned int FillHoles<IntImageType, vtkIntArray>
(vtkSmartPointer<vtkPolyData> surface, vtkSmartPointer<vtkIntArray> labels,
 const std::vector<IntImageType::PixelType>& validLabels, bool convertFromRAS);

template unsigned int FillHoles<FloatImageType, vtkFloatArray>
(vtkSmartPointer<vtkPolyData> surface, vtkSmartPointer<vtkFloatArray> labels,
 const std::vector<FloatImageType::PixelType>& validLabels, bool convertFromRAS);

template unsigned int FillHoles<UCharImageType, vtkUnsignedCharArray>
(vtkSmartPointer<vtkPolyData> surface, vtkSmartPointer<vtkUnsignedCharArray> labels,
 const std::vector<UCharImageType::PixelType>& validLabels, bool convertFromRAS);


//

template <typename TImage>
void ParcellateSurface
(vtkSmartPointer<vtkPolyData> surface, typename TImage::Pointer image,
 const std::string& labelArrayName, const std::vector<typename TImage::PixelType>& validLabels,
 bool convertFromRAS)
{
  using TArray = typename VTKArrayFromITKImage<TImage>::type;

  // Create array with surface labels
  vtkSmartPointer<TArray> labels =
    AssignLabels<TImage, TArray>(surface, image, labelArrayName, validLabels, convertFromRAS);

  // Fill holes
  unsigned int it = 0;
  unsigned int nHoles = 1;
  while(nHoles = FillHoles<TImage, TArray>(surface, labels, validLabels, convertFromRAS)
	&& it < 100) {
    it++;
  }

  // Attach labels to surface data
  surface->GetPointData()->AddArray(labels);
  surface->BuildLinks();
}

template void ParcellateSurface<IntImageType>
(vtkSmartPointer<vtkPolyData> surface, IntImageType::Pointer, const std::string& labelArrayName,
 const std::vector<IntImageType::PixelType>& validLabels, bool convertFromRAS);

template void ParcellateSurface<FloatImageType>
(vtkSmartPointer<vtkPolyData> surface, FloatImageType::Pointer, const std::string& labelArrayName,
 const std::vector<FloatImageType::PixelType>& validLabels, bool convertFromRAS);

template void ParcellateSurface<UCharImageType>
(vtkSmartPointer<vtkPolyData> surface, UCharImageType::Pointer, const std::string& labelArrayName,
 const std::vector<UCharImageType::PixelType>& validLabels, bool convertFromRAS);
