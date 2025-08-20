#include "MSDD.h"


//------------------

void CalculateSurfaceDistance
(vtkSmartPointer<vtkPolyData> surface0, vtkSmartPointer<vtkPolyData> surface1)
{
  const unsigned int& nPoints = surface0->GetNumberOfPoints();
  
  // Get distance
  vtkNew<vtkDistancePolyDataFilter> distanceFilter;
  distanceFilter->SetInputData(0, surface0);
  distanceFilter->SetInputData(1, surface1);
  distanceFilter->SignedDistanceOn();
  distanceFilter->Update();

  // Convert to float array
  auto buffer = dynamic_cast<vtkDoubleArray*>(distanceFilter->GetOutput()->GetPointData()
					      ->GetArray("Distance"));
  auto array = vtkSmartPointer<vtkFloatArray>::New();
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(buffer->GetNumberOfTuples());
  array->SetName("SurfaceDistance");

  for(unsigned int p = 0; p < nPoints; p++) {
    array->SetValue(p, static_cast<float>(buffer->GetValue(p)));
  }

  // Add array to surface
  surface0->GetPointData()->AddArray(array);
  surface0->BuildLinks();
}


float GetMeanDistanceWithinLabel(vtkSmartPointer<vtkPolyData> surface, const int& label)
{
  // Get arrays
  vtkSmartPointer<vtkFloatArray> distances =
    vtkFloatArray::SafeDownCast(surface->GetPointData()->GetArray("SurfaceDistance"));
  vtkSmartPointer<vtkIntArray> labels =
    vtkIntArray::SafeDownCast(surface->GetPointData()->GetArray("TargetLabels"));

  if(!distances) {
    std::cerr << "[MSDD] SurfaceDistance array does not exist :(" << std::endl;
    return 0.0;
  }
  if(!labels) {
    std::cerr << "[MSDD] TargetLabels array does not exist :(" << std::endl;
    return 0.0;
  }
  
  // Get sum of distances where labels == label
  float sum = 0.0;
  int count = 0;
  
  for(unsigned int n = 0; n < labels->GetNumberOfValues(); n++) {
    if(labels->GetValue(n) == label) {
      sum += distances->GetValue(n);
      ++count;
    }
  }

  // Return mean value
  return (count > 0) ? (sum / static_cast<float>(count)) : 0.0;
}


UCharImageType::Pointer MakeBinaryMask
(IntImageType::Pointer ref, const std::vector<int>& labels, UCharImageType::Pointer mask)
{
  if(!mask) {
    mask = InitializeZeroFilledImage<IntImageType, UCharImageType>(ref);
  }
  for(const auto& label : labels) {
    UCharImageType::Pointer temp = BinaryThresholdImage<IntImageType>(ref, label, label, 0, 1);
    mask = AddImages<UCharImageType>(temp, mask);
  }
  mask = BinaryThresholdImage<UCharImageType>(mask, 1, 1, 0, 1);

  return mask;
}


//-------------------------------------------------------------------------------------------------

MSDD::MSDD()
{
  this->m_defaultPixelValue = 1;

  this->m_lGM0 = nullptr;
  this->m_lWM0 = nullptr;
  this->m_lGM1 = nullptr;
  this->m_lWM1 = nullptr;

  this->m_rGM0 = nullptr;
  this->m_rWM0 = nullptr;
  this->m_rGM1 = nullptr;
  this->m_rWM1 = nullptr;
};


void MSDD::MakeHemiTemplates
(IntImageType::Pointer mask, const std::vector<int>& lGMs, const std::vector<int>& lWMs,
 const std::vector<int>& rGMs, const std::vector<int>& rWMs)
{
  // Left WM
  this->m_lMaskWM = MakeBinaryMask(mask, lWMs);  
  if(ImageSum<UCharImageType>(this->m_lMaskWM) == 0) {
    std::cerr << "Error: input hemis template does not contain any provided labels for lh WM\n";
    return;
  }
  
  // Left GM
  this->m_lMaskGM = DuplicateImage<UCharImageType>(this->m_lMaskWM);
  this->m_lMaskGM =  MakeBinaryMask(mask, lGMs, this->m_lMaskGM);
  if(ImageSum<UCharImageType>(this->m_lMaskGM) == 0) {
    std::cerr << "Error: input hemis template does not contain any provided labels for lh GM\n";
    return;
  }

  // Right WM
  this->m_rMaskWM = MakeBinaryMask(mask, rWMs);
  if(ImageSum<UCharImageType>(this->m_rMaskWM) == 0) {
    std::cerr << "Error: input hemis template does not contain any provided labels for rh WM\n";
    return;
  }

  // Left GM
  this->m_rMaskGM = DuplicateImage<UCharImageType>(this->m_rMaskWM);
  this->m_rMaskGM = MakeBinaryMask(mask, rGMs, this->m_rMaskGM);
  if(ImageSum<UCharImageType>(this->m_rMaskGM) == 0) {
    std::cerr << "Error: input hemis template does not contain any provided labels for rh GM\n";
    return;
  }
}


