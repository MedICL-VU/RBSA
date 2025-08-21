#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <string>
#include <limits>
#include <chrono>
#include <unordered_set>
#include <numeric>

#include "CLI11.hpp"

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

#include <itkAddImageFilter.h>
#include <itkAffineTransform.h>
#include <itkBinaryContourImageFilter.h>
#include <itkBinaryFillholeImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImageDuplicator.h>
#include <itkImageRegionConstIterator.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkMaskImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkMultiplyImageFilter.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkResampleImageFilter.h>
#include <itkStatisticsImageFilter.h>

#include <itkCommand.h>

#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkLine.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkCellArrayIterator.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkUnsignedIntArray.h>
#include <vtkIntArray.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkIdTypeArray.h>
#include <vtkIdList.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkTriangleFilter.h>

#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkMarchingCubes.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkTriangleFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkDecimatePro.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkStaticPointLocator.h>
#include <vtkStaticCellLocator.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCleanPolyData.h>


const int nDims = 3;

const std::vector<double> rasShift = {-1.0, -1.0, 1.0};

// ITK image types
using IndexType = itk::Index<nDims>;
using ContinuousIndexType = itk::ContinuousIndex<double, nDims>;
using FloatImageType = itk::Image<float, nDims>;
using VectorImageType = itk::Image<itk::Vector<float, nDims>, nDims>;
using IntImageType = itk::Image<int, nDims>;
using UCharImageType = itk::Image<unsigned char, nDims>;

using RBSAOutTuple = std::tuple<UCharImageType::Pointer,
                                VectorImageType::Pointer,
                                VectorImageType::Pointer>;

// Basic
template <typename TImage>
bool IsInside
(const typename TImage::PixelType& label, const std::vector<typename TImage::PixelType>& valid);
  

// IO
std::vector<std::string> SplitString(std::string str, const std::string& delimiter);

std::string GetFileExtension(std::string filename);

std::string GetBaseName(const std::string& filename);

bool CheckFileExtension(std::string filename, std::vector<std::string> exts);

template <typename TImage>
using ImageReaderType = itk::ImageFileReader<TImage>;
template <typename TImage>
typename TImage::Pointer ReadImage(const std::string& filename);

template <typename TImage>
using ImageWriterType = itk::ImageFileWriter<TImage>;
template <typename TImage>
void WriteImage(typename TImage::Pointer image, const std::string& filename);

vtkSmartPointer<vtkPolyData> ReadPolyData(const std::string& filename);

void WritePolyData(vtkSmartPointer<vtkPolyData> polyData, const std::string& filename);


// Misc. image filters
template <typename TImage>
using AddImageFilterType = itk::AddImageFilter<TImage>;
template <typename TImage>
typename TImage::Pointer AddImages
(typename TImage::Pointer image1, typename TImage::Pointer image2);

using BinaryFillHolesFilterType = itk::BinaryFillholeImageFilter<UCharImageType>;
UCharImageType::Pointer BinaryFillHoles
(UCharImageType::Pointer image, UCharImageType::PixelType value);

template<typename TInImage>
using BinaryThresholdImageFilterType = itk::BinaryThresholdImageFilter<TInImage, UCharImageType>;
template <typename TInImage>
typename UCharImageType::Pointer BinaryThresholdImage
(typename TInImage::Pointer image, const typename TInImage::PixelType& lower,
 const typename TInImage::PixelType& upper,
 const typename UCharImageType::PixelType& outsideValue = 0,
 const typename UCharImageType::PixelType& insideValue = 1);

template <typename TInImage, typename TOutImage>
using CastImageFilterType = itk::CastImageFilter<TInImage, TOutImage>;
template <typename TInImage, typename TOutImage>
typename TOutImage::Pointer CastImage(typename TInImage::Pointer image);

template <typename TImage>
using DuplicateImageFilterType = itk::ImageDuplicator<TImage>;
template <typename TImage>
typename TImage::Pointer DuplicateImage(typename TImage::Pointer image);

template <typename TInImage, typename TOutImage>
typename TOutImage::Pointer InitializeZeroFilledImage(typename TInImage::Pointer ref);

template <typename TImage>
bool IsLabelInImage(typename TImage::Pointer image, const typename TImage::PixelType label);

template <typename TImage>
using MaskImageFilterType = itk::MaskImageFilter<TImage, UCharImageType, TImage>;
template <typename TImage>
typename TImage::Pointer MaskImage(typename TImage::Pointer image, UCharImageType::Pointer mask);

template <typename TImage>
using MultiplyImageFilterType = itk::MultiplyImageFilter<TImage, TImage, TImage>;
template <typename TImage>
typename TImage::Pointer MultiplyImages
(typename TImage::Pointer image1, typename TImage::Pointer image2);

template <typename TImage>
using StatisticsImageFilterType = itk::StatisticsImageFilter<TImage>;
template <typename TImage>
typename TImage::PixelType ImageSum(typename TImage::Pointer image);

template <typename TImage>
using SubtractImageFilterType = itk::SubtractImageFilter<TImage, TImage, TImage>;
template <typename TImage>
typename TImage::Pointer SubtractImages
(typename TImage::Pointer image1, typename TImage::Pointer image2);


template <typename TImage>
using ImageRegionIteratorWithIndexType = itk::ImageRegionIteratorWithIndex<TImage>;

template <typename TImage>
using ImageRegionConstIteratorType = itk::ImageRegionConstIterator<TImage>;


template<typename TImage>
using LinearInterpolateType = itk::LinearInterpolateImageFunction<TImage, double>;

template<typename TImage>
using NearestNeighborInterpolateType = itk::NearestNeighborInterpolateImageFunction<TImage, double>;


template <typename TImage>
itk::ContinuousIndex<double, nDims> TransformNDimsDoubleToContinuousIndex
(typename TImage::Pointer image, double p0[nDims], bool convertFromRAS = false);

template <typename TImage>
typename TImage::IndexType TransformNDimsDoubleToIndex
(typename TImage::Pointer image, double p0[nDims], bool convertFromRAS = false);

template <typename TImage>
void TransformIndexToNDimsDouble
(typename TImage::Pointer image, typename TImage::IndexType index, double (&p0)[nDims]);


// VTK stuff
void GeneratePolyDataNormals
(vtkSmartPointer<vtkPolyData> polydata, bool overwrite = false, bool reverse = false);

template <typename TImage>
void TransformVTKPolyDataToITKImageSpace
(vtkSmartPointer<vtkPolyData> mesh, typename TImage::Pointer image);

template <typename TImage> struct VTKArrayFromITKImage;
template<> struct VTKArrayFromITKImage<IntImageType> { using type = vtkIntArray; };
template<> struct VTKArrayFromITKImage<UCharImageType> { using type = vtkUnsignedCharArray; };
template<> struct VTKArrayFromITKImage<FloatImageType> { using type = vtkFloatArray; };


void BinaryITKImageToVTKMesh
(UCharImageType::Pointer itkTargetImage, vtkSmartPointer<vtkPolyData> outputMesh);


// Other misc
void PrintDuration
  (std::chrono::steady_clock::time_point t0, std::string text, std::string time_type = "seconds");

void PrintNDimsDouble(double point[nDims]);

void VisualizePointSet
(vtkSmartPointer<vtkPoints> points, const std::string& filename,
 std::vector<vtkSmartPointer<vtkFloatArray>> pointDataFloatArrays = {});


#endif
