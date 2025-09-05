#include "utils.h"


/*
  --------------------------
  --- Basic vector stuff ---
  --------------------------
*/

// Is pixel value in vector of pixel values?
template <typename TImage>
bool IsInside(const TPixel<TImage>& label, const std::vector<TPixel<TImage>>& valid)
{
  if(valid.empty() || std::find(valid.begin(), valid.end(), label) == valid.end()) {
    return false;
  }
  return true;
}

template bool
IsInside<IntImageType>(const TPixel<IntImageType>& label,
		       const std::vector<TPixel<IntImageType>>& valid);

template bool
IsInside<FloatImageType>(const TPixel<FloatImageType>& label,
			 const std::vector<TPixel<FloatImageType>>& valid);

template bool
IsInside<UCharImageType>(const TPixel<UCharImageType>& label,
			 const std::vector<TPixel<UCharImageType>>& valid);


/*
  ----------
  --- IO ---
  ----------
*/

// String manipulation for file extensions
std::vector<std::string> SplitString(std::string str, const std::string& delimiter)
{
  std::vector<std::string> tokens;
  size_t pos = 0;
  std::string token;
  
  while ((pos = str.find(delimiter)) != std::string::npos) {
    token = str.substr(0, pos);
    tokens.push_back(token);
    str.erase(0, pos + delimiter.length());
  }
  tokens.push_back(str);
  
  return tokens;
}


std::string GetFileExtension(std::string filename)
{
  std::vector<std::string> str_vec = SplitString(filename, ".");
  int nstrs = static_cast<int>(str_vec.size());

  std::string ext;
  if(nstrs == 0) {
    std::cerr << filename << " has no extension :(" << std::endl;
  }
  else if(str_vec[nstrs - 1].compare("gz") == 0 && str_vec[nstrs - 1].compare("nii") == 0) {
    ext = "nii.gz";
  }
  else {
    ext = str_vec.back();
  }

  return ext;
}


std::string GetBaseName(const std::string& filename)
{
  std::string basename = filename.substr(filename.find_last_of("/") + 1);
  std::string::size_type const p(basename.find_last_of("."));
  return basename.substr(0, p);
}

    
bool CheckFileExtension(std::string filename, std::vector<std::string> exts)
{
  std::string f_ext = GetFileExtension(filename);
  
  for(const auto& ext : exts) {
    if(f_ext.compare(ext) == 0) {
      return true;
    }
  }
  return false;
}


// ITK reader
template <typename TImage>
TPointer<TImage> ReadImage(const std::string& filename)
{
  auto reader = ImageReaderType<TImage>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();
  
  return reader->GetOutput();
}

template TPointer<FloatImageType>
ReadImage<FloatImageType>(const std::string&);

template TPointer<IntImageType>
ReadImage<IntImageType>(const std::string&);

template TPointer<VectorImageType>
ReadImage<VectorImageType>(const std::string&);

template TPointer<UCharImageType>
ReadImage<UCharImageType>(const std::string&);


// ITK writer
template <typename TImage> void WriteImage
(TPointer<TImage> image, const std::string& filename)
{
  auto writer = ImageWriterType<TImage>::New();
  writer->SetInput(image);
  writer->SetFileName(filename);
  writer->Update();
}

template void
WriteImage<FloatImageType>(TPointer<FloatImageType> image, const std::string& filename);

template void
WriteImage<IntImageType>(TPointer<IntImageType> image, const std::string& filename);

template void
WriteImage<VectorImageType>(TPointer<VectorImageType> image, const std::string& filename);

template void
WriteImage<UCharImageType>(TPointer<UCharImageType> image, const std::string& filename);


// Polydata reader
vtkSmartPointer<vtkPolyData> ReadPolyData(const std::string& filename)
{
  const std::string& extension = GetFileExtension(filename);
  auto surface = vtkSmartPointer<vtkPolyData>::New();
  
  if(extension.compare("vtp") == 0) {
    vtkNew<vtkXMLPolyDataReader> reader;
    reader->SetFileName(filename.c_str());
    reader->Update();
    
    surface->DeepCopy(reader->GetOutput());
  }
  else if(extension.compare("vtk") == 0) {
    vtkNew<vtkPolyDataReader> reader;
    reader->SetFileName(filename.c_str());
    
    vtkNew<vtkTriangleFilter> triangleFilter;
    triangleFilter->SetInputConnection(reader->GetOutputPort());
    triangleFilter->Update();
    
    surface->DeepCopy(triangleFilter->GetOutput());
  }
  
  return surface;
}