void MSDD::CreateCustomParcellation()
{
  // Make sure default pixel value is not a target label value
  if(IsInside<IntImageType>(this->m_defaultPixelValue, this->m_labels)) {
    this->m_defaultPixelValue = *std::max_element(this->m_labels.begin(), this->m_labels.end());
  }

  // Convert all pixels in image to defaultPixelValue (1) except the target labels
  ImageRegionIteratorWithIndexType<IntImageType>
    iterator(this->m_parc, this->m_parc->GetLargestPossibleRegion());
  iterator.GoToBegin();

  while(!iterator.IsAtEnd()) {
    const int& pixel = iterator.Get();
    if(pixel != 0 && !IsInside<IntImageType>(pixel, this->m_labels)) {
      iterator.Set(this->m_defaultPixelValue);
    }
    ++iterator;
  }
}


void MSDD::GetLabelHemis()
{
  const size_t& nLabels = this->m_labels.size();
  std::vector<int> left, right;
  left.reserve(nLabels);
  right.reserve(nLabels);
  
  for(const auto& label : this->m_labels) {
    UCharImageType::Pointer labelMask =
      BinaryThresholdImage<IntImageType>(this->m_parc, label, label, 0, 1);
    UCharImageType::Pointer lMask = MultiplyImages<UCharImageType>(this->m_lMaskGM, labelMask);
    UCharImageType::Pointer rMask = MultiplyImages<UCharImageType>(this->m_rMaskGM, labelMask);

    const UCharImageType::PixelType& lSum = ImageSum<UCharImageType>(lMask);
    const UCharImageType::PixelType& rSum = ImageSum<UCharImageType>(rMask);

    if(lSum > rSum) {
      left.emplace_back(label);
    }
    else if(rSum > lSum) {
      right.emplace_back(label);
    }
  }

  this->m_hemiLabels = {std::move(left), std::move(right)};
}


void MSDD::GenerateSurfaces()
{
  const size_t& nLabels = this->m_labels.size();
  std::vector<int> left = std::get<0>(this->m_hemiLabels);
  std::vector<int> right = std::get<1>(this->m_hemiLabels);
  
  // Generate data (left)
  if(!left.empty()) {
    this->m_lGM0 = vtkSmartPointer<vtkPolyData>::New();
    this->m_lWM0 = vtkSmartPointer<vtkPolyData>::New();
    BinaryITKImageToVTKMesh(this->m_lMaskGM, this->m_lGM0);
    BinaryITKImageToVTKMesh(this->m_lMaskWM, this->m_lWM0);

    left.push_back(this->m_defaultPixelValue);
    ParcellateSurface<IntImageType>(this->m_lGM0, this->m_parc, "TargetLabels", left);
    ParcellateSurface<IntImageType>(this->m_lWM0, this->m_parc, "TargetLabels", left);

    this->m_lGM1 = WarpSurface(this->m_lGM0, this->m_warp, this->m_warpMask, false, false);
    this->m_lWM1 = WarpSurface(this->m_lWM0, this->m_warp, this->m_warpMask, false, true);

    CalculateSurfaceDistance(this->m_lGM0, this->m_lGM1);
    CalculateSurfaceDistance(this->m_lGM1, this->m_lGM0);
    CalculateSurfaceDistance(this->m_lWM0, this->m_lWM1);
    CalculateSurfaceDistance(this->m_lWM1, this->m_lWM0);
  }

  // Generate data (right)
  if(!right.empty()) {
    this->m_rGM0 = vtkSmartPointer<vtkPolyData>::New();
    this->m_rWM0 = vtkSmartPointer<vtkPolyData>::New();
    BinaryITKImageToVTKMesh(this->m_rMaskGM, this->m_rGM0);
    BinaryITKImageToVTKMesh(this->m_rMaskWM, this->m_rWM0);

    right.push_back(this->m_defaultPixelValue);
    ParcellateSurface<IntImageType>(this->m_rGM0, this->m_parc, "TargetLabels", right);
    ParcellateSurface<IntImageType>(this->m_rWM0, this->m_parc, "TargetLabels", right);
    
    this->m_rGM1 = WarpSurface(this->m_rGM0, this->m_warp, this->m_warpMask, false, false);
    this->m_rWM1 = WarpSurface(this->m_rWM0, this->m_warp, this->m_warpMask, false, true);

    CalculateSurfaceDistance(this->m_rGM0, this->m_rGM1);
    CalculateSurfaceDistance(this->m_rGM1, this->m_rGM0);
    CalculateSurfaceDistance(this->m_rWM0, this->m_rWM1);
    CalculateSurfaceDistance(this->m_rWM1, this->m_rWM0);
  }
}


