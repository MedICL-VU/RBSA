#include <iostream>
#include <vector>
#include <numeric>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIteratorWithIndex.h"



const unsigned int nDims = 3;
typedef itk::Vector<double,nDims> VectorType;
typedef itk::Image<VectorType,nDims> VectorImageType;
typedef itk::ImageFileReader<VectorImageType> VectorImageFileReaderType;
typedef itk::ImageFileWriter<VectorImageType> VectorImageFileWriterType;
typedef itk::ImageRegionIteratorWithIndex<VectorImageType> ImageIteratorType;


VectorImageType::Pointer readVectorImage(std::string filename) {
  VectorImageFileReaderType::Pointer vectorImageReader = VectorImageFileReaderType::New();
  vectorImageReader->SetFileName(filename.c_str());
  vectorImageReader->Update();

  VectorImageType::Pointer outputVectorImage = vectorImageReader->GetOutput();
  return outputVectorImage;
}


std::vector<VectorType> getFieldValues(std::vector<VectorImageType::Pointer> fieldImages,VectorImageType::IndexType index){
  unsigned int n = fieldImages.size();
  std::vector<VectorType> fieldValues;
  VectorImageType::Pointer field;
  
  for(unsigned int i = 0; i < n; i++){
    field = fieldImages.at(i);
    fieldValues.push_back(field->GetPixel(index));
  }
  return fieldValues;
}


int main(int argc, char * argv[]) {
  
  unsigned int nImages = argc - 1;
  std::string filename;
  std::vector<VectorImageType::Pointer> fieldImages;

  for(unsigned int i = 1; i < nImages; i++){
    filename = argv[i];
    VectorImageType::Pointer field = readVectorImage(filename);
    fieldImages.push_back(field);
  }

  VectorImageType::Pointer referenceField = fieldImages.at(0);
  VectorImageType::Pointer outputField = referenceField;
  double zeroVector[nDims] = {0};
  outputField->FillBuffer(zeroVector);

  std::vector<VectorType> fieldValues;
  ImageIteratorType fieldIterator(outputField,outputField->GetLargestPossibleRegion());
  fieldIterator.GoToBegin();
  unsigned int count = 0;
  std::vector<VectorType> nonZeroFieldValues;
  while(!fieldIterator.IsAtEnd()){
    fieldValues = getFieldValues(fieldImages,fieldIterator.GetIndex());
    for(unsigned int i = 0; i < fieldValues.size(); i++){
      if(fieldValues.at(i) != zeroVector) {
	nonZeroFieldValues.push_back(fieldValues.at(i));
      }
    }
    if(nonZeroFieldValues.size() == 1) {
      outputField->SetPixel(fieldIterator.GetIndex(),nonZeroFieldValues.at(0));
    } else if(nonZeroFieldValues.size() > 1) {
      VectorType newFieldValue;
      for(unsigned int d = 0; d < nDims; d++) {
	double sum = 0;
	for(unsigned int i = 0; i < nonZeroFieldValues.size(); i++){
	  sum += nonZeroFieldValues.at(i)[d];
	}
	newFieldValue[d] = sum/nonZeroFieldValues.size();
      }
      outputField->SetPixel(fieldIterator.GetIndex(),newFieldValue);
    }
    nonZeroFieldValues.clear();
    ++fieldIterator;
  }

  VectorImageFileWriterType::Pointer outputFieldWriter = VectorImageFileWriterType::New();
  outputFieldWriter->SetInput(outputField);
  outputFieldWriter->SetFileName(argv[argc-1]);
  outputFieldWriter->Update();

  return 0;
}