// Polydata writer
void WritePolyData(vtkSmartPointer<vtkPolyData> polyData, const std::string& filename)
{
  const std::string& extension = GetFileExtension(filename);

  if(extension.compare("vtp") == 0) {
    vtkNew<vtkXMLPolyDataWriter> writer;
    writer->SetInputData(polyData);
    writer->SetFileName(filename.c_str());
    writer->Write();
  }
  else if(extension.compare("vtk") == 0) {
    vtkNew<vtkPolyDataWriter> writer;
    writer->SetInputData(polyData);
    writer->SetFileName(filename.c_str());
    writer->Write();
  }
}


/*
  ---------------------------
  --- Image manipulations ---
  --------------------------- 
*/

// Addition
template <typename TInImage1, typename TInImage2, typename TOutImage>
TPointer<TOutImage> AddImages(TPointer<TInImage1> image1, TPointer<TInImage2> image2)
{
  auto filter = AddImageFilterType<TInImage1, TInImage2, TOutImage>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();

  TPointer<TOutImage> output = filter->GetOutput();
  output->DisconnectPipeline();
  return output;
}

template <typename TImage>
TPointer<TImage> AddImages(TPointer<TImage> image1, TPointer<TImage> image2)
{
  TPointer<TImage> output = AddImages<TImage, TImage, TImage>(image1, image2);
  return output;
}

template TPointer<UCharImageType>
AddImages<UCharImageType>(TPointer<UCharImageType> image1, TPointer<UCharImageType> image2);


template <typename TImage1, typename TImage2>
void AddImagesInPlace(TPointer<TImage1>& image1, TPointer<TImage2> image2)
{
  auto filter = AddImageFilterType<TImage1, TImage2, TImage1>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->InPlaceOn();
  filter->Update();

  image1 = filter->GetOutput();
  image1->DisconnectPipeline();
}

template <typename TImage>
void AddImagesInPlace(TPointer<TImage>& image1, TPointer<TImage> image2)
{
  AddImagesInPlace<TImage, TImage>(image1, image2);
}

template void
AddImagesInPlace<IntImageType, UCharImageType>(TPointer<IntImageType>& image1,
					       TPointer<UCharImageType> image2);

template void
AddImagesInPlace<UCharImageType>(TPointer<UCharImageType>& image1,
				 TPointer<UCharImageType> image2);

template void
AddImagesInPlace<VectorImageType>(TPointer<VectorImageType>& image1,
				  TPointer<VectorImageType> image2);


// Binary threshold
template <typename TInImage>
TPointer<UCharImageType> BinaryThresholdImage(TPointer<TInImage> image,
					      const TPixel<TInImage>& lower,
					      const TPixel<TInImage>& upper,
					      const TPixel<UCharImageType>& outside,
					      const TPixel<UCharImageType>& inside)
{
  auto filter = BinaryThresholdImageFilterType<TInImage>::New();
  filter->SetInput(image);
  filter->SetLowerThreshold(lower);
  filter->SetUpperThreshold(upper);
  filter->SetOutsideValue(outside);
  filter->SetInsideValue(inside);
  filter->Update();
  
  TPointer<UCharImageType> output = filter->GetOutput();
  output->DisconnectPipeline();
  return output;
}

template TPointer<UCharImageType>
BinaryThresholdImage<IntImageType>(TPointer<IntImageType> image,
				   const TPixel<IntImageType>& lower,
				   const TPixel<IntImageType>& upper,
				   const TPixel<UCharImageType>& outside,
				   const TPixel<UCharImageType>& inside);

template TPointer<UCharImageType>
BinaryThresholdImage<UCharImageType>(TPointer<UCharImageType> image,
				     const TPixel<UCharImageType>& lower,
				     const TPixel<UCharImageType>& upper,
				     const TPixel<UCharImageType>& outside,
				     const TPixel<UCharImageType>& inside);

