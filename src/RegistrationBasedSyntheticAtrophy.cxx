#include "BinaryImageMorphology.h"
#include "Blur.h"
#include "utils.h"


int main(int argc, char * argv[])
{
  std::chrono::steady_clock::time_point t, t_total = std::chrono::steady_clock::now();
  
  // Declare everything
  std::string parcellationFileName;
  std::string skullStripMaskFileName;

  int label;
  int wm;

  
  // Parse commandline args
  CLI::App app{"Input args:"};
  argv = app.ensure_utf8(argv);

  app.add_option
    ("-p,--parc", parcellationFileName,
     "Input cortical parcellation image filename (cannot be .mgz filetype)")
    ->type_name("")->group("REQUIRED");
  app.add_option
    ("-s,--skullstrip", skullStripMaskFileName,
     "Input skull strip mask filename (cannot be .mgz filetype)")
    ->type_name("")->group("REQUIRED");

  app.add_option
    ("-l,--ctx_label", label, "Value of target label for atrophy induction")
    ->type_name("")->group("REQUIRED");
  app.add_option
    ("-w,--wm_label", wm, "Value of wm label ipsilateral to target label")
    ->type_name("")->group("REQUIRED");
  
  CLI11_PARSE(app, argc, argv);


  // Check input args
  if(parcellationFileName.empty()) {
    std::cerr << "Error:  input cortical parcellation image filename (-p, --parc) is required.\n";
    return 1;
  }



  // Read data
  IntImageType::Pointer parcellation = ReadImage<IntImageType>(parcellationFileName);
  UCharImageType::Pointer skullStripMask = ReadImage<UCharImageType>(skullStripMaskFileName);

  // Create all necessary masks
  UCharImageType::Pointer labelMask = BinaryThresholdImage<IntImageType, UCharImageType>
    (parcellation, label, label);
  UCharImageType::Pointer wmMask = BinaryThresholdImage<IntImageType, UCharImageType>
    (parcellation, wm, wm);
  UCharImageType::Pointer brainMask = BinaryThresholdImage<IntImageType, UCharImageType>
    (parcellation, 1, std::numeric_limits<int>::max());
  //skullStripMask = DilateErodeCorrection(skullStripMask);


  UCharImageType::Pointer blurMask = GetBlurMask(labelMask, brainMask, skullStripMask, 0.4);
    

  // 
  
  /*
    Steps:
    1. Create label maps:
       - WM mask (wm from aparc+aseg)
       - skullStripMask (mri/brainmask.mgz)
       - Full brain mask (cortex, not cerebellum and brainstem)
    2. Get high resolution cropped volume around label ROI
    3. Make blur mask
    4. Perform erosion
    5. Get brain mask for atrophied timepoint
    6. Do registration between timepoints (on high res crop) 
    7. Mask/blur field
    8. Apply final field to full images at original resolution
    9. Get original surfaces
    9. Apply transform to original surfaces
    10. Measure true change
  */

  // Declare all possible input variables


  // CLI stuff


  
  
}
