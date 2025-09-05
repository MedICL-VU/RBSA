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
  std::string outputFilesBaseName;
  std::string msddDirName;
  std::string warpFileName;

  std::vector<TPixel<IntImageType>> targetLabels;
  std::vector<TPixel<IntImageType>> wmLabels = {2, 41};

  unsigned int nErosionIters = 4;
  float upsamplingFactor = 4.0;
  
  std::vector<std::string> inputTargetImageFileNames;
  std::vector<std::string> outputTargetImageFileNames;
  std::vector<std::string> inputTargetGMFileNames;
  std::vector<std::string> outputTargetGMFileNames;
  std::vector<std::string> inputTargetWMFileNames;
  std::vector<std::string> outputTargetWMFileNames;
  bool isRAS = false;

  std::string inputHemisTemplateFileName;
  std::vector<TPixel<IntImageType>> lhGMTemplateLabels;
  std::vector<TPixel<IntImageType>> lhWMTemplateLabels;
  std::vector<TPixel<IntImageType>> rhGMTemplateLabels;
  std::vector<TPixel<IntImageType>> rhWMTemplateLabels;
  
  // Parse commandline args
  CLI::App app{"Input args:"};
  argv = app.ensure_utf8(argv);
  
  // Required arguments:
  app.add_option
    ("-p,--parcellation", parcellationFileName,
     "Path to input cortical parcellation image")
    ->type_name("")->group("REQUIRED");
  app.add_option
    ("-s,--skullstrip", skullStripMaskFileName,
     "Path to input skull strip mask")
    ->type_name("")->group("REQUIRED");
  app.add_option
    ("-l,--target_labels", targetLabels,
     "Value of target labels for atrophy induction within input parcellation")
    ->type_name("")->group("REQUIRED");
  app.add_option
    ("-w,--wm_labels", wmLabels,
     "Value of wm label(s) ipsilateral to target label within input parcellation")
    ->type_name("")->group("REQUIRED");
  app.add_option
    ("-d,--output_dir", outputDirName, "Path to general directory")
    ->type_name("")->group("REQUIRED");
  
  // Optional args (general)
  app.add_option
    ("--n_atrophy_iters", nErosionIters,
     "Number of binary morphology iterations to induce synthetic atrophy (default = 4)")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--upsampling_factor", upsamplingFactor,
     "Factor by which to upsample image resolution for atrophy induction (default = 4.0)")
    ->type_name("")->group("OPTIONAL");
  app.add_option
    ("--warp", warpFileName,
     "Input displacement field to apply to target data (avoids recalculating warp)")
    ->type_name("")->group("OPTIONAL");
  
  // Target data
  app.add_option
    ("-b,--output_basename", outputFilesBaseName, "Basename for output warps/files")
    ->type_name("")->group("TARGET DATA PATHS (OPTIONAL)");
  app.add_option
    ("--input_images", inputTargetImageFileNames,
     "Path(s) to input target images for synthetic atrophy")
    ->type_name("")->group("TARGET DATA PATHS (OPTIONAL)");
  app.add_option
    ("--output_images", outputTargetImageFileNames,
     "Output path(s) for images with induced synthetic atrophy")
    ->type_name("")->group("TARGET DATA PATHS (OPTIONAL)");
  app.add_option
    ("--input_GMs", inputTargetGMFileNames,
     "Path(s) to input target gray matter (GM/pial) surfaces for synthetic atrophy")
    ->type_name("")->group("TARGET DATA PATHS (OPTIONAL)");
  app.add_option
    ("--output_GMs", outputTargetGMFileNames,
     "Output path(s) for gray matter (GM/pial) surfaces with induced synthetic atrophy")
    ->type_name("")->group("TARGET DATA PATHS (OPTIONAL)");
  app.add_option
    ("--input_WMs", inputTargetWMFileNames,
     "Path(s) to target input white matter (WM) surfaces for synthetic atrophy")
    ->type_name("")->group("TARGET DATA PATHS (OPTIONAL)");
  app.add_option
    ("--output_WMs", outputTargetWMFileNames,
     "Output path(s) for white matter (WM) surfaces with induced synthetic atrophy")
    ->type_name("")->group("TARGET DATA PATHS (OPTIONAL)");
  app.add_flag
    ("--RAS", isRAS,
     "Convert input target surface vertices from RAS to LPS coordinates")
    ->type_name("")->group("TARGET DATA PATHS (OPTIONAL)");

  // Required for ground truth change (MSDD) calculation
  app.add_option
    ("--MSDD_dir", msddDirName, "Path to output directory for MSDD outputs")
    ->type_name("")->group("MSDD CALCULATION (OPTIONAL)");
  app.add_option
    ("--hemis_template", inputHemisTemplateFileName, 
     "Filename for template image used to create MSDD calculated surfaces")
    ->type_name("")->group("MSDD CALCULATION (OPTIONAL)");
  app.add_option
    ("--lh_GM_template_labels", lhGMTemplateLabels,
     "Value of left GM label(s) in the input hemis_template")
    ->type_name("")->group("MSDD CALCULATION (OPTIONAL)");
  app.add_option
    ("--lh_WM_template_labels", lhWMTemplateLabels,
     "Value of left WM label(s) in the input hemis_template")
    ->type_name("")->group("MSDD CALCULATION (OPTIONAL)");
  app.add_option
    ("--rh_GM_template_labels", rhGMTemplateLabels,
     "Value of right GM label(s) in the input hemis_template")
    ->type_name("")->group("MSDD CALCULATION (OPTIONAL)");
  app.add_option
    ("--rh_WM_template_labels", rhWMTemplateLabels,
     "Value of right WM label(s) in the input hemis_template")
    ->type_name("")->group("MSDD CALCULATION (OPTIONAL)");
  

  // Parse args
  if(argc < 2) {
    std::cout << app.help() << std::endl;
    return 0;
  }
  
  CLI11_PARSE(app, argc, argv);
  
  // Check input args
  if(parcellationFileName.empty()) {
    std::cerr << "Error: input cortical parcellation image filename (-p, --parc) is required"
	      << std::endl;
    return 1;
  }

  if(skullStripMaskFileName.empty()) {
    std::cerr << "Error: input skull strip mask filename (-s, --skullstrip) is required"
	      << std::endl;
    return 1;
  }

  if(targetLabels.empty()) {
    std::cerr << "Error: must provide at least one target label for atrophy induction"
	      << std::endl;
    return 1;
  }

  if(outputDirName.empty()) {
    std::cerr << "Error: must provide path to output directory"
	      << std::endl;
    return 1;
  }
  
  // Create output filenames (if only a directory is provided)
  if(!inputTargetImageFileNames.empty()) {
    if(outputTargetImageFileNames.empty()) {
      for(const auto& inputFileName : inputTargetImageFileNames) {
	std::string outputBaseName = GetBaseName(inputFileName);
	if(!outputFilesBaseName.empty()) outputBaseName += "." + outputFilesBaseName;

	std::string outputFileName = outputDirName + outputBaseName + ".atrophy.nii.gz";
	outputTargetImageFileNames.emplace_back(outputFileName);
      }
    }
    else if(inputTargetImageFileNames.size() != outputTargetImageFileNames.size()) {
      std::cerr << "Error: must provide equal number of input/output target image filenames "
		<< "(or use output directory and default filename)"
                << std::endl;
      return 1;
    }
  }

  if(!inputTargetGMFileNames.empty()) {
    if(outputTargetGMFileNames.empty()) {
      for(const auto& inputFileName : inputTargetGMFileNames) {
	std::string outputBaseName = GetBaseName(inputFileName);
	if(!outputFilesBaseName.empty()) outputBaseName += "." + outputFilesBaseName;
	
	std::string outputFileName = outputDirName + outputBaseName + ".atrophy.vtp";
	outputTargetGMFileNames.emplace_back(outputFileName);
      }
    }
    else if(inputTargetGMFileNames.size() != outputTargetGMFileNames.size()) {
      std::cerr << "Error: must provide equal number of input/output target GM filenames "
                << "(or use output directory and default filename)"
		<< std::endl;
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
                << "(or use output directory and default filename)"
                << std::endl;
      return 1;
    }
  }


  //----------------------------------------------------------------------------------------------//

  // Read input image data
  auto parc = ReadImage<IntImageType>(parcellationFileName);
  auto skullStripMask = ReadImage<UCharImageType>(skullStripMaskFileName);

  // Get atrophy warp and mask
  TPointer<VectorImageType> warp;
  TPointer<UCharImageType> warpMask;

  if(warpFileName.empty()) {
    // No input warp --> generate from scratch
    RBSA atrophier;
    atrophier.SetInputParcellation(parc);
    atrophier.SetSkullStripMask(skullStripMask);
    atrophier.SetNumberOfErosionIterations(nErosionIters);
    atrophier.SetUpsamplingFactor(upsamplingFactor);
    atrophier.SetTargetLabels(targetLabels);
    atrophier.SetWMLabels(wmLabels);

    atrophier.GenerateAtrophyTransforms();
    warpMask = atrophier.GetWarpMask();
    warp = atrophier.GetWarp();
    
    // Write warp
    warpFileName = outputDirName + "/warp_";
    if(!outputFilesBaseName.empty()) warpFileName += outputFilesBaseName;
    warpFileName += ".nii.gz";

    std::cout << "Writing atrophy warp to " << warpFileName << std::endl;
    WriteImage<VectorImageType>(warp, warpFileName);
  }
  else {
    // User provided atrophy warp --> load from file and threshold for mask
    std::cout << "Reading warp from file (" << warpFileName << ")" << std::endl;
    warp = ReadImage<VectorImageType>(warpFileName);
    warpMask = BinaryThresholdVectorImage(warp);
  }
      
  // Apply to target image data
  std::cout << "Applying to target image data.." << std::endl;
  
  for(size_t n = 0; n < inputTargetImageFileNames.size(); n++) {
    auto input = ReadImage<FloatImageType>(inputTargetImageFileNames.at(n));
    auto output = WarpImage(input, warp);
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
    TPointer<IntImageType> targetParc = MakeTargetLabelParcellation<IntImageType>(parc, parcLabels);

    // GM surfaces
    for(size_t n = 0; n < inputTargetGMFileNames.size(); n++) {
      auto input = ReadPolyData(inputTargetGMFileNames.at(n));

      bool checkMask = false;
      auto output = WarpSurface(input, warp, warpMask, isRAS, checkMask);

      ParcellateSurface<IntImageType>(output, targetParc, "TargetLabels", parcLabels, isRAS);
      WritePolyData(output, outputTargetGMFileNames.at(n));
    }

    // WM surfaces
    for(size_t n = 0; n < inputTargetWMFileNames.size(); n++) {
      bool checkMask = true;
      auto input = ReadPolyData(inputTargetWMFileNames.at(n));
      auto output = WarpSurface(input, warp, warpMask, isRAS, checkMask);
      ParcellateSurface<IntImageType>(output, targetParc, "TargetLabels", parcLabels, isRAS);
      WritePolyData(output, outputTargetWMFileNames.at(n));
    }
  }
  

  // Measure true induced change (MSDD) ?
  if(inputHemisTemplateFileName.empty()) return 0;
  std::cout << "Calculating MSDD" << std::endl;
  
  MSDD msdd;
  msdd.SetTargetLabels(targetLabels);
  msdd.SetCorticalParcellation(parc);
  msdd.SetWarpMask(warpMask);
  msdd.SetWarp(warp);
  
  TPointer<IntImageType> hemisTemplate = ReadImage<IntImageType>(inputHemisTemplateFileName);
  msdd.MakeHemiTemplates(hemisTemplate, lhGMTemplateLabels, lhWMTemplateLabels,
			 rhGMTemplateLabels, rhWMTemplateLabels);
  msdd.Calculate();

  if(msddDirName.empty()) {
    msddDirName = outputDirName + "/MSDD";
    if(!outputFilesBaseName.empty()) msddDirName += "/" + outputFilesBaseName;
  }
  msdd.Write(msddDirName);

  return 0;
  
}
