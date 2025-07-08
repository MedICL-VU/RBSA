#include <iostream>
#include <string>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkCommand.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkChangeInformationImageFilter.h"
#include "itkConstantPadImageFilter.h"



const unsigned int nDims = 3;
typedef itk::Image<int,nDims> ImageType;
typedef itk::ImageFileReader<ImageType> ImageFileReaderType;
typedef itk::ImageFileWriter<ImageType> ImageFileWriterType;
typedef itk::ImageRegionIteratorWithIndex<ImageType> ImageIteratorType;






int main(int argc, char *argv[]) {

  //Verify command line arguments
  if( argc < 3 ) {
    std::cerr << "Usage: " << std::endl;
    std::cerr << "referenceImage targetImage outputImage lowerX lowerY lowerZ upperX upperY upperZ " << std::endl;
  }

  
  // Read data
  ImageFileReaderType::Pointer referenceImageReader = ImageFileReaderType::New();
  referenceImageReader->SetFileName(argv[1]);
  referenceImageReader->Update();
  ImageType::Pointer referenceImage = referenceImageReader->GetOutput();
  
  ImageFileReaderType::Pointer targetImageReader = ImageFileReaderType::New();
  targetImageReader->SetFileName(argv[2]);
  targetImageReader->Update();
  ImageType::Pointer targetImage = targetImageReader->GetOutput();


  // Create new image with ref image
  ImageType::IndexType start = referenceImage->GetLargestPossibleRegion().GetIndex();
  ImageType::SizeType size = referenceImage->GetLargestPossibleRegion().GetSize();
  ImageType::RegionType padImageRegion;
  padImageRegion.SetIndex(start);
  padImageRegion.SetSize(size);

  ImageType::Pointer padImage = ImageType::New();
  padImage->SetRegions(padImageRegion);
  padImage->SetSpacing(referenceImage->GetSpacing());
  padImage->SetOrigin(referenceImage->GetOrigin());
  padImage->SetDirection(referenceImage->GetDirection());
  padImage->Allocate();

  padImage->FillBuffer(itk::NumericTraits<int>::Zero);

  // Fill in image with either reference or target data
  int lowerBound[nDims], upperBound[nDims];
  for(unsigned int d = 0; d < nDims; d++){
    lowerBound[d] = atoi(argv[d+4]);
    upperBound[d] = atoi(argv[d+7]);
  }

  ImageIteratorType targetImageIterator(targetImage,targetImage->GetLargestPossibleRegion());
  targetImageIterator.GoToBegin();
  ImageType::IndexType padIndex;
  ImageType::IndexType targetIndex;
  ImageType::PixelType pixel;
  while(!targetImageIterator.IsAtEnd()) {
    targetIndex = targetImageIterator.GetIndex();
    for(unsigned int d = 0; d < nDims; d++) {
      padIndex[d] = targetIndex[d] + lowerBound[d];
    }
    padImage->SetPixel(padIndex,targetImage->GetPixel(targetIndex));
    ++targetImageIterator;
  }
  
  // Write image
  ImageFileWriterType::Pointer padImageWriter = ImageFileWriterType::New();
  padImageWriter->SetInput(padImage);
  padImageWriter->SetFileName(argv[3]);
  padImageWriter->Update();

  return 0;
}
