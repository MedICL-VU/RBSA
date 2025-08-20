#include "BinaryImageMorphology.h"
#include "BlurMask.h"
#include "HighResCrop.h"
#include "MSDD.h"
#include "ParcellateSurface.h"
#include "RBSA.h"
#include "Warp.h"

#include "utils.h"



int main(int argc, char * argv[])
{
  // Declare everything
  std::string parcellationFileName;
  std::string skullStripMaskFileName;
  std::string outputDirName;

  std::vector<int> targetLabels;
  std::vector<int> wmLabels = {2, 41};

  unsigned int nErosionIters = 4;
  float upsamplingFactor = 4.0;
  
  std::vector<std::string> inputTargetImageFileNames;
  std::vector<std::string> outputTargetImageFileNames;
  std::vector<std::string> inputTargetGMFileNames;
  std::vector<std::string> outputTargetGMFileNames;
  std::vector<std::string> inputTargetWMFileNames;
  std::vector<std::string> outputTargetWMFileNames;
  bool convertFromRAS = false;

  std::string inputHemisTemplateFileName;
  std::vector<int> lhGMTemplateLabels;
  std::vector<int> lhWMTemplateLabels;
  std::vector<int> rhGMTemplateLabels;
  std::vector<int> rhWMTemplateLabels;
  
  // Parse commandline args
  CLI::App app{"Input args:"};
  argv = app.ensure_utf8(argv);

  
  // Required args
  app.add_option
    ("-l,--target_labels", targetLabels,
     "Value of target labels for atrophy induction (always required)")
    ->type_name("");
  app.add_option
    ("-p,--parcellation", parcellationFileName, "Path to input cortical parcellation image")
    ->type_name("")->group("REQUIRED (normal mode)");
  app.add_option
    ("-m,--skullstrip_mask", skullStripMaskFileName, "Path to input skull strip mask")
    ->type_name("")->group("REQUIRED (normal mode)");
  app.add_option
    ("-d,--output_dir", outputDirName, "Path to output directory")
    ->type_name("")->group("REQUIRED");

  // Erosion morphology parameters
  app.add_option
    ("-n,--n_atrophy_iters", nErosionIters,
     "Number of binary morphology iterations to induce synthetic atrophy (default = 4)")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("-f,--upsampling_factor", upsamplingFactor,
     "Factor by which to upsample image resolution for atrophy induction (default = 4.0)")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("-w,--wm_labels", wmLabels,
     "Value of wm label(s) ipsilateral to target label (default = 2 (left) and/or 41 (right)")
    ->type_name("")->group("OPTIONAL");

  // Target image/surface data
  app.add_option
    ("-i,--input_images", inputTargetImageFileNames,
     "Input filenames of images in which to induce synthetic atrophy")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--input_GMs", inputTargetGMFileNames,
     "Input filenames of gray matter (GM/pial) surfaces in which to induce synthetic atrophy")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--input_WMs", inputTargetWMFileNames,
     "Input filenames of white matter (WM) surfaces in which to induce synthetic atrophy")
    ->type_name("")->group("OPTIONAL");
  app.add_flag
    ("--RAS", convertFromRAS,
     "Convert input target surface vertices from RAS to LPS coordinates")
    ->type_name("")->group("OPTIONAL");
    
  app.add_option
    ("-o,--output_images", outputTargetImageFileNames,
     "Output filenames of images with induced synthetic atrophy (overrides output_dir)")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--output_GMs", outputTargetGMFileNames,
     "Output filenames of atrophied gray matter (GM/pial) surfaces (overrides output_dir)")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--output_WMs", outputTargetWMFileNames,
     "Output filenames of atrophied  white matter (WM) surfaces (overrides output_dir)")
    ->type_name("")->group("OPTIONAL");

  // Required for ground truth change (MSDD) calculation
  app.add_option
    ("--hemis_template", inputHemisTemplateFileName, 
     "Filename for template image used to create MSDD calculated surfaces")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--lh_GM_template_labels", lhGMTemplateLabels,
     "Value of left GM label(s) in the input hemis_template")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--lh_WM_template_labels", lhWMTemplateLabels,
     "Value of left WM label(s) in the input hemis_template")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--rh_GM_template_labels", rhGMTemplateLabels,
     "Value of right GM label(s) in the input hemis_template")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--rh_WM_template_labels", rhWMTemplateLabels,
     "Value of right WM label(s) in the input hemis_template")
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
    std::cerr << "Error: must provide at least one target label for atrophy induction \n";
    return 1;
  }

  if(outputDirName.empty()) {
    std::cerr << "Error: must provide path to output directory\n";
    return 1;
  }


  // Create output filenames (if only a directory is provided)
  if(!inputTargetImageFileNames.empty()) {
    if(outputTargetImageFileNames.empty()) {
      for(const auto& inputFileName : inputTargetImageFileNames) {
	std::string outputBaseName = GetBaseName(inputFileName);
	std::string outputFileName = outputDirName + outputBaseName + ".atrophy.nii.gz";
	outputTargetImageFileNames.emplace_back(outputFileName);
      }
    }
    else if(inputTargetImageFileNames.size() != outputTargetImageFileNames.size()) {
      std::cerr << "Error: must provide equal number of input/output target image filenames "
		<< "(or use output directory and default filename)\n";
      return 1;
    }
  }

  if(!inputTargetGMFileNames.empty()) {
    if(outputTargetGMFileNames.empty()) {
      for(const auto& inputFileName : inputTargetGMFileNames) {
	std::string outputBaseName = GetBaseName(inputFileName);
	std::string outputFileName = outputDirName + outputBaseName + ".atrophy.vtp";
	outputTargetGMFileNames.emplace_back(outputFileName);
      }
      }
    else if(inputTargetGMFileNames.size() != outputTargetGMFileNames.size()) {
      std::cerr << "Error: must provide equal number of input/output target GM filenames "
                << "(or use output directory and default filename)\n";
      return 1;
    }
  }

  if(!inputTargetWMFileNames.empty()) {
    if(outputTargetWMFileNames.empty()) {
      for(const auto& inputFileName : inputTargetWMFileNames) {
        std::string outputBaseName = GetBaseName(inputFileName);
        std::string outputFileName = outputDirName + outputBaseName + ".atrophy.vtp";
        outputTargetWMFileNames.emplace_back(outputFileName);
      }
      }
    else if(inputTargetWMFileNames.size() != outputTargetWMFileNames.size()) {
      std::cerr << "Error: must provide equal number of input/output target WM filenames "
                << "(or use output directory and default filename)\n";
      return 1;
    }
  }


  //-----------------------------------------------------------------------------------------------//
  
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

  std::vector<RBSAOutputTuple> outputs;
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
  UCharImageType::Pointer warpMask;
  VectorImageType::Pointer imageWarp, surfaceWarp;
  
  auto compositeOutputs = CombineLabelOutputs(outputs);
  warpMask = std::get<0>(compositeOutputs);
  imageWarp = std::get<1>(compositeOutputs);
  surfaceWarp = std::get<2>(compositeOutputs);

  WriteImage<VectorImageType>(imageWarp, outputDirName + "warp.nii.gz");
  WriteImage<VectorImageType>(surfaceWarp, outputDirName + "warp_inverse.nii.gz");
  

  // Apply to target image data
  std::cout << "Applying to target image data.." << std::endl;
  
  for(size_t n = 0; n < inputTargetImageFileNames.size(); n++) {
    auto input = ReadImage<FloatImageType>(inputTargetImageFileNames.at(n));
    auto output = WarpImage(input, imageWarp);
    WriteImage<FloatImageType>(output, outputTargetImageFileNames.at(n));
  }

  // Apply to target surface data
  std::cout << "Applying to target surface.." << std::endl;
  
  if(inputTargetGMFileNames.size() > 0 || inputTargetWMFileNames.size() > 0) {
    // Make parcellation image specific to cortical labels
    std::vector<int> parcLabels;
    for(const auto& label : targetLabels) {
      parcLabels.emplace_back(label);
    }
    IntImageType::Pointer targetParc =
      MakeTargetLabelParcellation<IntImageType>(parcellation, parcLabels);

    // GM surfaces
    for(size_t n = 0; n < inputTargetGMFileNames.size(); n++) {
      auto input = ReadPolyData(inputTargetGMFileNames.at(n));
      auto output = WarpSurface(input, surfaceWarp, warpMask, convertFromRAS);
      ParcellateSurface<IntImageType>(output, targetParc, "TargetLabels", parcLabels,
				      convertFromRAS);
      WritePolyData(output, outputTargetGMFileNames.at(n));
    }

    // WM surfaces
    for(size_t n = 0; n < inputTargetWMFileNames.size(); n++) {
      auto input = ReadPolyData(inputTargetWMFileNames.at(n));
      auto output = WarpSurface(input, surfaceWarp, warpMask, convertFromRAS);
      ParcellateSurface<IntImageType>(output, targetParc, "TargetLabels", parcLabels,
                                      convertFromRAS);
      WritePolyData(output, outputTargetWMFileNames.at(n));
    }
  }
  

  // Measure true induced change (MSDD) ?
  if(inputHemisTemplateFileName.empty()) return 0;
  std::cout << "Calculating MSDD" << std::endl;

  MSDD msdd;
  msdd.SetTargetLabels(targetLabels);
  msdd.SetCorticalParcellation(parcellation);
  msdd.SetWarpMask(warpMask);
  msdd.SetWarp(surfaceWarp);
  
  IntImageType::Pointer hemis_template = ReadImage<IntImageType>(inputHemisTemplateFileName);
  msdd.MakeHemiTemplates(hemis_template, lhGMTemplateLabels, lhWMTemplateLabels,
			 rhGMTemplateLabels, rhWMTemplateLabels);
  msdd.Calculate();

  std::string msddDirName = outputDirName + "/MSDD";
  msdd.Write(msddDirName);

  return 0;
  
}