template TPointer<UCharImageType>
BinaryThresholdImage<FloatImageType>(TPointer<FloatImageType> image,
				     const TPixel<FloatImageType>& lower,
				     const TPixel<FloatImageType>& upper,
				     const TPixel<UCharImageType>& outside,
				     const TPixel<UCharImageType>& inside);


template <typename TInImage>
TPointer<UCharImageType> BinaryThresholdImage(TPointer<TInImage> image,
					      const TPixel<TInImage>& lower,
					      const TPixel<TInImage>& upper)
{
  return BinaryThresholdImage<TInImage>(image, lower, upper, 0, 1);
}

template TPointer<UCharImageType>
BinaryThresholdImage<IntImageType>(TPointer<IntImageType> image,
				   const TPixel<IntImageType>& lower,
				   const TPixel<IntImageType>& upper);

template TPointer<UCharImageType>
BinaryThresholdImage<UCharImageType>(TPointer<UCharImageType> image,
				     const TPixel<UCharImageType>& lower,
				     const TPixel<UCharImageType>& upper);


template <typename TInImage>
TPointer<UCharImageType> BinaryThresholdImage(typename TInImage::Pointer image)
{
  TPixel<TInImage> lower = static_cast<TPixel<TInImage>>(std::ceil(eps));
  TPixel<TInImage> upper = std::numeric_limits<TPixel<TInImage>>::max();
  return BinaryThresholdImage<TInImage>(image, lower, upper, 0, 1);
}

template TPointer<UCharImageType>
BinaryThresholdImage<IntImageType>(TPointer<IntImageType> image);

template TPointer<UCharImageType>
BinaryThresholdImage<UCharImageType>(TPointer<UCharImageType> image);


TPointer<UCharImageType> BinaryThresholdVectorImage(TPointer<VectorImageType> image)
{
  auto filter = VectorMagnitudeImageFilterType::New();
  filter->SetInput(image);
  filter->Update();
  
  return BinaryThresholdImage<FloatImageType>(filter->GetOutput());
}


// Binary threshold image (in place)
void BinaryThresholdImageInPlace(TPointer<UCharImageType>& image,
				 const TPixel<UCharImageType>& lower,
				 const TPixel<UCharImageType>& upper,
				 const TPixel<UCharImageType>& outside,
				 const TPixel<UCharImageType>& inside)
{
  auto filter = BinaryThresholdImageFilterType<UCharImageType>::New();
  filter->SetInput(image);
  filter->SetLowerThreshold(lower);
  filter->SetUpperThreshold(upper);
  filter->SetOutsideValue(outside);
  filter->SetInsideValue(inside);
  filter->InPlaceOn();
  filter->Update();

  image = filter->GetOutput();
  image->DisconnectPipeline();
}


void BinaryThresholdImageInPlace(TPointer<UCharImageType>& image,
				 const TPixel<UCharImageType>& lower,
				 const TPixel<UCharImageType>& upper)
{
  BinaryThresholdImageInPlace(image, lower, upper, 0, 1);
}


void BinaryThresholdImageInPlace(TPointer<UCharImageType>& image)
{
  TPixel<UCharImageType> lower = static_cast<TPixel<UCharImageType>>(std::ceil(eps));
  TPixel<UCharImageType> upper = std::numeric_limits<TPixel<UCharImageType>>::max();
  BinaryThresholdImageInPlace(image, lower, upper, 0, 1);
}


// Cast image
template <typename TInImage, typename TOutImage>
typename TOutImage::Pointer CastImage(typename TInImage::Pointer image)
{
  auto filter = CastImageFilterType<TInImage, TOutImage>::New();
  filter->SetInput(image);
  filter->Update();
  
  typename TOutImage::Pointer output = filter->GetOutput();
  output->DisconnectPipeline();
  return output;
}

template TPointer<FloatImageType>
CastImage<UCharImageType, FloatImageType>(TPointer<UCharImageType> image);


// Duplicate image
template <typename TImage>
TPointer<TImage> DuplicateImage(TPointer<TImage> image)
{
  auto duplicator = DuplicateImageFilterType<TImage>::New();
  duplicator->SetInputImage(image);
  duplicator->Update();

  return duplicator->GetModifiableOutput();
}

template TPointer<IntImageType>
DuplicateImage<IntImageType>(TPointer<IntImageType> image);

