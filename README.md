# Registration-Based Synthetic Atrophy (RBSA): Synthetic Atrophy for Longitudinal Cortical Surfaces

## Overview

This repository contains a Registration-based tool for inducing ground truth, synthetic gray matter (GM) atrophy in brain images and cortical surfaces. Given an input cortical label map, the RBSA pipeline performs the following steps to induce synthetic atrophy in a user-specified target label (or labels):

1. Isolates the target GM label(s) from the input parcellation to create a mask for the original timepoint
2. Performs a series of binary image morphology operations in each target label to create corresponding masks for the "atrophied" timepoint
3. Registers the original masks to the "atrophied" masks to yield a displacement field that pushes the GM/CSF boundary inwards towards the WM. Individual fields are generated for each label and then combined to produce single, atrophy inducing transformation.
4. Applies the transformation to any input images/surfaces.
5. Calculates the ground truth, mean change induced within each target label, defined as the mean surface displacement difference (MSDD):

```math
\textrm{MSDD}_{\textrm{label}} =
\left[ \frac{1}{2}( \textrm{GM}_{\textrm{orig}\rightarrow\textrm{atrophy}} + \textrm{GM}_{\textrm{atrophy}\rightarrow\textrm{orig}} ) - \frac{1}{2}( \textrm{WM}_{\textrm{orig}\rightarrow\textrm{atrophy}} + \textrm{WM}_{\textrm{atrophy}\rightarrow\textrm{orig}} ) \right]_{\textrm{label}}
```


## Getting started

### Dependencies

- ITK: https://itk.org/download/
  - version 4.13.2 recommended, as this is what I had access to when developing the new version... but feel free to let me know if it works with a more recent version
- VTK: https://vtk.org/download/
  - version 9.3.0 or later
- FreeSurfer (optional): https://surfer.nmr.mgh.harvard.edu/fswiki/DownloadAndInstall
  - Not required to run, but there is an option to run the pipeline in a way to easily interface with FreeSurfer's recon-all outputs instead of using user-provided paths for all inputs. I designed the tool using FS label maps; it should theoretically work with any compatible set of labels (as outlined in the paper), but I haven't actually tested it with anything else. If you do use something other than FS to generate these, feel free to let me know!

### Compilation

Once you've installed ITK and VTK (and optionally FS), you can run the ./setup.sh script to compile the reposity:

./setup.sh --itk <ITK_install_path>/lib/cmake/ITK-<version> --vtk <VTK_install_path>/lib64/cmake/VTK-<version>

This will point the compiler to the directories containing the ITKConfig.cmake and VTKConfig.cmake files, respectively. Running this script will automatically build and make the RBSA executable.


### Usage

You can run the RBSA software with its required arguments as follows:

```
./build/bin/RBSA -p <parcellation> -s <skullstrip> -o <output_dir> -t <target_labels> --w <wm_labels>
```
- -p: the input cortical parcellation
- -s: a skullstripped image or mask
- o: the directory to which the output warps (e.g., the atrophy inducing transform and inverse) will be written
- t: a list of target GM labels for atrophy induction that exist within the input parcellation
- w: a list of WM labels ipsilateral to the target labels, also within the input parcellation

To see all possible input args, you can print out the help message by running `./RBSA -h`.

In addition to calling the executable directly, you can also configure everything by using the `run_RBSA.py` script along with a .yaml configuration file. Examples of these are provided in the `configs` directory. This is set up to have 2 different input modes:

1. Normal mode, where all absolute paths to each input arg are specified directly in the config
   - Example: config/example.normal_mode.yaml
2. FS mode, where the .yaml file lists a directory of subjects (the SUBJECTS_DIR FS environment variable), a list of subjects, and the basenames (with no directory information) for each input
   - Example: example.FS_mode.yaml

If you are planning to use this in conjunction with FS recon-all outputs, I highly recommend running this by calling the run_RBSA.py script with a config set up for FS mode. The main advantage here is that this script will automatically convert all input images/surfaces from FS filetypes (.mgz images, and .pial/.white surfaces) to filetypes compatible with ITK/VTK (.nii.gz images and .vtk surfaces). It will also automatically configure all input filenames for multiple input subjects (e.g., if want to run this on all subjects in the SUBJECTS_DIR using same input parcellation, skullstrip image, and target/wm labels for each). In this case, if you specify an output directory, it will create a subdir called <output_dir>/<subject_id> for each subject.


## References

If you use this tool, please cite the original paper:

Synthetic Atrophy for Longitudinal Cortical Surface Analyses, K. E. Larson, I. Oguz. Frontiers in Neuroimaging, 2022. https://doi.org/10.3389/fnimg.2022.861687

Note that the current version on GitHub has changed slightly from the one described in the paper, due to me (the author) becoming much better at programming... but the general procedure is still functionally the same!