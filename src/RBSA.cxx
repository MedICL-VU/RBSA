#include "RBSA.h"

//--------------------------------------------------------------------------------------------------

RBSA::RBSA(){}


RBSAOutputTuple RBSA::GenerateAtrophyTransforms(unsigned int targetLabel)
{
  // Create label mask for original timepoint
  UCharImageType::Pointer origLabelMask =
    BinaryThresholdImage<IntImageType>(m_parcellation, targetLabel, targetLabel, 0, 1);

  // Get the blur mask data
  BlurMaskGenerator blurMaskGenerator;
  blurMaskGenerator.SetLabelMask(origLabelMask);
  blurMaskGenerator.SetBrainMask(m_brainMask);
  blurMaskGenerator.SetSkullStripMask(m_skullStripMask);
  blurMaskGenerator.SetStepSize(0.5);
  blurMaskGenerator.Generate();

  UCharImageType::Pointer blurMask = blurMaskGenerator.GetMask();
  
  // Crop necessary masks around blurMask and upsample resolution
  HighResCropFromReferenceMask highResCropFilter;
  highResCropFilter.SetReferenceImage(blurMask);
  highResCropFilter.SetCropRegionBuffer(2);
  highResCropFilter.SetResamplingFactor(m_upsamplingFactor);
  highResCropFilter.FindCropRegion();

  UCharImageType::Pointer blurMaskHighResCrop =
    highResCropFilter.Apply<UCharImageType>(blurMask);
  UCharImageType::Pointer origLabelMaskHighResCrop =
    highResCropFilter.Apply<UCharImageType>(origLabelMask);
  UCharImageType::Pointer origBrainMaskHighResCrop =
    highResCropFilter.Apply<UCharImageType>(m_brainMask);
  UCharImageType::Pointer skullStripMaskHighResCrop =
    highResCropFilter.Apply<UCharImageType>(m_skullStripMask);
  UCharImageType::Pointer wmMaskHighResCrop =
    highResCropFilter.Apply<UCharImageType>(m_wmMask);

  // Create cropped/upsampled masks for atrophied timepoint
  UCharImageType::Pointer atrophyLabelMaskHighResCrop =
    LocalizedAtrophy(origLabelMaskHighResCrop, wmMaskHighResCrop, m_nErosionIters);
  UCharImageType::Pointer atrophyBrainMaskHighResCrop =
    ReplaceLabelInImage(origBrainMaskHighResCrop,
                        origLabelMaskHighResCrop,
                        atrophyLabelMaskHighResCrop);

  // Register brain masks (OrigHighResCrop to AtrophyHighResCrop)
  Warper atrophyWarper;
  atrophyWarper.SetOriginalLabel(origBrainMaskHighResCrop);
  atrophyWarper.SetAtrophyLabel(atrophyBrainMaskHighResCrop);
  atrophyWarper.SetRegistrationMask(origLabelMaskHighResCrop);
  atrophyWarper.ComputeTransform();

  VectorImageType::Pointer warpHighResCrop = atrophyWarper.GetWarp();
  VectorImageType::Pointer warpInverseHighResCrop = atrophyWarper.GetInverseWarp();

  // Apply blur mask to displacement field
  blurMaskGenerator.BuildInterpolationMetaData(blurMaskHighResCrop, origLabelMaskHighResCrop);

  VectorImageType::Pointer warpMaskedHighResCrop =
    blurMaskGenerator.ApplyToWarp(warpHighResCrop);
  VectorImageType::Pointer warpInverseMaskedHighResCrop =
    blurMaskGenerator.ApplyToWarp(warpInverseHighResCrop);

  // Revert fields back to original dimension/resolution
  VectorImageType::Pointer warpMasked =
    highResCropFilter.Revert<VectorImageType>(warpMaskedHighResCrop);
  VectorImageType::Pointer warpInverseMasked =
    highResCropFilter.Revert<VectorImageType>(warpInverseMaskedHighResCrop);

  // Return
  return {blurMask, warpMasked, warpInverseMasked};
}



RBSAOutputTuple CombineLabelOutputs(std::vector<RBSAOutputTuple> labelData)
{
  VectorImageType::PixelType zeroVec = itk::NumericTraits<VectorImageType::PixelType>::ZeroValue();

  const size_t& nOutputs = labelData.size();
  if(labelData.size() == 1) return labelData.at(0);
    
  // Initialize composite images
  UCharImageType::Pointer ref = std::get<0>(labelData.at(0));
  UCharImageType::Pointer outMask = InitializeZeroFilledImage<UCharImageType, UCharImageType>(ref);
  VectorImageType::Pointer outWarp = InitializeZeroFilledImage<UCharImageType, VectorImageType>(ref);
  VectorImageType::Pointer outInverseWarp = DuplicateImage<VectorImageType>(outWarp);

  // Combine inputs
  std::vector<VectorImageType::PixelType> disps, inverseDisps;
  ImageRegionIteratorWithIndexType<VectorImageType> iterator
    (outWarp, outWarp->GetLargestPossibleRegion());
  iterator.GoToBegin();

  VectorImageType::PixelType pixel;
  
  while(!iterator.IsAtEnd()) {
    const VectorImageType::IndexType& index = iterator.GetIndex();
    bool isWarpPixel = false;
    
    // Get values from input warp if mask is nonzero
    for(const auto& data : labelData) {
      UCharImageType::Pointer mask = std::get<0>(data);
      VectorImageType::Pointer warp = std::get<1>(data);
      VectorImageType::Pointer inverseWarp = std::get<2>(data);

      if(mask->GetPixel(index) == 1) {
	isWarpPixel = true;
        disps.emplace_back(warp->GetPixel(index));
        inverseDisps.emplace_back(inverseWarp->GetPixel(index));
      }
    }
            
    // Calculate output image values
    if(isWarpPixel) {
      outMask->SetPixel(index, 1);

      // Warp
      pixel.Fill(0);
      for(const auto& disp : disps) {
        for(unsigned int d = 0; d < nDims; d++) {
          pixel[d] += disp[d];
        }
      }
      pixel /= static_cast<double>(disps.size());
      outWarp->SetPixel(index, pixel);

      // Inverse warp
      pixel.Fill(0);

      for(const auto& disp : inverseDisps) {
        for(unsigned int d = 0; d < nDims; d++) {
          pixel[d] += disp[d];
        }
      }
      pixel /= static_cast<double>(inverseDisps.size());
      outInverseWarp->SetPixel(index, pixel);
    }

    // Reset
    disps.clear();
    inverseDisps.clear();
    ++iterator;
  }

  // Output
  return {outMask, outWarp, outInverseWarp};
}
