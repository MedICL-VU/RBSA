#ifndef HIGH_RES_CROP_H
#define HIGH_RES_CROP_H

#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageRegion.h>
#include <itkCropImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkPasteImageFilter.h>

#include "utils.h"



// Main class
class HighResCropFromReferenceMask {
 public:
  HighResCropFromReferenceMask();

  // Set params
  void SetReferenceImage(TPointer<UCharImageType> image) { this->m_ref = image; }
  void SetCropRegionBuffer(unsigned int buffer = 0) { this->m_buffer = buffer; }
  void SetResamplingFactor(float factor = 1.0 ) { this->m_resamplingFactor = factor; }

  // Get params
  TIndex<UCharImageType> GetStartIndex() const { return this->m_startIndex; }
  TIndex<UCharImageType> GetEndIndex() const { return this->m_endIndex; }
  
  // Member functions
  void FindCropRegion();
  template <typename TImage> TPointer<TImage> Apply(TPointer<TImage> image);
  template <typename TImage> TPointer<TImage> Downsample(TPointer<TImage> image);
  template <typename TImage> TPointer<TImage> Revert(TPointer<TImage> image);
  
 private:
  TPointer<UCharImageType> m_ref;
  TIndex<UCharImageType> m_startIndex;
  TIndex<UCharImageType> m_endIndex;
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
TPointer<TImage> ResampleImage(TPointer<TImage> image, double factor);


#endif

  