template TPointer<VectorImageType>
DuplicateImage<VectorImageType>(TPointer<VectorImageType> image);

template TPointer<UCharImageType>
DuplicateImage<UCharImageType>(TPointer<UCharImageType> image);


// Create zero-filled image
template <typename TInImage, typename TOutImage>
TPointer<TOutImage> InitializeZeroFilledImage(TPointer<TInImage> ref)
{
  TPointer<TOutImage> image = TOutImage::New();
  image->SetRegions(ref->GetLargestPossibleRegion());
  image->SetDirection(ref->GetDirection());
  image->SetOrigin(ref->GetOrigin());
  image->SetSpacing(ref->GetSpacing());
  image->Allocate();
  image->FillBuffer(itk::NumericTraits<TPixel<TOutImage>>::ZeroValue());

  image->DisconnectPipeline();
  return image;
}

template TPointer<IntImageType>
InitializeZeroFilledImage<IntImageType, IntImageType>(TPointer<IntImageType> image);

template TPointer<UCharImageType>
InitializeZeroFilledImage<IntImageType, UCharImageType>(TPointer<IntImageType> image);

template TPointer<VectorImageType>
InitializeZeroFilledImage<IntImageType, VectorImageType>(TPointer<IntImageType> image);

template TPointer<VectorImageType>
InitializeZeroFilledImage<UCharImageType, VectorImageType>(TPointer<UCharImageType> image);


// Check if label is in image
template <typename TImage>
bool IsLabelInImage(TPointer<TImage> image, const TPixel<TImage> label)
{
  ImageRegionConstIteratorType<TImage> iterator(image, image->GetLargestPossibleRegion());
  for(iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator) {
    if(iterator.Get() == label) return true;
    ++iterator;
  }
  return false;
}

template bool IsLabelInImage<IntImageType>
(TPointer<IntImageType> image, const TPixel<IntImageType> label);

template bool
IsLabelInImage<UCharImageType>(TPointer<UCharImageType> image, const TPixel<UCharImageType> label);


// Mask image
template <typename TImage>
TPointer<TImage> MaskImage(TPointer<TImage> image, TPointer<UCharImageType> mask)
{
  auto filter = MaskImageFilterType<TImage>::New();
  filter->SetInput1(image);
  filter->SetInput2(mask);
  filter->Update();
  
  TPointer<TImage> output = filter->GetOutput();
  output->DisconnectPipeline();
  return output;
}

template TPointer<VectorImageType>
MaskImage<VectorImageType>(TPointer<VectorImageType> image, TPointer<UCharImageType> mask);


// Multiplication
template <typename TInImage1, typename TInImage2, typename TOutImage>
TPointer<TOutImage> MultiplyImages(TPointer<TInImage1> image1, TPointer<TInImage2> image2)
{
  auto filter = MultiplyImageFilterType<TInImage1, TInImage2, TOutImage>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();

  typename TOutImage::Pointer output = filter->GetOutput();
  output->DisconnectPipeline();
  return output;
}

template <typename TImage>
TPointer<TImage> MultiplyImages(TPointer<TImage> image1, TPointer<TImage> image2)
{
  TPointer<TImage> output = MultiplyImages<TImage, TImage, TImage>(image1, image2);
  return output;
}

template TPointer<IntImageType>
MultiplyImages<IntImageType>(TPointer<IntImageType> image1, TPointer<IntImageType> image2);

template TPointer<UCharImageType>
MultiplyImages<UCharImageType>(TPointer<UCharImageType> image1, TPointer<UCharImageType> image2);


template <typename TImage1, typename TImage2>
void MultiplyImagesInPlace(TPointer<TImage1>& image1, TPointer<TImage2> image2)
{
  auto filter = MultiplyImageFilterType<TImage1, TImage2, TImage1>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->InPlaceOn();
  filter->Update();

  image1 = filter->GetOutput();
  image1->DisconnectPipeline();
}

template <typename TImage>
void MultiplyImagesInPlace(TPointer<TImage>& image1, TPointer<TImage> image2)
{
  MultiplyImagesInPlace<TImage, TImage>(image1, image2);
}

template void
MultiplyImagesInPlace<UCharImageType>(TPointer<UCharImageType>& image1,
				      TPointer<UCharImageType> image2);


