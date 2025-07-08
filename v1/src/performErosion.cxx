#include <iostream>
#include <string>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkBinaryErodeImageFilter.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkAddImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkCommand.h"



int main(int argc, char *argv[]) {

  //Verify command line arguments
  if( argc < 5 ) {
    std::cerr << "Usage: " << std::endl;
    std::cerr << "originalLabelMask wmMask erodedLabelMask kernelRad writeMasks?" << std::endl;
  }

  const unsigned int nDims = 3;
  const int wmValue = 2;
  const int kernelRad = atoi(argv[4]);
  const unsigned int writeMasks = atoi(argv[5]);


  // Read label image
  typedef itk::Image < int, nDims > ImageType;
  typedef itk::ImageFileReader < ImageType > ImageFileReaderType;
  ImageFileReaderType::Pointer originalRoiReader = ImageFileReaderType::New();
  originalRoiReader->SetFileName(argv[1]);
  originalRoiReader->Update();
  ImageType::Pointer origMask = originalRoiReader->GetOutput();
  
  ImageType::SizeType imageSize = origMask->GetLargestPossibleRegion().GetSize();


  // Set up kernel
  typedef itk::BinaryBallStructuringElement < ImageType::PixelType, nDims > StructuringElementType;
  StructuringElementType kernel;
  kernel.SetRadius(kernelRad);
  kernel.CreateStructuringElement();


  // Set up eroder
  typedef itk::BinaryErodeImageFilter < ImageType, ImageType, StructuringElementType > BinaryErodeType;
  BinaryErodeType::Pointer eroder = BinaryErodeType::New();
  eroder->SetInput(origMask);
  eroder->SetKernel(kernel);
  eroder->SetErodeValue(1);
  eroder->Update();

  // Dilate white matter
  ImageFileReaderType::Pointer wmReader = ImageFileReaderType::New();
  wmReader->SetFileName(argv[2]);
  wmReader->Update();
  
  typedef itk::BinaryDilateImageFilter < ImageType, ImageType, StructuringElementType > BinaryDilateType;
  BinaryDilateType::Pointer dilateMask = BinaryDilateType::New();
  dilateMask->SetInput(wmReader->GetOutput());
  dilateMask->SetKernel(kernel);
  dilateMask->SetDilateValue(1);
  dilateMask->Update();


  // Get final binary mask
  typedef itk::SubtractImageFilter < ImageType, ImageType, ImageType > SubtractType;
  SubtractType::Pointer origMinusErodeMask = SubtractType::New();
  origMinusErodeMask->SetInput1(origMask);
  origMinusErodeMask->SetInput2(eroder->GetOutput());
  origMinusErodeMask->Update();

  typedef itk::MultiplyImageFilter < ImageType, ImageType, ImageType > MultiplyType;
  MultiplyType::Pointer origMinusErodeTimesDilateMask = MultiplyType::New();
  origMinusErodeTimesDilateMask->SetInput1(origMinusErodeMask->GetOutput());
  origMinusErodeTimesDilateMask->SetInput2(dilateMask->GetOutput());
  origMinusErodeTimesDilateMask->Update();

  typedef itk::AddImageFilter < ImageType, ImageType, ImageType > AddType;
  AddType::Pointer finalMask = AddType::New();
  finalMask->SetInput1(origMinusErodeTimesDilateMask->GetOutput());
  finalMask->SetInput2(eroder->GetOutput());
  finalMask->Update();

  typedef itk::BinaryThresholdImageFilter < ImageType, ImageType > BinaryThresholdImageFilterType;
  BinaryThresholdImageFilterType::Pointer thresholder = BinaryThresholdImageFilterType::New();
  thresholder->SetInput(finalMask->GetOutput());
  thresholder->SetLowerThreshold(1);
  thresholder->SetUpperThreshold(1);
  thresholder->SetOutsideValue(0);
  thresholder->SetInsideValue(1);
  thresholder->Update();

  
  typedef itk::ImageFileWriter < ImageType > ImageFileWriterType;
  ImageFileWriterType::Pointer finalMaskWriter = ImageFileWriterType::New();
  finalMaskWriter->SetInput(thresholder->GetOutput());
  std::string finalMaskFileName = argv[3];
  finalMaskWriter->SetFileName(finalMaskFileName);
  finalMaskWriter->Update();


  // Write out all the masks (optional)

  if(writeMasks==1) {
    ImageFileWriterType::Pointer erodeMaskWriter = ImageFileWriterType::New();
    erodeMaskWriter->SetInput(eroder->GetOutput());
    std::string erodeFileName = "erodeMask.nii.gz";
    erodeMaskWriter->SetFileName(erodeFileName);
    erodeMaskWriter->Update();
    
    ImageFileWriterType::Pointer wmMaskWriter = ImageFileWriterType::New();
    wmMaskWriter->SetInput(wmReader->GetOutput());
    std::string wmFileName = "wmMask.nii.gz";
    wmMaskWriter->SetFileName(wmFileName);
    wmMaskWriter->Update();

    ImageFileWriterType::Pointer dilateMaskWriter = ImageFileWriterType::New();
    dilateMaskWriter->SetInput(dilateMask->GetOutput());
    std::string dilateFileName = "dilateMask.nii.gz";
    dilateMaskWriter->SetFileName(dilateFileName);
    dilateMaskWriter->Update();

    ImageFileWriterType::Pointer origMinusErodeMaskWriter = ImageFileWriterType::New();
    origMinusErodeMaskWriter->SetInput(origMinusErodeMask->GetOutput());
    std::string origMinusErodeMaskFileName = "origMinusErodeMask.nii.gz";
    origMinusErodeMaskWriter->SetFileName(origMinusErodeMaskFileName);
    origMinusErodeMaskWriter->Update();
    
    ImageFileWriterType::Pointer origMinusErodeTimesDilateMaskWriter = ImageFileWriterType::New();
    origMinusErodeTimesDilateMaskWriter->SetInput(origMinusErodeTimesDilateMask->GetOutput());
    std::string origMinusErodeTimesDilateMaskFileName = "origMinusErodeTimesDilateMask.nii.gz";
    origMinusErodeTimesDilateMaskWriter->SetFileName(origMinusErodeTimesDilateMaskFileName);
    origMinusErodeTimesDilateMaskWriter->Update();
  }


  return 0;
}
