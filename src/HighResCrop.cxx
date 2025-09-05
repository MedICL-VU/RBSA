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
      TIndex<UCharImageType> idx = iterator.GetIndex();
      
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
TPointer<TImage> ResampleImage(TPointer<TImage> image, double factor)
{
  // Initialize resampler
  auto resampleFilter = ResampleImageFilterType<TImage>::New();
  resampleFilter->SetInput(image);
  resampleFilter->SetOutputDirection(image->GetDirection());
  resampleFilter->SetOutputOrigin(image->GetOrigin());
  resampleFilter->SetDefaultPixelValue(itk::NumericTraits<TPixel<TImage>>::ZeroValue());
  
  // Set transform to identity
  auto transform = itk::IdentityTransform<double, nDims>::New();
  transform->SetIdentity();
  resampleFilter->SetTransform(transform);

  // Set interpolator based on TImage
  if(typeid(TPixel<TImage>) == typeid(TPixel<FloatImageType>)
     || typeid(TPixel<TImage>) == typeid(TPixel<VectorImageType>)) {
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

template TPointer<UCharImageType>
ResampleImage<UCharImageType>(TPointer<UCharImageType> image, double factor);

template TPointer<FloatImageType>
ResampleImage<FloatImageType>(TPointer<FloatImageType> image, double factor);


// Apply()
template <typename TImage>
TPointer<TImage> HighResCropFromReferenceMask::Apply(TPointer<TImage> image)
{
  TRegion<TImage> region;
  region.SetIndex(this->m_startIndex);
  region.SetUpperIndex(this->m_endIndex);

  auto cropFilter = ROIImageFilterType<TImage>::New();
  cropFilter->SetInput(image);
  cropFilter->SetRegionOfInterest(region);
  cropFilter->Update();
  
  TPointer<TImage> outputImage =
    ResampleImage<TImage>(cropFilter->GetOutput(), this->m_resamplingFactor);
  
  return outputImage;
}

template TPointer<UCharImageType>
HighResCropFromReferenceMask::Apply<UCharImageType>(TPointer<UCharImageType> image);

template TPointer<FloatImageType>
HighResCropFromReferenceMask::Apply<FloatImageType>(TPointer<FloatImageType> image);


// Downsample (restore original resolution, but keep as cropped patch)
template <typename TImage>
TPointer<TImage> HighResCropFromReferenceMask::Downsample(TPointer<TImage> image)
{
  auto resampled = ResampleImage<TImage>(image, static_cast<double>(1) / this->m_resamplingFactor);
  return image;
}

template TPointer<UCharImageType>
HighResCropFromReferenceMask::Downsample<UCharImageType>(TPointer<UCharImageType> image);

template TPointer<VectorImageType>
HighResCropFromReferenceMask::Downsample<VectorImageType>(TPointer<VectorImageType> image);


// Revert
template <typename TImage>
TPointer<TImage> HighResCropFromReferenceMask::Revert(TPointer<TImage> image)
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

  TPointer<TImage> output = pasteFilter->GetOutput();
  output->DisconnectPipeline();
  return output;
}

template TPointer<VectorImageType>
HighResCropFromReferenceMask::Revert<VectorImageType>(TPointer<VectorImageType> image);