// Subtraction
template <typename TInImage1, typename TInImage2, typename TOutImage>
TPointer<TOutImage> SubtractImages(TPointer<TInImage1> image1, TPointer<TInImage2> image2)
{
  auto filter = SubtractImageFilterType<TInImage1, TInImage2, TOutImage>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();
  
  TPointer<TOutImage> output = filter->GetOutput();
  output->DisconnectPipeline();
  return output;
}

template <typename TImage>
TPointer<TImage> SubtractImages(TPointer<TImage> image1, TPointer<TImage> image2)
{
  TPointer<TImage> output = SubtractImages<TImage, TImage, TImage>(image1, image2);
  return output;
}

template TPointer<UCharImageType>
SubtractImages<UCharImageType>(TPointer<UCharImageType> image1, TPointer<UCharImageType> image2);


template <typename TImage1, typename TImage2>
void SubtractImagesInPlace
(TPointer<TImage1>& image1, TPointer<TImage2> image2)
{
  auto filter = SubtractImageFilterType<TImage1, TImage2, TImage1>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->InPlaceOn();
  filter->Update();

  image1 = filter->GetOutput();
  image1->DisconnectPipeline();
}

template <typename TImage>
void SubtractImagesInPlace(TPointer<TImage>& image1, TPointer<TImage> image2)
{
  SubtractImagesInPlace<TImage, TImage>(image1, image2);
}

template void
SubtractImagesInPlace<UCharImageType>(TPointer<UCharImageType>& image1,
				      TPointer<UCharImageType> image2);


// Sum of image values
template <typename TImage>
float ImageSum(TPointer<TImage> image)
{
  auto filter = StatisticsImageFilterType<TImage>::New();
  filter->SetInput(image);
  filter->Update();

  return static_cast<float>(filter->GetSum());
}

template float
ImageSum<UCharImageType>(TPointer<UCharImageType> image);

template float
ImageSum<IntImageType>(TPointer<IntImageType> image);


// Image index functions
template <typename TImage>
ContinuousIndexType TransformNDimsDoubleToContinuousIndex(TPointer<TImage> image,
							  double p0[nDims],
							  bool isRAS)
{
  itk::Point<double, nDims> itkPoint;
  for(unsigned int d = 0; d < nDims; d++) {
    itkPoint[d] = (isRAS) ? rasShift[d] * p0[d] : p0[d];
  }

  ContinuousIndexType cIndex;
  bool isInside = image->TransformPhysicalPointToContinuousIndex(itkPoint, cIndex);
  if(!isInside) {
    std::cerr << "huh, index " << cIndex << " is not inside\n";
  }
  return cIndex;
}

template ContinuousIndexType
TransformNDimsDoubleToContinuousIndex<UCharImageType>(TPointer<UCharImageType> image,
						      double p0[nDims],
						      bool isRAS);

template ContinuousIndexType
TransformNDimsDoubleToContinuousIndex<FloatImageType>(TPointer<FloatImageType> image,
						      double p0[nDims],
						      bool isRAS);

template ContinuousIndexType
TransformNDimsDoubleToContinuousIndex<VectorImageType>(TPointer<VectorImageType> image,
						       double p0[nDims],
						       bool isRAS);


template <typename TImage>
TIndex<TImage> TransformNDimsDoubleToIndex(TPointer<TImage> image, double p0[nDims], bool isRAS)
{
  itk::Point<double, nDims> itkPoint;
  for(unsigned int d = 0; d < nDims; d++) {
    itkPoint[d] = (isRAS) ? rasShift[d] * p0[d] : p0[d];
  }
  
  TIndex<TImage> index;
  bool isInside = image->TransformPhysicalPointToIndex(itkPoint, index);
  return index;
}

template TIndex<IntImageType>
TransformNDimsDoubleToIndex<IntImageType>(TPointer<IntImageType> image,
					  double p0[nDims],
					  bool isRAS);

template TIndex<UCharImageType>
TransformNDimsDoubleToIndex<UCharImageType>(TPointer<UCharImageType> image,
					    double p0[nDims],
					    bool isRAS);

template TIndex<FloatImageType>
TransformNDimsDoubleToIndex<FloatImageType>(TPointer<FloatImageType> image,
					    double p0[nDims],
					    bool isRAS);


