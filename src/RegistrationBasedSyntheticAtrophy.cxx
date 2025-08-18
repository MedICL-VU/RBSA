#include "RegistrationBasedSyntheticAtrophy.h"

//--------------------------------------------------------------------------------------------------


RBSA::RBSA(){}


std::tuple<UCharImageType::Pointer, VectorImageType::Pointer, VectorImageType::Pointer>
RBSA::GenerateAtrophyTransforms(unsigned int targetLabel)
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
  return std::make_tuple(blurMask, warpMasked, warpInverseMasked);
}




int main(int argc, char * argv[])
{
  std::chrono::steady_clock::time_point t, t_total = std::chrono::steady_clock::now();
  
  // Declare everything
  std::string parcellationFileName;
  std::string skullStripMaskFileName;

  std::vector<std::string> inputTargetImageFileNames;
  std::vector<std::string> outputTargetImageFileNames;

  std::vector<std::string> inputTargetSurfaceFileNames;
  std::vector<std::string> outputTargetSurfaceFileNames;

  std::string inputLeftHemiTemplateFileName;
  std::string inputRightHemiTemplateFileName;
  std::string outputMSDDDirName;
  
  std::vector<int> targetLabels = {};
  std::vector<int> wmLabels = {};
  
  unsigned int nErosionIters = 4;
  float upsamplingFactor = 4.0;

  bool convertFromRAS = false;
  
  // Parse commandline args
  CLI::App app{"Input args:"};
  argv = app.ensure_utf8(argv);

  // Required input images
  app.add_option
    ("-p,--parcellation", parcellationFileName,
     "Input cortical parcellation image filename (cannot be .mgz filetype)")
    ->type_name("")->group("REQUIRED");
  app.add_option
    ("-m,--skullstrip_mask", skullStripMaskFileName,
     "Input skull strip mask filename (cannot be .mgz filetype)")
    ->type_name("")->group("REQUIRED");

  // Required labels (atrophy target and WM label)
  app.add_option
    ("-l,--target_labels", targetLabels,
     "Value of target labels for atrophy induction (must all be in same hemisphere)")
    ->type_name("")->group("REQUIRED");
  app.add_option
    ("-w,--wm_labels", wmLabels,
     "Value of wm label(s) ipsilateral to target label (default = 2 (left) or 41 (right)")
    ->type_name("")->group("OPTIONAL");
  
  // Erosion morphology parameters
  app.add_option
    ("-n,--num_atrophy_iterations", nErosionIters,
     "Number of binary morphology iterations to induce synthetic atrophy (default = 4)")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("-f,--upsampling_factor", upsamplingFactor,
     "Factor by which to upsample image resolution for atrophy induction (default = 4.0)")
    ->type_name("")->group("OPTIONAL");

  // Target image/surface data
  app.add_option
    ("-i,--input_images", inputTargetImageFileNames,
     "Input filenames of images in which to induce synthetic atrophy")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("-o,--output_images", outputTargetImageFileNames,
     "Output filenames of images with induced synthetic atrophy")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("-s,--input_surfaces", inputTargetSurfaceFileNames,
     "Input filenames of surfaces in which to induce synthetic atrophy")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("-t,--output_surfaces", outputTargetSurfaceFileNames,
     "Output filenames of surfaces with induced synthetic atrophy")
    ->type_name("")->group("OPTIONAL");
  app.add_flag
    ("--RAS", convertFromRAS,
     "Convert surface vertices from RAS to LPS coordinates")
    ->type_name("")->group("OPTIONAL");
  
  // Required for ground truth change (MSDD) calculation
  app.add_option
    ("--lh_template", inputLeftHemiTemplateFileName, 
     "Filename for left hemisphere template line (required for MSDD in left hemi; 1=WM, 2=GM")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--rh_template", inputRightHemiTemplateFileName,
     "Filename for left hemisphere template line (required for MSDD in right hemi; 1=WM, 2=GM")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--MSDD_outdir", outputMSDDDirName, "Path to directory for MSDD outputs")
    ->type_name("")->group("OPTIONAL");
  
  CLI11_PARSE(app, argc, argv);

  
  // Check input args
  if(parcellationFileName.empty()) {
    std::cerr << "Error: input cortical parcellation image filename (-p, --parc) is required \n";
    return 1;
  }
  
  if(skullStripMaskFileName.empty()) {
    std::cerr << "Error: input skull strip mask filename (-s, --skullstrip) is required \n";
    return 1;
  }

  if(targetLabels.empty()) {
    std::cerr << "Error: must input at least one target label for atrophy induction \n";
    return 1;
  }

  if(inputTargetImageFileNames.size() != outputTargetImageFileNames.size()) {
    std::cerr << "Error: must input equal number of input/output target images\n";
    return 1;
  }
  if(inputTargetSurfaceFileNames.size() != outputTargetSurfaceFileNames.size()) {
    std::cerr << "Error: must input equal number of input/output target surfaces\n";
    return 1;
  }

  bool doMSDD = !outputMSDDDirName.empty();
  if(!doMSDD &&
     (!inputLeftHemiTemplateFileName.empty() || !inputRightHemiTemplateFileName.empty())) {
    std::cerr << "Error: template provided for MSDD calculation, but no output path provided..\n";
    return 1;
  }
  if(doMSDD && (inputLeftHemiTemplateFileName.empty() && inputRightHemiTemplateFileName.empty())) {
    std::cerr << "Error: must provide left/right hemi template(s) for MSDD calculation \n";
    return 1;
  }

  // Main class
  RBSA atrophier;
  atrophier.SetNumberOfErosionIterations(nErosionIters);
  atrophier.SetUpsamplingFactor(upsamplingFactor);
  
  // Initialize masks
  IntImageType::Pointer parcellation = ReadImage<IntImageType>(parcellationFileName);
  atrophier.SetInputParcellation(parcellation);
  
  UCharImageType::Pointer brainMask =
    BinaryThresholdImage<IntImageType>(parcellation, 1, std::numeric_limits<int>::max(), 0, 1);
  atrophier.SetBrainMask(brainMask);

  UCharImageType::Pointer skullStripMask = ReadImage<UCharImageType>(skullStripMaskFileName);
  skullStripMask = BinaryFillHoles(skullStripMask);
  atrophier.SetSkullStripMask(skullStripMask);  

  UCharImageType::Pointer wmMask =
    InitializeZeroFilledImage<IntImageType, UCharImageType>(parcellation);
  for(const auto& label : wmLabels) {
    UCharImageType::Pointer temp =
      BinaryThresholdImage<IntImageType>(parcellation, label, label, 0, 1);
    wmMask = AddImages<UCharImageType>(wmMask, temp);
  }
  atrophier.SetWMMask(wmMask);


  // Generate atrophy inducing transforms for each input label
  const size_t nTargetLabels = targetLabels.size();

  std::vector<RBSAOutTuple> outputs;
  outputs.reserve(nTargetLabels);

  for(const auto& label : targetLabels) {
    if(!IsLabelInImage<IntImageType>(parcellation, label)) {
      std::cerr << "Label value " << label << " not found in input parcellation :(\n";
      continue;
    }
    
    std::cout << "Creating transforms for label " << label << "..." << std::endl;
    auto labelOutput = atrophier.GenerateAtrophyTransforms(label);
    outputs.emplace_back(labelOutput);
  }

  if(outputs.empty()) {
    std::cerr << "No atrophy warps generated... exiting now\n";
    return 1;
  }


  // Combine to create composite fields to deform all regions at once
  std::cout << "Combining all warps.." << std::endl;
  VectorImageType::Pointer imageWarp, surfaceWarp;
  
  if(nTargetLabels > 1) {
    std::array<VectorImageType::Pointer, 2> compositeWarps = CombineWarps(outputs);
    imageWarp = compositeWarps.at(0);
    surfaceWarp = compositeWarps.at(1);
  }
  else {
    imageWarp = std::get<1>(outputs.at(0));
    surfaceWarp = std::get<2>(outputs.at(0));
  }

  // Apply to target data
  std::cout << "Applying to target data.." << std::endl;
  
  for(size_t n = 0; n < inputTargetImageFileNames.size(); n++) {
    auto input = ReadImage<FloatImageType>(inputTargetImageFileNames.at(n));
    auto output = WarpImage(input, imageWarp);
    WriteImage<FloatImageType>(output, outputTargetImageFileNames.at(n));
  }

  if(inputTargetSurfaceFileNames.size() > 0) {
    std::vector<int> parcLabels;
    for(const auto& label : targetLabels) {
      parcLabels.emplace_back(label);
    }
    IntImageType::Pointer targetParc =
      MakeTargetLabelParcellation<IntImageType>(parcellation, parcLabels);

    for(size_t n = 0; n < inputTargetSurfaceFileNames.size(); n++) {
      auto input = ReadPolyData(inputTargetSurfaceFileNames.at(n));
      auto output = WarpSurface(input, surfaceWarp, convertFromRAS);
      ParcellateSurface<IntImageType>(output, targetParc, "TargetLabels", parcLabels,
				      convertFromRAS);
      WritePolyData(output, outputTargetSurfaceFileNames.at(n));
    }
  }

  // Measure true induced change (MSDD) ?
  if(outputMSDDDirName.empty()) return 0;
  std::cout << "Calculating MSDD" << std::endl;
  
  UCharImageType::Pointer left = ReadImage<UCharImageType>(inputLeftHemiTemplateFileName);
  UCharImageType::Pointer right = ReadImage<UCharImageType>(inputRightHemiTemplateFileName);
  
  MSDD msdd;
  msdd.SetTargetLabels(targetLabels);
  msdd.SetCorticalParcellation(parcellation);
  msdd.SetWarp(surfaceWarp);
  msdd.SetLeftHemiImage(left);
  msdd.SetRightHemiImage(right);
  msdd.Calculate();
  msdd.Write(outputMSDDDirName);

  return 0;
  
}
