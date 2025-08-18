#ifndef BINARY_IMAGE_MORPHOLOGY_H
#define BINARY_IMAGE_MORPHOLOGY_H

#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryDilateImageFilter.h>

#include "utils.h"

const int kernelRadius = 1;


// Type defs
using BinaryBallStructuringElementType =
  itk::BinaryBallStructuringElement<UCharImageType::PixelType, nDims>;

using BinaryDilateImageFilterType =
  itk::BinaryDilateImageFilter<UCharImageType, UCharImageType, BinaryBallStructuringElementType>;

using BinaryErodeImageFilterType =
  itk::BinaryErodeImageFilter<UCharImageType, UCharImageType, BinaryBallStructuringElementType>;


// Functions
UCharImageType::Pointer DilateImage
(UCharImageType::Pointer image, BinaryBallStructuringElementType kernel,
 UCharImageType::PixelType value = 1);

UCharImageType::Pointer ErodeImage
(UCharImageType::Pointer image, BinaryBallStructuringElementType kernel,
 UCharImageType::PixelType value = 1);

UCharImageType::Pointer DilateErodeCorrection(UCharImageType::Pointer image);

UCharImageType::Pointer LocalizedAtrophy
(UCharImageType::Pointer targetLabel, UCharImageType::Pointer referenceAdjacentLabel,
 unsigned int nIters);

UCharImageType::Pointer ReplaceLabelInImage
(UCharImageType::Pointer inputImage, UCharImageType::Pointer origLabelMask,
 UCharImageType::Pointer atrophyLabelMask);

#endif