void MSDD::Calculate()
{
  // Make sure all inputs have been set
  if(!this->m_parc) {
    std::cerr << "Error: cortical parcellation (m_parc) is null\n";
    return;
  }
  if(!this->m_lMaskGM) {
    std::cerr << "Error: left GM mask (this->m_lMaskGM) is null\n";
    return;
  }
  if(!this->m_lMaskWM) {
    std::cerr << "Error: left WM mask (this->m_lMaskWM) is null\n";
    return;
  }
  if(!this->m_rMaskGM) {
    std::cerr << "Error: right GM mask (this->m_rMaskGM) is null\n";
    return;
  }
  if(!this->m_rMaskWM) {
    std::cerr << "Error: right WM mask (this->m_rMaskWM) is null\n";
    return;
  }

  // Generate surface data
  this->CreateCustomParcellation();
  this->GetLabelHemis();
  this->GenerateSurfaces();

  // Get MSDD in each target label
  this->m_MSDDs.clear();
  this->m_MSDDs.reserve(this->m_labels.size());

  for(const auto& label : this->m_labels) {
    // Get data for the correct hemisphere
    vtkSmartPointer<vtkPolyData> gm0, wm0, gm1, wm1;
    if(IsInside<IntImageType>(label, std::get<0>(this->m_hemiLabels))) {
      gm0 = this->m_lGM0;
      wm0 = this->m_lWM0;
      gm1 = this->m_lGM1;
      wm1 = this->m_lWM1;
    }
    else if(IsInside<IntImageType>(label, std::get<1>(this->m_hemiLabels))) {
      gm0 = this->m_rGM0;
      wm0 = this->m_rWM0;
      gm1 = this->m_rGM1;
      wm1 = this->m_rWM1;
    }
    else {
      std::cerr << "Can't identify hemisphere for label " << label << ":(\n";
      this->m_MSDDs.emplace_back(std::numeric_limits<float>::quiet_NaN());
      continue;
    }

    // Get mean surface distance within target label
    float gmMean01 = GetMeanDistanceWithinLabel(gm0, label);
    float wmMean01 = GetMeanDistanceWithinLabel(wm0, label);
    float gmMean10 = GetMeanDistanceWithinLabel(gm1, label);
    float wmMean10 = GetMeanDistanceWithinLabel(wm1, label);

    // MSDD
    float gmMSD = 0.5 * (gmMean01 - gmMean10);
    float wmMSD = 0.5 * (wmMean01 - wmMean10);
    this->m_MSDDs.emplace_back(gmMSD - wmMSD);
  }
}


void MSDD::Write(const std::string& dirname)
{
  // Set up output filenames
  std::string lGM0FileName = dirname + "/gm_lh_orig.vtp";
  std::string lWM0FileName = dirname + "/wm_lh_orig.vtp";
  std::string lGM1FileName = dirname + "/gm_lh_atrophy.vtp";
  std::string lWM1FileName = dirname + "/wm_lh_atrophy.vtp";

  std::string rGM0FileName = dirname + "/gm_rh_orig.vtp";
  std::string rWM0FileName = dirname + "/wm_rh_orig.vtp";
  std::string rGM1FileName = dirname + "/gm_rh_atrophy.vtp";
  std::string rWM1FileName = dirname + "/wm_rh_atrophy.vtp";

  std::string warpFileName = dirname + "/warp.nii.gz";
  std::string msddFileName = dirname + "/msdd.txt";
  
  // Write surfaces
  if(this->m_lGM0) WritePolyData(this->m_lGM0, lGM0FileName);
  if(this->m_lWM0) WritePolyData(this->m_lWM0, lWM0FileName);
  if(this->m_lGM1) WritePolyData(this->m_lGM1, lGM1FileName);
  if(this->m_lWM1) WritePolyData(this->m_lWM1, lWM1FileName);

  if(this->m_rGM0) WritePolyData(this->m_rGM0, rGM0FileName);
  if(this->m_rWM0) WritePolyData(this->m_rWM0, rWM0FileName);
  if(this->m_rGM1) WritePolyData(this->m_rGM1, rGM1FileName);
  if(this->m_rWM1) WritePolyData(this->m_rWM1, rWM1FileName);
  
  // Write MSDDs
  const size_t& nLabels = this->m_labels.size();
  
  std::ofstream f(msddFileName.c_str());
  f << "Label MSDD\n";
  f << std::fixed << std::setprecision(4);
  
  for(size_t n = 0; n < nLabels; n++) {
    if(!std::isnan(this->m_MSDDs.at(n))) {
      f << this->m_labels.at(n) << " " << this->m_MSDDs.at(n) << "\n";
    }
    else {
      f << this->m_labels.at(n) << " NaN\n";
    }
  }
  f.close();
}
