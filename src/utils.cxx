#include "utils.h"

/*
  ----------
  --- IO ---
  ----------
*/

// Image reader
template <typename ImageType>
typename ImageType::Pointer ReadImage(const std::string& filename)
{
  auto reader = ImageReaderType<ImageType>::New();
  reader->SetFileName(filename.c_str());
  reader->Update();
  
  return reader->GetOutput();
}
template FloatImageType::Pointer ReadImage<FloatImageType>(const std::string&);
template FloatVectorImageType::Pointer ReadImage<FloatVectorImageType>(const std::string&);
template IntImageType::Pointer ReadImage<IntImageType>(const std::string&);
template UCharImageType::Pointer ReadImage<UCharImageType>(const std::string&);

// Image writer
template <typename ImageType>
void WriteImage(typename ImageType::Pointer image, const std::string& filename)
{
  auto writer = ImageWriterType<ImageType>::New();
  writer->SetInput(image);
  writer->SetFileName(filename);
  writer->Update();
}
template void WriteImage<FloatImageType>
(FloatImageType::Pointer image, const std::string& filename);
template void WriteImage<IntImageType>
(IntImageType::Pointer image, const std::string& filename);
template void WriteImage<UCharImageType>
(UCharImageType::Pointer image, const std::string& filename);


/*
  ------------------
  --- Image math ---
  ------------------
*/

// Add image filter
template <typename InImage1Type, typename InImage2Type, typename OutImageType>
typename OutImageType::Pointer AddImages
(typename InImage1Type::Pointer image1, typename InImage2Type::Pointer image2)
{
  auto filter = AddImageFilterType<InImage1Type, InImage2Type, OutImageType>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();
  
  return filter->GetOutput();
}
template IntImageType::Pointer AddImages<IntImageType, IntImageType, IntImageType>
(IntImageType::Pointer image1, IntImageType::Pointer image2);
template UCharImageType::Pointer AddImages<UCharImageType, UCharImageType, UCharImageType>
(UCharImageType::Pointer image1, UCharImageType::Pointer image2);

// Binary threshold
template <typename InImageType, typename OutImageType>
typename OutImageType::Pointer BinaryThresholdImage
(typename InImageType::Pointer image, typename InImageType::PixelType lower,
 typename InImageType::PixelType upper, typename OutImageType::PixelType outside,
 typename OutImageType::PixelType inside)
{
  auto filter = BinaryThresholdImageFilterType<InImageType, OutImageType>::New();
  filter->SetInput(image);
  filter->SetLowerThreshold(lower);
  filter->SetUpperThreshold(upper);
  filter->SetOutsideValue(outside);
  filter->SetInsideValue(inside);
  filter->Update();

  return filter->GetOutput();
}
template UCharImageType::Pointer BinaryThresholdImage<IntImageType, UCharImageType>
(IntImageType::Pointer image, IntImageType::PixelType lower, IntImageType::PixelType upper,
 UCharImageType::PixelType outside, UCharImageType::PixelType inside);
template UCharImageType::Pointer BinaryThresholdImage<UCharImageType, UCharImageType>
(UCharImageType::Pointer image, UCharImageType::PixelType lower, UCharImageType::PixelType upper,
 UCharImageType::PixelType outside, UCharImageType::PixelType inside);
template UCharImageType::Pointer BinaryThresholdImage<FloatImageType, UCharImageType>
(FloatImageType::Pointer image, FloatImageType::PixelType lower, FloatImageType::PixelType upper,
 UCharImageType::PixelType outside, UCharImageType::PixelType inside);

// Mask image filter
template <typename InImageType, typename MaskImageType, typename OutImageType>
typename OutImageType::Pointer MaskImage
(typename InImageType::Pointer image, typename MaskImageType::Pointer mask)
{
  // TO-DO
  
}

// Multiply image filter
template <typename InImage1Type, typename InImage2Type, typename OutImageType>
typename OutImageType::Pointer MultiplyImages
(typename InImage1Type::Pointer image1, typename InImage2Type::Pointer image2)
{
  auto filter = MultiplyImageFilterType<InImage1Type, InImage2Type, OutImageType>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();

  return filter->GetOutput();
}
template IntImageType::Pointer MultiplyImages<IntImageType, IntImageType, IntImageType>
(IntImageType::Pointer image1, IntImageType::Pointer image2);
template UCharImageType::Pointer MultiplyImages<UCharImageType, UCharImageType, UCharImageType>
(UCharImageType::Pointer image1, UCharImageType::Pointer image2);

// Subtract image filter
template <typename InImage1Type, typename InImage2Type, typename OutImageType>
typename OutImageType::Pointer SubtractImages
(typename InImage1Type::Pointer image1, typename InImage2Type::Pointer image2)
{
  auto filter = SubtractImageFilterType<InImage1Type, InImage2Type, OutImageType>::New();
  filter->SetInput1(image1);
  filter->SetInput2(image2);
  filter->Update();

  return filter->GetOutput();
}
template IntImageType::Pointer SubtractImages<IntImageType, IntImageType, IntImageType>
(IntImageType::Pointer image1, IntImageType::Pointer image2);
template UCharImageType::Pointer SubtractImages<UCharImageType, UCharImageType, UCharImageType>
(UCharImageType::Pointer image1, UCharImageType::Pointer image2);



/*
  ---------------------------
  --- Image manipulations ---
  --------------------------- 
*/

template <typename InImageType, typename OutImageType>
typename OutImageType::Pointer CastImage(typename InImageType::Pointer image)
{
  auto filter = CastImageFilterType<InImageType, OutImageType>::New();
  filter->SetInput(image);
  filter->Update();
  
  return filter->GetOutput();
}  



/*
  -----------------------------
  --- Helpful for debugging ---
  -----------------------------
*/

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
