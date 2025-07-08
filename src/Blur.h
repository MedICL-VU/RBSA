#ifndef BLUR_H
#define BLUR_H

#include <itkSignedMaurerDistanceMapImageFilter.h>

#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkImageImport.h>
#include <vtkMarchingCubes.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkDecimatePro.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkXMLPolyDataWriter.h>


#include "BinaryImageMorphology.h"
#include "utils.h"

using BinaryContourImageFilterType = itk::BinaryContourImageFilter<UCharImageType, UCharImageType>;
UCharImageType::Pointer BinaryContourImage(UCharImageType::Pointer image);

using MinimumMaximumImageCalculatorType = itk::MinimumMaximumImageCalculator<FloatImageType>;
float GetMaximumImageValue(FloatImageType::Pointer image);

using SignedMaurerDistanceMapImageFilterType =
    itk::SignedMaurerDistanceMapImageFilter<UCharImageType, FloatImageType>;
FloatImageType::Pointer SignedDistanceTransform(IntImageType::Pointer image);

UCharImageType::Pointer GetBlurMask
(UCharImageType::Pointer labelMask, UCharImageType::Pointer brainMask,
 UCharImageType::Pointer skullStripMask, float stepSize = 0.4);


#endif
