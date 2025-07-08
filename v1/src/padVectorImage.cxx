#include <iostream>
#include <string>
#include "itkImage.h"
#include "itkVector.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkCommand.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkChangeInformationImageFilter.h"
#include "itkConstantPadImageFilter.h"



const unsigned int nDims = 3;
typedef itk::Image<int,nDims> ImageType;
typedef itk::Vector<double,nDims> VectorType;
typedef itk::Image<VectorType,nDims> VectorImageType;

typedef itk::ImageFileReader<ImageType> ImageFileReaderType;
typedef itk::ImageFileReader<VectorImageType> VectorImageFileReaderType;

typedef itk::ImageFileWriter<ImageType> ImageFileWriterType;
typedef itk::ImageFileWriter<VectorImageType> VectorImageFileWriterType;

typedef itk::ImageRegionIteratorWithIndex<ImageType> ImageIteratorType;
typedef itk::ImageRegionIteratorWithIndex<VectorImageType> VectorImageIteratorType;



int main(int argc, char *argv[]) {

  //Verify command line arguments
  if( argc < 3 ) {
    std::cerr << "Usage: " << std::endl;
    std::cerr << "referenceImage targetImage outputImage lowerX lowerY lowerZ upperX upperY upperZ" << std::endl;
  }

  
  // Read data
  ImageFileReaderType::Pointer referenceImageReader = ImageFileReaderType::New();
  referenceImageReader->SetFileName(argv[1]);
  referenceImageReader->Update();
  ImageType::Pointer referenceImage = referenceImageReader->GetOutput();
  
  VectorImageFileReaderType::Pointer targetImageReader = VectorImageFileReaderType::New();
  targetImageReader->SetFileName(argv[2]);
  targetImageReader->Update();
  VectorImageType::Pointer targetImage = targetImageReader->GetOutput();


  // Create new image with ref image
  VectorImageType::IndexType start = referenceImage->GetLargestPossibleRegion().GetIndex();
  VectorImageType::SizeType size = referenceImage->GetLargestPossibleRegion().GetSize();
  VectorImageType::RegionType padImageRegion;
  padImageRegion.SetIndex(start);
  padImageRegion.SetSize(size);

  VectorImageType::Pointer padImage = VectorImageType::New();
  padImage->SetRegions(padImageRegion);
  padImage->SetSpacing(referenceImage->GetSpacing());
  padImage->SetOrigin(referenceImage->GetOrigin());
  padImage->SetDirection(referenceImage->GetDirection());
  padImage->Allocate();
  
  VectorImageType::PixelType zeroVector;
  zeroVector.Fill(0.0);
  padImage->FillBuffer(zeroVector);


  // Fill in image with target data
  int lowerBound[nDims], upperBound[nDims];
  for(unsigned int d = 0; d < nDims; d++){
    lowerBound[d] = atoi(argv[d+4]);
    upperBound[d] = atoi(argv[d+7]);
  }
  
  VectorImageIteratorType targetImageIterator(targetImage,targetImage->GetLargestPossibleRegion());
  targetImageIterator.GoToBegin();
  VectorImageType::IndexType padIndex;
  VectorImageType::IndexType targetIndex;
  VectorImageType::PixelType pixel;
  while(!targetImageIterator.IsAtEnd()) {
    targetIndex = targetImageIterator.GetIndex();
    for(unsigned int d = 0; d < nDims; d++) {
      padIndex[d] = targetIndex[d] + lowerBound[d];
    }
    padImage->SetPixel(padIndex,targetImage->GetPixel(targetIndex));
    ++targetImageIterator;
  }
  
  
  // Write image
  VectorImageFileWriterType::Pointer padImageWriter = VectorImageFileWriterType::New();
  padImageWriter->SetInput(padImage);
  padImageWriter->SetFileName(argv[3]);
  padImageWriter->Update();

  
  return 0;
}
