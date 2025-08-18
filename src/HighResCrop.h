#ifndef HIGH_RES_CROP_H
#define HIGH_RES_CROP_H

#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageRegion.h>
#include <itkCropImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkPasteImageFilter.h>

#include "utils.h"

using RegionType = itk::ImageRegion<nDims>;


// Main class
class HighResCropFromReferenceMask {
 public:
  HighResCropFromReferenceMask();

  // Set params
  void SetReferenceImage(UCharImageType::Pointer image) { this->m_ref = image; }
  void SetCropRegionBuffer(unsigned int buffer = 0) { this->m_buffer = buffer; }
  void SetResamplingFactor(float factor = 1.0 ) { this->m_resamplingFactor = factor; }

  // Get params
  UCharImageType::IndexType GetStartIndex() const { return this->m_startIndex; }
  UCharImageType::IndexType GetEndIndex() const { return this->m_endIndex; }
  
  // Member functions
  void FindCropRegion();
  template <typename TImage> typename TImage::Pointer Apply(typename TImage::Pointer image);
  template <typename TImage> typename TImage::Pointer Revert(typename TImage::Pointer image);
  
 private:
  UCharImageType::Pointer m_ref;
  UCharImageType::IndexType m_startIndex;
  UCharImageType::IndexType m_endIndex;
  unsigned int m_buffer;
  float m_resamplingFactor;
};



// Typedefs
template <typename TImage>
using CropImageFilterType = itk::CropImageFilter<TImage, TImage>;

template <typename TImage>
using ResampleImageFilterType = itk::ResampleImageFilter<TImage, TImage>;

template <typename TImage>
using ROIImageFilterType = itk::RegionOfInterestImageFilter<TImage, TImage>;

template <typename TImage>
using ConstantPadImageFilterType = itk::ConstantPadImageFilter<TImage, TImage>;

template <typename TImage>
using PasteImageFilterType = itk::PasteImageFilter<TImage, TImage>;

// Functions
template <typename TImage>
typename TImage::Pointer ResampleImage(typename TImage::Pointer image, double factor);


#endif





  
