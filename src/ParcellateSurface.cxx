#include "ParcellateSurface.h"


//--------------------------------------------------------------------------------------------------

template <typename TImage>
TPointer<TImage> MakeTargetLabelParcellation(TPointer<TImage> input,
                                             std::vector<TPixel<TImage>>& vLabels,
                                             TPixel<TImage> defaultPixelValue)
{
  // Make sure default pixel value is not a target label value
  if(IsInside<TImage>(defaultPixelValue, vLabels)) {
    defaultPixelValue = *std::max_element(vLabels.begin(), vLabels.end());
  }
  vLabels.emplace_back(defaultPixelValue);

  // Convert all pixels in image to defaultPixelValue (1) except the target labels
  TPointer<TImage> output = DuplicateImage<TImage>(input);
  ImageRegionIteratorWithIndexType<TImage> iterator(output, output->GetLargestPossibleRegion());
  iterator.GoToBegin();

  while(!iterator.IsAtEnd()) {
    const TPixel<TImage>& pixel = iterator.Get();
    if(pixel != 0 && !IsInside<TImage>(pixel, vLabels)) {
      iterator.Set(defaultPixelValue);
    }
    ++iterator;
  }
  return output;
}

template TPointer<IntImageType>
MakeTargetLabelParcellation<IntImageType>(TPointer<IntImageType> input,
					  std::vector<TPixel<IntImageType>>& vLabels,
					  TPixel<IntImageType> defaultPixelValue);


//
template <typename TImage>
TPixel<TImage> MajorityVote(const std::vector<TPixel<TImage>>& votes,
			    const std::vector<TPixel<TImage>>& vLabels)
{
  unsigned int nVotes = votes.size();
  std::vector<TPixel<TImage>> candidates;
  std::vector<unsigned int> count;

  unsigned int nCandidates = 0;
  count.resize(nVotes, 0);

  // Tally up all possible labels
  for(unsigned int i = 0; i < nVotes; i++) {
    TPixel<TImage> label = votes.at(i);
    if(!IsInside<TImage>(label, vLabels)) continue;

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
  TPixel<TImage> choice = 0;

  for(unsigned int i = 0; i < nCandidates; i++) {
    if(count.at(i) > maxVote) {
      maxVote = count.at(i);
      choice = candidates.at(i);
    }
  }
  return choice;
}

template TPixel<IntImageType>
MajorityVote<IntImageType>(const std::vector<TPixel<IntImageType>>& votes,
			   const std::vector<TPixel<IntImageType>>& vLabels);

template TPixel<FloatImageType>
MajorityVote<FloatImageType>(const std::vector<TPixel<FloatImageType>>& votes,
			     const std::vector<TPixel<FloatImageType>>& vLabels);

template TPixel<UCharImageType>
MajorityVote<UCharImageType>(const std::vector<TPixel<UCharImageType>>& votes,
			     const std::vector<TPixel<UCharImageType>>& vLabels);


//
template <typename TImage, typename TArray>
vtkSmartPointer<TArray> AssignLabels(vtkSmartPointer<vtkPolyData> surface,
				     TPointer<TImage> image,
				     const std::string& labelArrayName,
				     const std::vector<TPixel<TImage>>& vLabels,
				     bool isRAS)
{
  GeneratePolyDataNormals(surface);
  vtkSmartPointer<vtkDataArray> surfaceNormals = surface->GetPointData()->GetArray("Normals");
  const unsigned int& nPoints = surface->GetNumberOfPoints();
  
  vtkNew<TArray> labelsArray;
  labelsArray->SetNumberOfComponents(1);
  labelsArray->SetNumberOfValues(nPoints);
  labelsArray->SetName(labelArrayName.c_str());
  labelsArray->Fill(0.0);
  
  for(unsigned int p = 0; p < nPoints; p++) {
    double p0[nDims];
    surface->GetPoint(p, p0);

    TIndex<TImage> index = TransformNDimsDoubleToIndex<TImage>(image, p0, isRAS);
    
    if(image->GetLargestPossibleRegion().IsInside(index)) {
      TPixel<TImage> pixel = image->GetPixel(index);
      
      // Is this pixel a valid label?
      if(IsInside<TImage>(pixel, vLabels)) {
	labelsArray->SetValue(p, pixel);
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
	    const double dir = (isRAS) ? (rasShift[d] * normal[d]) : normal[d];
	    p1[d] -= step * dir;
	  }
	  
	  // Check again
	  index = TransformNDimsDoubleToIndex<TImage>(image, p1, isRAS);
	  if(!image->GetLargestPossibleRegion().IsInside(index)) continue;
	  
	  pixel = image->GetPixel(index);
	  if(IsInside<TImage>(pixel, vLabels)) {
	    labelsArray->SetValue(p, pixel);
	    break;
	  }
	}
      }
    }
    else {
      std::cout << "[AssignLabels():] not inside. huh.\n";
    }
  }

  return labelsArray;
}

