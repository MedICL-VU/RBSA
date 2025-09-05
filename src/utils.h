#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <string>
#include <limits>
#include <chrono>
#include <numeric>
#include <type_traits>

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
#include <itkVectorMagnitudeImageFilter.h>

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


const float eps = 1e-5;
const int nDims = 3;
const int intInf = std::numeric_limits<int>::max();
const std::vector<double> rasShift = {-1.0, -1.0, 1.0};


// ITK image types
template <typename TImage>
using TPointer = typename TImage::Pointer;

template <typename TImage>
using TPixel = typename TImage::PixelType;

template <typename TImage>
using TIndex = typename TImage::IndexType;

template <typename TImage>
using TRegion = typename TImage::RegionType;

using ContinuousIndexType = itk::ContinuousIndex<double, nDims>;

using VectorImageType = itk::Image<itk::Vector<float, nDims>, nDims>;
using IntImageType = itk::Image<int, nDims>;
using UCharImageType = itk::Image<unsigned char, nDims>;

const itk::Vector<float, nDims> vZero = itk::NumericTraits<float>::ZeroValue();
using FloatImageType = itk::Image<float, nDims>;


// Basic
template <typename TImage>
bool IsInside
(const TPixel<TImage>& label, const std::vector<TPixel<TImage>>& valid);
  

// IO
std::vector<std::string> SplitString(std::string str, const std::string& delimiter);

std::string GetFileExtension(std::string filename);

std::string GetBaseName(const std::string& filename);

bool CheckFileExtension(std::string filename, std::vector<std::string> exts);

template <typename TImage>
using ImageReaderType = itk::ImageFileReader<TImage>;
template <typename TImage>
TPointer<TImage> ReadImage(const std::string& filename);

template <typename TImage>
using ImageWriterType = itk::ImageFileWriter<TImage>;
template <typename TImage>
void WriteImage(TPointer<TImage> image, const std::string& filename);

vtkSmartPointer<vtkPolyData> ReadPolyData(const std::string& filename);

void WritePolyData(vtkSmartPointer<vtkPolyData> polyData, const std::string& filename);


/* 
   Misc. image filters
*/
// Add images
template <typename TInImage1, typename TInImage2, typename TOutImage>
using AddImageFilterType = itk::AddImageFilter<TInImage1, TInImage2, TOutImage>;

template <typename TImage>
TPointer<TImage> AddImages(TPointer<TImage> image1, TPointer<TImage> image2);

template <typename TInImage1, typename TInImage2, typename TOutImage>
TPointer<TOutImage> AddImages(TPointer<TInImage1> image1, TPointer<TInImage2> image2);

template <typename TImage1, typename TImage2>
void AddImagesInPlace(TPointer<TImage1>& image1, TPointer<TImage2> image2);

template <typename TImage>
void AddImagesInPlace(TPointer<TImage>& image1, TPointer<TImage> image2);


// Binary threshold image
template<typename TInImage>
using BinaryThresholdImageFilterType = itk::BinaryThresholdImageFilter<TInImage, UCharImageType>;

using VectorMagnitudeImageFilterType =
  itk::VectorMagnitudeImageFilter<VectorImageType, FloatImageType>;

template <typename TInImage>
TPointer<UCharImageType> BinaryThresholdImage(TPointer<TInImage> image,
					      const TPixel<TInImage>& lower,
					      const TPixel<TInImage>& upper,
					      const TPixel<UCharImageType>& outsideValue,
					      const TPixel<UCharImageType>& insideValue);

template <typename TInImage>
TPointer<UCharImageType> BinaryThresholdImage(TPointer<TInImage> image,
                                              const TPixel<TInImage>& lower,
                                              const TPixel<TInImage>& upper);

template <typename TInImage>
TPointer<UCharImageType> BinaryThresholdImage(TPointer<TInImage> image);

TPointer<UCharImageType> BinaryThresholdVectorImage(TPointer<VectorImageType> image);


// Binary threshold image (in-place)
void BinaryThresholdImageInPlace(TPointer<UCharImageType>& image,
				 const TPixel<UCharImageType>& lower,
				 const TPixel<UCharImageType>& upper,
				 const TPixel<UCharImageType>& outsideValue,
				 const TPixel<UCharImageType>& insideValue);

void BinaryThresholdImageInPlace(TPointer<UCharImageType>& image,
				 const TPixel<UCharImageType>& lower,
				 const TPixel<UCharImageType>& upper);

void BinaryThresholdImageInPlace(TPointer<UCharImageType>& image);


// Cast image
template <typename TInImage, typename TOutImage>
using CastImageFilterType = itk::CastImageFilter<TInImage, TOutImage>;

template <typename TInImage, typename TOutImage>
TPointer<TOutImage> CastImage(TPointer<TInImage> image);


// Duplicate image
template <typename TImage>
using DuplicateImageFilterType = itk::ImageDuplicator<TImage>;

