#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
#include <limits>
#include <chrono>

#include "CLI11.hpp"

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

#include <itkAddImageFilter.h>
#include <itkBinaryContourImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkMaskImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkMultiplyImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkSubtractImageFilter.h>

#include <itkCommand.h>

#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkFloatArray.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkIdTypeArray.h>
#include <vtkIdList.h>


constexpr int nDims = 3;


// ITK image types
using FloatImageType = itk::Image<float, nDims>;
using FloatVectorImageType = itk::Image<itk::Vector<FloatImageType::PixelType, nDims>, nDims>;
using IntImageType = itk::Image<int, nDims>;
using UCharImageType = itk::Image<unsigned char, nDims>;


// IO
template <typename ImageType>
using ImageReaderType = itk::ImageFileReader<ImageType>;
template <typename ImageType>
typename ImageType::Pointer ReadImage(const std::string& filename);

template <typename ImageType>
using ImageWriterType = itk::ImageFileWriter<ImageType>;
template <typename ImageType>
void WriteImage(typename ImageType::Pointer image, const std::string& filename);


// Image math
template <typename InImage1Type, typename InImage2Type, typename OutImageType>
using AddImageFilterType = itk::AddImageFilter<InImage1Type, InImage2Type, OutImageType>;
template <typename InImage1Type, typename InImage2Type, typename OutImageType>
typename OutImageType::Pointer AddImages
(typename InImage1Type::Pointer image1, typename InImage2Type::Pointer image2);

/*
template <typename InImageType, typename OutImageType>
using BinaryContourImageFilterType = itk::BinaryContourImageFilter<InImageType, OutImageType>;
typename ImageType::Pointer BinaryContourImage(InImageType inImage, OutImageType outImage);
*/
template <typename InImageType, typename OutImageType>
using BinaryThresholdImageFilterType = itk::BinaryThresholdImageFilter<InImageType, OutImageType>;
template <typename InImageType, typename OutImageType>
typename OutImageType::Pointer BinaryThresholdImage
(typename InImageType::Pointer image, typename InImageType::PixelType lower,
 typename InImageType::PixelType upper, typename OutImageType::PixelType outsideValue = 0,
 typename OutImageType::PixelType insideValue = 1);

template <typename InImageType, typename MaskImageType, typename OutImageType>
using MaskImageFilterType = itk::MaskImageFilter<InImageType, MaskImageType, OutImageType>;
template <typename InImageType, typename MaskImageType, typename OutImageType>
typename OutImageType::Pointer MaskImage
(typename InImageType::Pointer image, typename MaskImageType::Pointer mask);

template <typename InImage1Type, typename InImage2Type, typename OutImageType>
using MultiplyImageFilterType = itk::MultiplyImageFilter<InImage1Type, InImage2Type, OutImageType>;
template <typename InImage1Type, typename InImage2Type, typename OutImageType>
typename OutImageType::Pointer MultiplyImages
(typename InImage1Type::Pointer image1, typename InImage2Type::Pointer image2);

template <typename InImage1Type, typename InImage2Type, typename OutImageType>
using SubtractImageFilterType = itk::SubtractImageFilter<InImage1Type, InImage2Type, OutImageType>;
template <typename InImage1Type, typename InImage2Type, typename OutImageType>
typename OutImageType::Pointer SubtractImages
(typename InImage1Type::Pointer image1, typename InImage2Type::Pointer image2);

// Misc. image filters
template <typename InImageType, typename OutImageType>
using CastImageFilterType = itk::CastImageFilter<InImageType, OutImageType>;
template <typename InImageType, typename OutImageType>
typename OutImageType::Pointer CastImage(typename InImageType::Pointer image);
/*
template <typename ImageType>
using ChangeInformationImageFilterType = itk::ChangeInformationImageFilter<ImageType>;

template <typename ImageType>
using ImageRegionIteratorWithIndexType = itk::ImageRegionIteratorWithIndex<ImageType>;

using LinearInterpolateUCharImageFunctionType = itk::LinearInterpolateImageFunction<UCharImageType>;

template <typename InImageType, typename OutImageType>
using RegionOfInterestImageFilterType = itk::RegionOfInterestImageFilter<InImageType, OutImageType>;
*/



// Other misc
void PrintDuration
  (std::chrono::steady_clock::time_point t0, std::string text, std::string time_type = "seconds");

#endif