template <typename TImage>
void TransformIndexToNDimsDouble(TPointer<TImage> image, TIndex<TImage> index, double (&p0)[nDims])
{
  itk::Point<double, nDims> itkPoint;
  image->TransformIndexToPhysicalPoint(index, itkPoint);

  for(unsigned int d = 0; d < nDims; d++) {
    p0[d] = itkPoint[d];
  }
}

template void
TransformIndexToNDimsDouble<UCharImageType>(TPointer<UCharImageType> image,
					    TIndex<UCharImageType> index,
					    double (&p0)[nDims]);


/*
  ----------------------
  --- VTK data stuff ---
  ----------------------
*/

void GeneratePolyDataNormals(vtkSmartPointer<vtkPolyData> polydata, bool overwrite, bool reverse)
{
  // Check if normals exist
  const bool& hasPointNormals = polydata->GetPointData()->HasArray("Normals");
  const bool& hasCellNormals = polydata->GetCellData()->HasArray("Normals");
  
  const bool& needsPointNormals = (overwrite) ? true : !hasPointNormals;
  const bool& needsCellNormals = (overwrite) ? true : !hasCellNormals;
  
  if(needsPointNormals && hasPointNormals) {
    polydata->GetPointData()->RemoveArray("Normals");
  }
  if(needsCellNormals && hasCellNormals) {
    polydata->GetCellData()->RemoveArray("Normals");
  }
  
  // Generate normals if not already there
  if(needsPointNormals || needsCellNormals) {
    vtkNew<vtkPolyDataNormals> normalsFilter;
    normalsFilter->SetInputData(polydata);
    normalsFilter->SetComputePointNormals(needsPointNormals);
    normalsFilter->SetComputeCellNormals(needsCellNormals);
    if(reverse) normalsFilter->FlipNormalsOn();
    normalsFilter->Update();
    
    // Add normals to original polydata
    if(needsPointNormals) {
      polydata->GetPointData()->SetNormals
	(normalsFilter->GetOutput()->GetPointData()->GetNormals());
    }
    if(needsCellNormals) {
      polydata->GetCellData()->SetNormals
	(normalsFilter->GetOutput()->GetCellData()->GetNormals());
    }
    polydata->BuildLinks();
  }
}



template <typename TImage>
void TransformVTKPolyDataToITKImageSpace
(vtkSmartPointer<vtkPolyData> mesh, TPointer<TImage> image)
{
  // Get image info
  const auto& origin = image->GetOrigin();
  const auto& direction = image->GetDirection();
  
  // Edit points
  for(unsigned int p = 0; p < mesh->GetNumberOfPoints(); p++) {
    double vtkPoint[nDims];
    mesh->GetPoint(p, vtkPoint);

    double itkPoint[nDims];
    for(unsigned int i = 0; i < nDims; i++) {
      itkPoint[i] = 0;
      for(unsigned int j = 0; j < nDims; j++) {
        itkPoint[i] += (direction[i][j] * (vtkPoint[j] - origin[j]));
      }
      itkPoint[i] += origin[i];
    }
    mesh->GetPoints()->SetPoint(p, itkPoint);
  }
  mesh->BuildLinks();
}

template void TransformVTKPolyDataToITKImageSpace<UCharImageType>
(vtkSmartPointer<vtkPolyData> mesh, TPointer<UCharImageType> image);


