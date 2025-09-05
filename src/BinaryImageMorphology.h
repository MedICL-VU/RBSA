#ifndef BINARY_IMAGE_MORPHOLOGY_H
#define BINARY_IMAGE_MORPHOLOGY_H

#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryDilateImageFilter.h>

#include "utils.h"

const int kernelRadius = 1;


// Type defs
using BinaryBallStructuringElementType =
  itk::BinaryBallStructuringElement<TPixel<UCharImageType>, nDims>;

using BinaryDilateImageFilterType =
  itk::BinaryDilateImageFilter<UCharImageType, UCharImageType, BinaryBallStructuringElementType>;

using BinaryErodeImageFilterType =
  itk::BinaryErodeImageFilter<UCharImageType, UCharImageType, BinaryBallStructuringElementType>;


// Functions
TPointer<UCharImageType> DilateImage(TPointer<UCharImageType> image,
				    BinaryBallStructuringElementType kernel,
				    TPixel<UCharImageType> value = 1);

TPointer<UCharImageType> ErodeImage(TPointer<UCharImageType> image,
				   BinaryBallStructuringElementType kernel,
				   TPixel<UCharImageType> value = 1);

TPointer<UCharImageType> DilateErodeCorrection(TPointer<UCharImageType> image);

TPointer<UCharImageType> LocalizedAtrophy(TPointer<UCharImageType> targetLabel,
					 TPointer<UCharImageType> referenceAdjacentLabel,
					 unsigned int nIters);

TPointer<UCharImageType> ReplaceLabelInImage(TPointer<UCharImageType> inputImage,
					    TPointer<UCharImageType> origLabelMask,
					    TPointer<UCharImageType> atrophyLabelMask);

#endif
