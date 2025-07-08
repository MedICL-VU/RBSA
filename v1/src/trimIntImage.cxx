#include <iostream>
#include <string>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkCommand.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkRegionOfInterestImageFilter.h"



const unsigned int nDims = 3;
typedef itk::Image<int,nDims> ImageType;
typedef itk::ImageFileReader<ImageType> ImageFileReaderType;
typedef itk::ImageFileWriter<ImageType> ImageFileWriterType;
typedef itk::ImageRegionIteratorWithIndex<ImageType> ImageIteratorType;



int main(int argc, char *argv[]) {

  //Verify command line arguments
  if( argc < 3 ) {
    std::cerr << "Usage: " << std::endl;
    std::cerr << "inputImage trimmedImage trimIndexX trimIndexY trimIndexZ trimIndexEndX trimIndexEndY trimIndexEndZ" << std::endl;
  }

  
  // Read data
  ImageFileReaderType::Pointer inputImageReader = ImageFileReaderType::New();
  inputImageReader->SetFileName(argv[1]);
  inputImageReader->Update();
  ImageType::Pointer fullImage = inputImageReader->GetOutput();
  

  // Trim the image
  ImageType::RegionType trimRegion;
  ImageType::IndexType trimIndex;
  ImageType::SizeType trimSize;
  for(unsigned int d = 0; d < nDims; d++){
    trimIndex[d] = atoi(argv[d+3]);
    trimSize[d] = atoi(argv[d+6]) - trimIndex[d];
  }
  trimRegion.SetIndex(trimIndex);
  trimRegion.SetSize(trimSize);
  
  typedef itk::RegionOfInterestImageFilter<ImageType,ImageType> RegionOfInterestImageFilterType;
  RegionOfInterestImageFilterType::Pointer trimImageRoiFilter = RegionOfInterestImageFilterType::New();
  trimImageRoiFilter->SetInput(fullImage);
  trimImageRoiFilter->SetRegionOfInterest(trimRegion);
  trimImageRoiFilter->Update();
  
    
  // Write image
  ImageFileWriterType::Pointer trimmedImageWriter = ImageFileWriterType::New();
  trimmedImageWriter->SetInput(trimImageRoiFilter->GetOutput());
  trimmedImageWriter->SetFileName(argv[2]);
  trimmedImageWriter->Update();

  return 0;
}