void BinaryITKImageToVTKMesh
(TPointer<UCharImageType> itkTargetImage, vtkSmartPointer<vtkPolyData> outputMesh)
{
  // Convert itkTargetImage to vtkTargetImage (same info except direction matrix is identity)
  const auto& region = itkTargetImage->GetLargestPossibleRegion();
  const auto& size = region.GetSize();
  const auto& spacing = itkTargetImage->GetSpacing();
  const auto& origin = itkTargetImage->GetOrigin();

  auto vtkTargetImage = vtkSmartPointer<vtkImageData>::New();
  vtkTargetImage->SetDimensions(size[0], size[1], size[2]);
  vtkTargetImage->SetSpacing(spacing[0], spacing[1], spacing[2]);
  vtkTargetImage->SetOrigin(origin[0], origin[1], origin[2]);
  vtkTargetImage->SetExtent(0, size[0] - 1, 0, size[1] - 1, 0, size[2] - 1);
  vtkTargetImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  for(unsigned int k = 0; k < size[2]; k++) {
    for(unsigned int j = 0; j < size[1]; j++) {
      for(unsigned int i = 0; i < size[0]; i++) {
        const TIndex<UCharImageType>& index = {i, j, k};
        const TPixel<UCharImageType>& value = itkTargetImage->GetPixel(index);
        unsigned char* voxel =
          static_cast<unsigned char*>(vtkTargetImage->GetScalarPointer(i, j, k));
        *voxel = value;
      }
    }
  }
  vtkTargetImage->Modified();

  // Generate mesh from vtkImage
  vtkNew<vtkMarchingCubes> marchingCubesFilter;
  marchingCubesFilter->SetInputData(vtkTargetImage);
  marchingCubesFilter->ComputeScalarsOff();
  marchingCubesFilter->ComputeNormalsOff();
  marchingCubesFilter->ComputeGradientsOff();
  marchingCubesFilter->SetValue(0, 0.5);

  vtkNew<vtkTriangleFilter> triangleFilter;
  triangleFilter->SetInputConnection(marchingCubesFilter->GetOutputPort());
  triangleFilter->Update();

  // Transform mesh back to original itkTargetImage space
  vtkSmartPointer<vtkPolyData> temp = vtkSmartPointer<vtkPolyData>::New();
  temp->DeepCopy(triangleFilter->GetOutput());
  TransformVTKPolyDataToITKImageSpace<UCharImageType>(temp, itkTargetImage);

  // Compute normals
  vtkNew<vtkPolyDataNormals> normalsFilter;
  normalsFilter->SetInputData(temp);
  normalsFilter->ComputePointNormalsOn();
  normalsFilter->ComputeCellNormalsOff();
  normalsFilter->AutoOrientNormalsOn();

  vtkNew<vtkCleanPolyData> cleaner;
  cleaner->SetInputConnection(normalsFilter->GetOutputPort());
  cleaner->SetTolerance(1e-4);
  cleaner->PointMergingOn();
  cleaner->Update();

  // Deep copy to result to ouputMesh
  outputMesh->DeepCopy(cleaner->GetOutput());
}




/*
  -----------------------------
  --- Helpful for debugging ---
  -----------------------------
*/

void PrintNDimsDouble(double point[nDims])
{
  std::cout << "(" << point[0] << " " << point[1] << " " << point[2] << ")\n";
}


void PrintDuration(std::chrono::steady_clock::time_point t0, std::string text, std::string time_type)
{
  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
  
  if(time_type.compare("minutes") == 0) {
    std::stringstream m_str;
    m_str << std::fixed << std::setprecision(1)
	  << std::chrono::duration<float>((t1 - t0) / 60.0).count() << text << " ("
	  << m_str.str() << " min)\n";
  }
  else if(time_type.compare("seconds") == 0) {
    std::cout << text << " ("
	      << std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count() << " s)\n";
  }
  else if(time_type.compare("milliseconds") == 0) {
    std::cout << text << " (" <<
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms)\n";
  }
  else if(time_type.compare("microseconds") == 0) {
    std::cout << text << " (" <<
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() << " us)\n";
  }
  else if(time_type.compare("nanoseconds") == 0) {
    std::cout << text << " (" <<
      std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() << " ns)\n";
  }
  else {
    std::cout << "Must input valid time_type to print_duration()\n";
  }
}

void VisualizePointSet(vtkSmartPointer<vtkPoints> points,
		       const std::string& filename,
		       std::vector<vtkSmartPointer<vtkFloatArray>> pointDataFloatArrays)
{
  auto polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(points);
  polydata->BuildLinks();
  
  vtkNew<vtkVertexGlyphFilter> vertexGlyphFilter;
  vertexGlyphFilter->SetInputData(polydata);
  vertexGlyphFilter->Update();
  polydata = vertexGlyphFilter->GetOutput();
  
  if(!pointDataFloatArrays.empty()) {
    for(const auto& array : pointDataFloatArrays) {
      polydata->GetPointData()->AddArray(array);
      polydata->BuildLinks();
    }
  }
  
  WritePolyData(polydata, filename);
}
