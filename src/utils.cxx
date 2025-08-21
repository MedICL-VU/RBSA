#include "utils.h"


/*
  --------------------------
  --- Basic vector stuff ---
  --------------------------
*/

// Is pixel value in vector of pixel values?
template <typename TImage>
bool IsInside
(const typename TImage::PixelType& label, const std::vector<typename TImage::PixelType>& valid)
{
  if(valid.empty() || std::find(valid.begin(), valid.end(), label) == valid.end()) {
    return false;
  }
  return true;
}

template bool IsInside<IntImageType>
(const IntImageType::PixelType& label, const std::vector<IntImageType::PixelType>& valid);

template bool IsInside<FloatImageType>
(const FloatImageType::PixelType& label, const std::vector<FloatImageType::PixelType>& valid);

template bool IsInside<UCharImageType>
(const UCharImageType::PixelType& label, const std::vector<UCharImageType::PixelType>& valid);


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


std::string GetFileExtension(std::string filename) {
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


std::string GetBaseName(const std::string& filename) {
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
typename TImage::Pointer ReadImage(const std::string& filename)
{
  auto reader = ImageReaderType<TImage>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();
  
  return reader->GetOutput();
}

template FloatImageType::Pointer ReadImage<FloatImageType>(const std::string&);
template IntImageType::Pointer ReadImage<IntImageType>(const std::string&);
template VectorImageType::Pointer ReadImage<VectorImageType>(const std::string&);
template UCharImageType::Pointer ReadImage<UCharImageType>(const std::string&);


// ITK writer
template <typename TImage>
void WriteImage
(typename TImage::Pointer image, const std::string& filename)
{
  auto writer = ImageWriterType<TImage>::New();
  writer->SetInput(image);
  writer->SetFileName(filename);
  writer->Update();
}

template void WriteImage<FloatImageType>
(FloatImageType::Pointer image, const std::string& filename);
template void WriteImage<IntImageType>
(IntImageType::Pointer image, const std::string& filename);
template void WriteImage<VectorImageType>
(VectorImageType::Pointer image, const std::string& filename);
template void WriteImage<UCharImageType>
(UCharImageType::Pointer image, const std::string& filename);


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
template <typename TImage>
typename TImage::Pointer AddImages
(typename TImage::Pointer image1, typename TImage::Pointer image2)
{
  auto filter = AddImageFilterType<TImage>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();

  return filter->GetOutput();
}

template IntImageType::Pointer AddImages<IntImageType>
(IntImageType::Pointer image1, IntImageType::Pointer image2);

template UCharImageType::Pointer AddImages<UCharImageType>
(UCharImageType::Pointer image1, UCharImageType::Pointer image2);


// Binary fill holes
UCharImageType::Pointer BinaryFillHoles
(UCharImageType::Pointer image, UCharImageType::PixelType value)
{
  auto filter = BinaryFillHolesFilterType::New();
  filter->SetInput(image);
  filter->SetForegroundValue(value);
  filter->Update();

  return filter->GetOutput();
}


// Binary threshold
template <typename TInImage>
typename UCharImageType::Pointer BinaryThresholdImage
(typename TInImage::Pointer image, const typename TInImage::PixelType& lower,
 const typename TInImage::PixelType& upper, const UCharImageType::PixelType& outside,
 const UCharImageType::PixelType& inside)
{
  auto filter = BinaryThresholdImageFilterType<TInImage>::New();
  filter->SetInput(image);
  filter->SetLowerThreshold(lower);
  filter->SetUpperThreshold(upper);
  filter->SetOutsideValue(outside);
  filter->SetInsideValue(inside);
  filter->Update();
  
  return filter->GetOutput();
}

template UCharImageType::Pointer BinaryThresholdImage<IntImageType>
(IntImageType::Pointer image, const IntImageType::PixelType& lower,
 const IntImageType::PixelType& upper, const UCharImageType::PixelType& outside,
 const UCharImageType::PixelType& inside);

template UCharImageType::Pointer BinaryThresholdImage<UCharImageType>
(UCharImageType::Pointer image, const UCharImageType::PixelType& lower,
 const UCharImageType::PixelType& upper, const UCharImageType::PixelType& outside,
 const UCharImageType::PixelType& inside);

template UCharImageType::Pointer BinaryThresholdImage<FloatImageType>
(FloatImageType::Pointer image, const FloatImageType::PixelType& lower,
 const FloatImageType::PixelType& upper, const unsigned char& outside,
 const UCharImageType::PixelType& inside);


// Cast image
template <typename TInImage, typename TOutImage>
typename TOutImage::Pointer CastImage(typename TInImage::Pointer image)
{
  auto filter = CastImageFilterType<TInImage, TOutImage>::New();
  filter->SetInput(image);
  filter->Update();
  
  return filter->GetOutput();
}

template FloatImageType::Pointer CastImage<UCharImageType, FloatImageType>
(UCharImageType::Pointer image);
template UCharImageType::Pointer CastImage<FloatImageType, UCharImageType>
(FloatImageType::Pointer image);


// Duplicate image
template <typename TImage>
typename TImage::Pointer DuplicateImage(typename TImage::Pointer image)
{
  auto duplicator = DuplicateImageFilterType<TImage>::New();
  duplicator->SetInputImage(image);
  duplicator->Update();

  return duplicator->GetModifiableOutput();
}
template FloatImageType::Pointer DuplicateImage<FloatImageType>
(FloatImageType::Pointer image);

template IntImageType::Pointer DuplicateImage<IntImageType>
(IntImageType::Pointer image);

template VectorImageType::Pointer DuplicateImage<VectorImageType>
(VectorImageType::Pointer image);

template UCharImageType::Pointer DuplicateImage<UCharImageType>
(UCharImageType::Pointer image);


// Create zero-filled image
template <typename TInImage, typename TOutImage>
typename TOutImage::Pointer InitializeZeroFilledImage(typename TInImage::Pointer ref)
{
  typename TOutImage::Pointer image = TOutImage::New();
  image->SetRegions(ref->GetLargestPossibleRegion());
  image->SetDirection(ref->GetDirection());
  image->SetOrigin(ref->GetOrigin());
  image->SetSpacing(ref->GetSpacing());
  image->Allocate();
  image->FillBuffer(itk::NumericTraits<typename TOutImage::PixelType>::ZeroValue());
  
  return image;
}

template UCharImageType::Pointer InitializeZeroFilledImage<IntImageType, UCharImageType>
(IntImageType::Pointer image);

template UCharImageType::Pointer InitializeZeroFilledImage<UCharImageType, UCharImageType>
(UCharImageType::Pointer image);

template FloatImageType::Pointer InitializeZeroFilledImage<UCharImageType, FloatImageType>
(UCharImageType::Pointer image);

template VectorImageType::Pointer InitializeZeroFilledImage<UCharImageType, VectorImageType>
(UCharImageType::Pointer image);


// Check if label is in image
template <typename TImage>
bool IsLabelInImage(typename TImage::Pointer image, const typename TImage::PixelType label)
{
  ImageRegionConstIteratorType<TImage> iterator(image, image->GetLargestPossibleRegion());
  for(iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator) {
    if(iterator.Get() == label) return true;
    ++iterator;
  }
  return false;
}

template bool IsLabelInImage<IntImageType>
(IntImageType::Pointer image, const IntImageType::PixelType label);

template bool IsLabelInImage<UCharImageType>
(UCharImageType::Pointer image, const UCharImageType::PixelType label);


// Mask image
template <typename TImage>
typename TImage::Pointer MaskImage(typename TImage::Pointer image, UCharImageType::Pointer mask)
{
  auto filter = MaskImageFilterType<TImage>::New();
  filter->SetInput1(image);
  filter->SetInput2(mask);
  filter->Update();
  
  return filter->GetOutput();
}

template VectorImageType::Pointer MaskImage<VectorImageType>
(VectorImageType::Pointer image, UCharImageType::Pointer mask);


// Multiplication
template <typename TImage>
typename TImage::Pointer MultiplyImages
(typename TImage::Pointer image1, typename TImage::Pointer image2)
{
  auto filter = MultiplyImageFilterType<TImage>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();

  return filter->GetOutput();
}

template IntImageType::Pointer MultiplyImages<IntImageType>
(IntImageType::Pointer image1, IntImageType::Pointer image2);

template UCharImageType::Pointer MultiplyImages<UCharImageType>
(UCharImageType::Pointer image1, UCharImageType::Pointer image2);


// Subtraction
template <typename TImage>
typename TImage::Pointer SubtractImages
(typename TImage::Pointer image1, typename TImage::Pointer image2)
{
  auto filter = SubtractImageFilterType<TImage>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();

  return filter->GetOutput();
}

template IntImageType::Pointer SubtractImages<IntImageType>
(IntImageType::Pointer image1, IntImageType::Pointer image2);

template UCharImageType::Pointer SubtractImages<UCharImageType>
(UCharImageType::Pointer image1, UCharImageType::Pointer image2);


// Sum of image values
template <typename TImage>
typename TImage::PixelType ImageSum(typename TImage::Pointer image)
{
  auto filter = StatisticsImageFilterType<TImage>::New();
  filter->SetInput(image);
  filter->Update();

  return static_cast<typename TImage::PixelType>(filter->GetSum());
}

template UCharImageType::PixelType ImageSum<UCharImageType>(UCharImageType::Pointer image);



// Image index functions
template <typename TImage>
ContinuousIndexType TransformNDimsDoubleToContinuousIndex
(typename TImage::Pointer image, double p0[nDims], bool convertFromRAS)
{
  itk::Point<double, nDims> itkPoint;
  for(unsigned int d = 0; d < nDims; d++) {
    itkPoint[d] = (convertFromRAS) ? rasShift[d] * p0[d] : p0[d];
  }

  ContinuousIndexType cIdx;
  bool isInside = image->TransformPhysicalPointToContinuousIndex(itkPoint, cIdx);
  if(!isInside) {
    std::cerr << "huh, index " << cIdx << " is not inside\n";
  }
  return cIdx;
}

template ContinuousIndexType TransformNDimsDoubleToContinuousIndex<IntImageType>
(IntImageType::Pointer image, double p0[nDims], bool convertFromRAS);

template ContinuousIndexType TransformNDimsDoubleToContinuousIndex<UCharImageType>
(UCharImageType::Pointer image, double p0[nDims], bool convertFromRAS);

template ContinuousIndexType TransformNDimsDoubleToContinuousIndex<FloatImageType>
(FloatImageType::Pointer image, double p0[nDims], bool convertFromRAS);

template ContinuousIndexType TransformNDimsDoubleToContinuousIndex<VectorImageType>
(VectorImageType::Pointer image, double p0[nDims], bool convertFromRAS);

template <typename TImage>
typename TImage::IndexType TransformNDimsDoubleToIndex
(typename TImage::Pointer image, double p0[nDims], bool convertFromRAS)
{
  itk::Point<double, nDims> itkPoint;
  for(unsigned int d = 0; d < nDims; d++) {
    itkPoint[d] = (convertFromRAS) ? rasShift[d] * p0[d] : p0[d];
  }
  
  typename TImage::IndexType idx;
  bool isInside = image->TransformPhysicalPointToIndex(itkPoint, idx);

  return idx;
}

template IntImageType::IndexType TransformNDimsDoubleToIndex<IntImageType>
(IntImageType::Pointer image, double p0[nDims], bool convertFromRAS);

template UCharImageType::IndexType TransformNDimsDoubleToIndex<UCharImageType>
(UCharImageType::Pointer image, double p0[nDims], bool convertFromRAS);

template FloatImageType::IndexType TransformNDimsDoubleToIndex<FloatImageType>
(FloatImageType::Pointer image, double p0[nDims], bool convertFromRAS);



template <typename TImage>
void TransformIndexToNDimsDouble
(typename TImage::Pointer image, typename TImage::IndexType index, double (&p0)[nDims])
{
  itk::Point<double, nDims> itkPoint;
  image->TransformIndexToPhysicalPoint(index, itkPoint);

  for(unsigned int d = 0; d < nDims; d++) {
    p0[d] = itkPoint[d];
  }
}

template void TransformIndexToNDimsDouble<VectorImageType>
(VectorImageType::Pointer image, VectorImageType::IndexType index, double (&p0)[nDims]);

template void TransformIndexToNDimsDouble<UCharImageType>
(UCharImageType::Pointer image, UCharImageType::IndexType index, double (&p0)[nDims]);


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
(vtkSmartPointer<vtkPolyData> mesh, typename TImage::Pointer image)
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
(vtkSmartPointer<vtkPolyData> mesh, UCharImageType::Pointer image);


void BinaryITKImageToVTKMesh
(UCharImageType::Pointer itkTargetImage, vtkSmartPointer<vtkPolyData> outputMesh)
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
        const UCharImageType::IndexType& idx = {i, j, k};
        const UCharImageType::PixelType& value = itkTargetImage->GetPixel(idx);
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


void PrintDuration
(std::chrono::steady_clock::time_point t0, std::string text, std::string time_type)
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

void VisualizePointSet
(vtkSmartPointer<vtkPoints> points, const std::string& filename,
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
