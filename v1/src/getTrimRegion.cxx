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
    std::cerr << "referenceImage trimMargin" << std::endl;
  }

  
  // Read data
  ImageFileReaderType::Pointer referenceImageReader = ImageFileReaderType::New();
  referenceImageReader->SetFileName(argv[1]);
  referenceImageReader->Update();
  ImageType::Pointer referenceImage = referenceImageReader->GetOutput();

  int trimMargin = atoi(argv[2]);
  

  // Get trim ROI
  ImageType::IndexType referenceIndex = referenceImage->GetLargestPossibleRegion().GetIndex();
  ImageType::SizeType referenceSize = referenceImage->GetLargestPossibleRegion().GetSize();
  ImageType::IndexType trimIndex;
  for(int d = 0; d < nDims; d++) {
    trimIndex[d] = 0;
  }
  bool setIndex = false;

  ImageIteratorType referenceImageIterator(referenceImage,referenceImage->GetLargestPossibleRegion());
  referenceImageIterator.GoToBegin();
  while(!referenceImageIterator.IsAtEnd()) {
    if(referenceImageIterator.Get()==1 && !setIndex) {
      trimIndex = referenceImageIterator.GetIndex();
      setIndex = true;
    } else if(referenceImageIterator.Get()==1)
      for(unsigned int d = 0; d < nDims; d++) {
	if(referenceImageIterator.GetIndex()[d] < trimIndex[d]){
	  trimIndex[d] = referenceImageIterator.GetIndex()[d];
	}
      }
    ++referenceImageIterator;
  }
  
  ImageType::IndexType trimIndexEnd;
  for(int d = 0; d < nDims; d++) {
    trimIndexEnd[d] = 0;
  }
  bool setIndexEnd = false;

  referenceImageIterator.GoToReverseBegin();
  while(!referenceImageIterator.IsAtReverseEnd()) {
    if(referenceImageIterator.Get()==1 && !setIndexEnd) {
      trimIndexEnd = referenceImageIterator.GetIndex();
      setIndexEnd = true;
    } else if(referenceImageIterator.Get()==1)
      for(unsigned int d = 0; d < nDims; d++) {
        if(referenceImageIterator.GetIndex()[d] > trimIndexEnd[d]){
          trimIndexEnd[d] = referenceImageIterator.GetIndex()[d];
        }
      }
    --referenceImageIterator;
  }

  ImageType::IndexType trimIndexNew;
  ImageType::IndexType trimIndexEndNew;
  ImageType::SizeType trimSize;
  int lowerBound, upperBound;

  /*
for(unsigned int d = 0; d < nDims; d++) {
    trimIndexNew[d] = trimIndex[d] -= trimMargin;
  */

 
  for(unsigned int d = 0; d < nDims; d++) {
    lowerBound = trimIndex[d] - trimMargin;
    if(lowerBound < referenceIndex[d]) {
      trimIndexNew[d] = referenceIndex[d];
    } else {
      trimIndexNew[d] = lowerBound;
    }
    upperBound = trimIndexEnd[d] + trimMargin;
    if(upperBound > referenceSize[d]) {
      trimIndexEndNew[d] = referenceSize[d];
    } else {
      trimIndexEndNew[d] = upperBound;
    }
    trimSize[d] = trimIndexEndNew[d] - trimIndexNew[d];
  }

    
  // Output commandline variables
  std::cout << trimIndexNew[0] << " " << trimIndexNew[1] << " " << trimIndexNew[2] << " " << std::flush;
  std::cout << trimIndexEndNew[0] << " " << trimIndexEndNew[1] << " " << trimIndexEndNew[2] << std::endl;

  
  return 0;
}