template vtkSmartPointer<vtkIntArray>
AssignLabels<IntImageType, vtkIntArray>(vtkSmartPointer<vtkPolyData> surface,
					TPointer<IntImageType> image,
					const std::string& labelArrayName,
					const std::vector<TPixel<IntImageType>>& vLabels,
					bool isRAS);

template vtkSmartPointer<vtkFloatArray>
AssignLabels<FloatImageType, vtkFloatArray>(vtkSmartPointer<vtkPolyData> surface,
					    TPointer<FloatImageType> image,
					    const std::string& labelArrayName,
					    const std::vector<TPixel<FloatImageType>>& vLabels,
					    bool isRAS);

template vtkSmartPointer<vtkUnsignedCharArray>
AssignLabels<UCharImageType, vtkUnsignedCharArray>(vtkSmartPointer<vtkPolyData> surface,
						   TPointer<UCharImageType> image,
						   const std::string& labelArrayName,
						   const std::vector<TPixel<UCharImageType>>& vLabels,
						   bool isRAS);

//
template <typename TImage, typename TArray> unsigned int
FillHoles(vtkSmartPointer<vtkPolyData> surface,
	  vtkSmartPointer<TArray> labelsArray,
	  const std::vector<TPixel<TImage>>& vLabels,
	  bool isRAS)
{
  unsigned int nPoints = labelsArray->GetNumberOfTuples();
  unsigned int nFail = 0;  
  
  for(unsigned int p = 0; p < nPoints; p++) {
    // Get pixel value
    auto label = labelsArray->GetValue(p);
    if(IsInside<TImage>(label, vLabels)) continue;
    
    // Not a valid label... let's see if any one-hop neighbors have a valid label to steal
    std::vector<TPixel<TImage>> candidates;
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
	const auto candidate = labelsArray->GetValue(q);
	if(IsInside<TImage>(candidate, vLabels)) {
	  candidates.push_back(candidate);
	}
      }
    }

    // Now we vote on which candidate is the best    
    if(!candidates.empty()) {
      auto selected = MajorityVote<TImage>(candidates, vLabels);
      labelsArray->SetValue(p, selected);

      if(!IsInside<TImage>(selected, vLabels))
	nFail++;
    }
    else {
      nFail++;
    }
  }
  
  return nFail;
}

template unsigned int
FillHoles<IntImageType, vtkIntArray>(vtkSmartPointer<vtkPolyData> surface,
				     vtkSmartPointer<vtkIntArray> labelsArray,
				     const std::vector<TPixel<IntImageType>>& vLabels,
				     bool isRAS);

template unsigned int
FillHoles<FloatImageType, vtkFloatArray>(vtkSmartPointer<vtkPolyData> surface,
					 vtkSmartPointer<vtkFloatArray> labelsArray,
					 const std::vector<TPixel<FloatImageType>>& vLabels,
					 bool isRAS);

template unsigned int
FillHoles<UCharImageType, vtkUnsignedCharArray>(vtkSmartPointer<vtkPolyData> surface,
						vtkSmartPointer<vtkUnsignedCharArray> labelsArray,
						const std::vector<TPixel<UCharImageType>>& vLabels,
						bool isRAS);

//
template <typename TImage> void
ParcellateSurface(vtkSmartPointer<vtkPolyData> surface,
		  TPointer<TImage> image,
		  const std::string& labelArrayName,
		  const std::vector<TPixel<TImage>>& vLabels,
		  bool isRAS)
{
  using TArray = typename VTKArrayFromITKImage<TImage>::type;

  // Create array with surface labels
  vtkSmartPointer<TArray> labelsArray =
    AssignLabels<TImage, TArray>(surface, image, labelArrayName, vLabels, isRAS);

  // Fill holes
  unsigned int it = 0;
  unsigned int nHoles = 1;
  while(nHoles = FillHoles<TImage, TArray>(surface, labelsArray, vLabels, isRAS)
	&& it < 100) {
    it++;
  }

  // Attach labels to surface data
  surface->GetPointData()->AddArray(labelsArray);
  surface->BuildLinks();
}

template void
ParcellateSurface<IntImageType>(vtkSmartPointer<vtkPolyData> surface,
				TPointer<IntImageType> image,
				const std::string& labelArrayName,
				const std::vector<TPixel<IntImageType>>& vLabels,
				bool isRAS);

template void
ParcellateSurface<FloatImageType>(vtkSmartPointer<vtkPolyData> surface,
				  TPointer<FloatImageType> image,
				  const std::string& labelArrayName,
				  const std::vector<TPixel<FloatImageType>>& vLabels,
				  bool isRAS);

template void
ParcellateSurface<UCharImageType>(vtkSmartPointer<vtkPolyData> surface,
				  TPointer<UCharImageType> image,
				  const std::string& labelArrayName,
				  const std::vector<TPixel<UCharImageType>>& vLabels,
				  bool isRAS);