template <typename TImage>
TPointer<TImage> DuplicateImage(TPointer<TImage> image);


// Initialize new image
template <typename TInImage, typename TOutImage>
TPointer<TOutImage> InitializeZeroFilledImage(TPointer<TInImage> ref);


// Check if label is in image
template <typename TImage>
bool IsLabelInImage(TPointer<TImage> image, const TPixel<TImage> label);


// Mask imag
template <typename TImage>
using MaskImageFilterType = itk::MaskImageFilter<TImage, UCharImageType, TImage>;

template <typename TImage>
TPointer<TImage> MaskImage(TPointer<TImage> image, TPointer<UCharImageType> mask);


// Multiple images
template <typename TInImage1, typename TInImage2, typename TOutImage>
using MultiplyImageFilterType = itk::MultiplyImageFilter<TInImage1, TInImage2, TOutImage>;

template <typename TInImage1, typename TInImage2, typename TOutImage>
TPointer<TOutImage> MultiplyImages(TPointer<TInImage1> image1, TPointer<TInImage2> image2);

template <typename TImage>
TPointer<TImage> MultiplyImages(TPointer<TImage> image1, TPointer<TImage> image2);

template <typename TImage1, typename TImage2>
void MultiplyImagesInPlace(TPointer<TImage1>& image1, TPointer<TImage2> image2);

template <typename TImage>
void MultiplyImagesInPlace(TPointer<TImage>& image1, TPointer<TImage> image2);


// Get sum of voxels in image
template <typename TImage>
using StatisticsImageFilterType = itk::StatisticsImageFilter<TImage>;

template <typename TImage>
float ImageSum(TPointer<TImage> image);


// Subtract images
template <typename TInImage1, typename TInImage2, typename TOutImage>
using SubtractImageFilterType = itk::SubtractImageFilter<TInImage1, TInImage2, TOutImage>;

template <typename TInImage1, typename TInImage2, typename TOutImage>
TPointer<TOutImage> SubtractImages(TPointer<TInImage1> image1, TPointer<TInImage2> image2);

template <typename TImage>
TPointer<TImage> SubtractImages(TPointer<TImage> image1, TPointer<TImage> image2);

template <typename TImage1, typename TImage2>
void SubtractImagesInPlace(TPointer<TImage1>& image1, TPointer<TImage2> image2);

template <typename TImage>
void SubtractImagesInPlace(TPointer<TImage>& image1, TPointer<TImage> image2);


// Image iterators
template <typename TImage>
using ImageRegionIteratorWithIndexType = itk::ImageRegionIteratorWithIndex<TImage>;

template <typename TImage>
using ImageRegionConstIteratorType = itk::ImageRegionConstIterator<TImage>;


// Image interpolators
template<typename TImage>
using LinearInterpolateType = itk::LinearInterpolateImageFunction<TImage, double>;

template<typename TImage>
using NearestNeighborInterpolateType = itk::NearestNeighborInterpolateImageFunction<TImage, double>;


// Image index to double (and vice versa)
template <typename TImage>
ContinuousIndexType TransformNDimsDoubleToContinuousIndex(TPointer<TImage> image,
							  double p0[nDims],
							  bool isRAS = false);

template <typename TImage>
TIndex<TImage> TransformNDimsDoubleToIndex(TPointer<TImage> image,
					   double p0[nDims],
					   bool isRAS = false);

template <typename TImage>
void TransformIndexToNDimsDouble(TPointer<TImage> image,
				 TIndex<TImage> index,
				 double (&p0)[nDims]);


// VTK stuff
void GeneratePolyDataNormals(vtkSmartPointer<vtkPolyData> polydata,
			     bool overwrite = false,
			     bool reverse = false);

template <typename TImage>
void TransformVTKPolyDataToITKImageSpace(vtkSmartPointer<vtkPolyData> mesh, TPointer<TImage> image);

template <typename TImage> struct VTKArrayFromITKImage;
template<> struct VTKArrayFromITKImage<IntImageType> { using type = vtkIntArray; };
template<> struct VTKArrayFromITKImage<UCharImageType> { using type = vtkUnsignedCharArray; };
template<> struct VTKArrayFromITKImage<FloatImageType> { using type = vtkFloatArray; };


void BinaryITKImageToVTKMesh(TPointer<UCharImageType> itkTargetImage,
			     vtkSmartPointer<vtkPolyData> outputMesh);


// Other misc
void PrintDuration(std::chrono::steady_clock::time_point t0,
		   std::string text,
		   std::string time_type = "seconds");

void PrintNDimsDouble(double point[nDims]);

void VisualizePointSet(vtkSmartPointer<vtkPoints> points,
		       const std::string& filename,
		       std::vector<vtkSmartPointer<vtkFloatArray>> pointDataFloatArrays = {});


#endif
