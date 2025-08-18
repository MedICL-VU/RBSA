#include <typeinfo>
#include "HighResCrop.h"


HighResCropFromReferenceMask::HighResCropFromReferenceMask()
{
  this->m_buffer = 0;
}


void HighResCropFromReferenceMask::FindCropRegion()
{
  // Get reference image info
  const auto& size = m_ref->GetLargestPossibleRegion().GetSize();
  const auto& spacing = m_ref->GetSpacing();
  const auto& origin = m_ref->GetOrigin();
  const auto& direction = m_ref->GetDirection();
  const auto& index = m_ref->GetLargestPossibleRegion().GetIndex();

  // Initialize
  ImageRegionIteratorWithIndexType<UCharImageType>
    iterator(m_ref, m_ref->GetLargestPossibleRegion());
  iterator.GoToReverseBegin();
  this->m_startIndex = iterator.GetIndex();
  iterator.GoToBegin();
  this->m_endIndex = iterator.GetIndex();

  // Get crop region indices
  for(iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator) {
    if(iterator.Get() == 1) {
      UCharImageType::IndexType idx = iterator.GetIndex();
      
      for(unsigned int d = 0; d < nDims; d++) {
	this->m_startIndex[d] = (idx[d] < this->m_startIndex[d]) ? idx[d] : this->m_startIndex[d];
	this->m_endIndex[d] = (idx[d] > this->m_endIndex[d]) ? idx[d] : this->m_endIndex[d];
      }
    }
  }
  
  // Add a buffer
  for(unsigned int d = 0; d < nDims; d++) {
    const unsigned int lower = index[d];
    const unsigned int upper = index[d] + size[d] - 1;
    
    this->m_startIndex[d] = (this->m_startIndex[d] - this->m_buffer >= lower)
      ? this->m_startIndex[d] - this->m_buffer : lower;
    this->m_endIndex[d] = (this->m_endIndex[d] + this->m_buffer <= upper)
      ? this->m_endIndex[d] + this->m_buffer : upper;
  }
}


// Resample()
template <typename TImage>
typename TImage::Pointer ResampleImage(typename TImage::Pointer image, double factor)
{
  // Initialize resampler
  auto resampleFilter = ResampleImageFilterType<TImage>::New();
  resampleFilter->SetInput(image);
  resampleFilter->SetOutputDirection(image->GetDirection());
  resampleFilter->SetOutputOrigin(image->GetOrigin());
  resampleFilter->SetDefaultPixelValue(itk::NumericTraits<typename TImage::PixelType>::ZeroValue());
  
  // Set transform to identity
  auto transform = itk::IdentityTransform<double, nDims>::New();
  transform->SetIdentity();
  resampleFilter->SetTransform(transform);

  // Set interpolator based on TImage
  if(typeid(typename TImage::PixelType) == typeid(FloatImageType::PixelType)
     || typeid(typename TImage::PixelType) == typeid(VectorImageType::PixelType)) {
    auto interpolator = LinearInterpolateType<TImage>::New();
    resampleFilter->SetInterpolator(interpolator);
  }
  else {
    auto interpolator = NearestNeighborInterpolateType<TImage>::New();
    resampleFilter->SetInterpolator(interpolator);
  }

  // Set upsampled spacing and size
  auto spacing = image->GetSpacing();
  auto size = image->GetLargestPossibleRegion().GetSize();

  for(unsigned int d = 0; d < nDims; d++) {
    spacing[d] /= factor;
    size[d] *= factor;
  }
  resampleFilter->SetOutputSpacing(spacing);
  resampleFilter->SetSize(size);

  // Resample
  resampleFilter->Update();
  return resampleFilter->GetOutput();
}

template UCharImageType::Pointer
ResampleImage<UCharImageType>(UCharImageType::Pointer image, double factor);
template FloatImageType::Pointer
ResampleImage<FloatImageType>(FloatImageType::Pointer image, double factor);


// Apply()
template <typename TImage>
typename TImage::Pointer HighResCropFromReferenceMask::Apply(typename TImage::Pointer image)
{
  typename TImage::RegionType region;
  region.SetIndex(this->m_startIndex);
  region.SetUpperIndex(this->m_endIndex);

  auto cropFilter = ROIImageFilterType<TImage>::New();
  cropFilter->SetInput(image);
  cropFilter->SetRegionOfInterest(region);
  cropFilter->Update();
  
  typename TImage::Pointer outputImage =
    ResampleImage<TImage>(cropFilter->GetOutput(), this->m_resamplingFactor);
    
  return outputImage;
}

template UCharImageType::Pointer
HighResCropFromReferenceMask::Apply<UCharImageType>(UCharImageType::Pointer image);

template FloatImageType::Pointer
HighResCropFromReferenceMask::Apply<FloatImageType>(FloatImageType::Pointer image);


// Revert()
template <typename TImage>
typename TImage::Pointer HighResCropFromReferenceMask::Revert(typename TImage::Pointer image)
{
  // Resample back to original spacing
  auto resampled = ResampleImage<TImage>(image, static_cast<double>(1) / this->m_resamplingFactor);

  // Create new image w/ correct dimensions
  auto canvas = InitializeZeroFilledImage<UCharImageType, TImage>(this->m_ref);
  
  // Paste resampled image into new
  auto pasteFilter = PasteImageFilterType<TImage>::New();
  pasteFilter->SetSourceImage(resampled);
  pasteFilter->SetDestinationImage(canvas);
  pasteFilter->SetSourceRegion(resampled->GetLargestPossibleRegion());
  pasteFilter->SetDestinationIndex(this->m_startIndex);
  pasteFilter->Update();

  return pasteFilter->GetOutput();
}

template UCharImageType::Pointer
HighResCropFromReferenceMask::Revert<UCharImageType>(UCharImageType::Pointer image);

template VectorImageType::Pointer
HighResCropFromReferenceMask::Revert<VectorImageType>(VectorImageType::Pointer image);
