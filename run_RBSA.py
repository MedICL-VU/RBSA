#!/usr/bin/env python

import os
import sys
import argparse
import glob
import yaml


description = """
Wrapper for the Registration-Based Synthetic Atrophy (RBSA) software. Given an input, this tool 
performs the following steps to induce synthetic atrophy in a user-specified target label (or 
labels):

1. Isolates the target GM label(s) from the input parcellation to create a mask for the original 
   timepoint
2. Performs a series of binary image morphology operations in each target label to create 
   corresponding masks for the "atrophied" timepoint
3. Registers the original masks to the "atrophied" masks to yield a displacement field that pushes 
   the GM/CSF boundary inwards towards the WM. Individual fields are generated for each label and
   then combined to produce single, atrophy inducing transformation.
4. Applies the transformation to any input images/surfaces.
5. Calculates the ground truth, mean change induced within each target label, defined as the mean 
   surface displacement difference (MSDD).

The code has two input modes:
1. Normal mode
   - Requires user to provide specific paths for each required/optional input
   - Limited to processing 1 subject (e.g., using labels from a single input cortical parcellation)
     at a time
2. FreeSurfer (FS) mode
   - Requires either a list of subjects (--s) and/or the environment variable SUBJECTS_DIR (--sd). 
   - Output paths will be set automatically in the FS filename convention, but the user can override
     this by specifying an output directory (--output_dir) if desired.
   - Inputs should be provided as basenames of recon-all output files within the respective subject 
     dirs (e.g., --parcellation aparc+aseg.mgz, --skullstrip_mask brainmask.mgz, etc.) rather than 
     as absolute paths. The same recon-all stream output image will be used for each subject 
     (assuming it exists).
"""

#---------------------------------------------------------------------------------------------------

def main():
    # Parse commandline args
    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(1)

    if '--config' not in sys.argv:
        _error('Please input a .yaml config file (--config <CONFIG>)')
    
    parser = argparse.ArgumentParser(description=description)
    parser = _define_args(parser)
    pargs = parser.parse_args()

    config_path = pargs.config
    no_MSDD = pargs.no_MSDD
    no_output = pargs.no_output

    # Read config file
    if not os.path.isfile(config_path):
        _error(f'config file {config_path} does not exist :(')
    elif config_path.split('.')[-1] != 'yaml':
        _error(f'config file must have .yaml extension (user provided {config_path})')
    
    config = yaml.safe_load(open(config_path))
    mode, FS_config = (
        ('FS', config.get('FS_mode')) if config.get('FS_mode') is not None
        else ('normal', None)
    )
    fnames_config = config.get('Filenames') if config.get('Filenames') is not None else _error(
        '.yaml config should have a top-level item containing all input paths'
    )
    labels_config = config.get('Labels') if config.get('Labels') is not None else _error(
        '.yaml config should have a top-level item containing all input paths'
    )
    
    FS_defaults = _get_FS_defaults() if mode == 'FS' else None
    
    # Arg checking (mode specific)
    if mode == 'FS':
        if not os.environ.get('FREESURFER_HOME'):
            _error('FREESURFER_HOME is not set. Please source FreeSurfer.')

        FS_subjects_dir = (
            FS_config.get('subjects_dir') if FS_config.get('subjects_dir') is not None
            else os.environ.get('SUBJECTS_DIR')
        )
        if FS_subjects_dir is None:
            _error('must specify subjects_dir in either the .yaml config or the os environment.')
        
        FS_subject_ids = (
            pargs.subject_id if pargs.subject_id is not None else FS_config.get('subject_ids')
        )
        if fnames_config.get('parcellation') is None:
            fnames_config['parcellation'] = FS_defaults.get('parcellation')
            print(f'no path to cortical parcellation specified in .yaml config... using default '
                  '{FS_defaults.get("parcellation")]}.')

        if fnames_config.get('skullstrip') is None:
            fnames_config['skullstrip'] = FS_defaults.get('skullstrip')
            print(f'no path to skullstrip image specified in .yaml config... using default '
                  '{FS_defaults.get("skullstrip")}.')

        if fnames_config.get('hemis_template') is None and not no_MSDD:
            fnames_config['hemis_template'] = FS_defaults.get('hemis_template')
            print(f'no path to template image (hemis_template) used to create surfaces '
                  'for measuring ground truth change (e.g., MSDD)... using default '
                  'FS_defaults.get("hemis_template")]}.')

        if not no_MSDD and fnames_config['hemis_template'] != 'ribbon.mgz' and (
                labels_config.get('lh_GM_template_labels') is None
                or labels_config.get('lh_WM_template_labels') is None
                or labels_config.get('rh_GM_template_labels') is None
                or labels_config.get('rh_WM_template_labels') is None
        ):
            _error('must provide label values for left GM, left WM, right GM, and right WM if not '
                   'using the default ribbon.mgz FreeSurfer recon-all output for the '
                   'hemis_template variable')
        else:
            labels_config['lh_GM_template_labels'] = 3
            labels_config['lh_WM_template_labels'] = 2
            labels_config['rh_WM_template_labels'] = 42
            labels_config['rh_WM_template_labels'] = 41

    elif mode == 'normal':
        FS_subjects_dir = None
        FS_subject_ids = None
        
        if fnames_config.get('parcellation') is None:
            _error('must specify path to cortical parcellation (parcellation) when running in '
                   'normal mode.')
        
        if fnames_config.get('skullstrip') is None:
            _error('must specify path to skullstrip image (skullstrip) when running in normal '
                   'mode.')

        if fnames_config.get('output_dir') is None and not no_output:
            _error('must specify path to output directory (output_dir) when running in normal mode '
                   'unless --no_output is specified.')

        if not no_MSDD:
            if fnames_config.get('hemis_template') is None:
                _error('must specify path to template image (hemis_template) used to create '
                       'surfaces for measuring ground truth change (e.g., MSDD) unless --no_MSDD '
                       'is specified.')

            if (labels_config.get('lh_GM_template_labels') is None
                or labels_config.get('lh_WM_template_labels') is None
                or labels_config.get('rh_GM_template_labels') is None
                or labels_config.get('rh_WM_template_labels') is None
            ):
                _error('must provide label values for left GM, left WM, right GM, and right WM '
                       ' within the hemis_template image.')

        if labels_config.get('wm_labels') is None:
            _error('must provide values for the WM labels within the input cortical parcellation.')

    # Checking target data paths
    if fnames_config.get('input_images') is None and fnames_config.get('output_images') is not None:
        _error('output_images is provided, but input_images is empty')

    if fnames_config.get('input_GMs') is None and fnames_config.get('output_GMs') is not None:
        _error('output_GMs is provided, but input_GMs is empty')

    if fnames_config.get('input_WMs') is None and fnames_config.get('output_WMs') is not None:
        _error('output_WMs is provided, but input_WMs is empty')

    # Checking target label values
    if labels_config.get('atrophy_target_labels') is None:
        if labels_config.get('atrophy_target_file') is None:
            _error('must provide within the .yaml config a list of target labels for synthetic '
                   'atrophy induction either (1) directly, via the atrophy_target_labels item, or '
                   '(2) inside of a separate text file, the path to which is specified via the '
                   'atrophy_target_labels_file within the .yaml config.')
        else:
            if not os.path.isfile(labels_config.get('atrophy_target_file')):
                _error(f'{labels_config.get("atrophy_target_file")} is not a valid file :(')
            with open(labels_config.get('atrophy_target_file')) as f:
                labels_config['atrophy_target_labels'] = f.read().splitlines()
    else:
        if labels_config.get('atrophy_target_labels_file') is not None:
            labels_config['atrophy_target_labels_file'] = None
            print('Warning: both "atrophy_target_labels_file" and "atrophy_target_labels" provided '
                  'in .yaml config... taking target labels from "atrophy_target_labels" and '
                  'ignoring "atrophy_target_labels_file"')

    for labels in [
            'atrophy_target_labels', 'wm_labels', 'lh_GM_template_labels', 'lh_WM_template_labels',
            'rh_GM_template_labels', 'rh_WM_template_labels'
    ]:
        if not isinstance(labels_config[f'{labels}'], list):
            labels_config[f'{labels}'] = [labels_config[f'{labels}']]
            
    # Checking parameter values
    n_atrophy_iters = config.get('n_atrophy_iters')
    upsampling_factor = config.get('upsampling_factor')
        
    if n_atrophy_iters is None and upsampling_factor is None:
        n_atrophy_iters = 4
        upsampling_factor = 4.0
        print('using default values n_atrophy_iters=4 and upsampling_factor=4.0')
    
    elif config.get('n_atrophy_iters') is None:
        n_atrophy_iters = 1 if upsampling_factor < 1 else int(upsampling_factor)
        print(f'n_atrophy_iters not specified... calculating from int(upsampling_factor)')

    elif config.get('upsampling_factor') is None:
        upsampling_factor = float(n_atrophy_iters)
        print('upsampling_factor not specified... setting equal to n_atrophy_iters to induce '
              'equivalent of 1 voxel worth of atrophy')
        
    if not isinstance(config.get('n_atrophy_iters'), int):
        n_atrophy_iters = int(n_atrophy_iters)
        print(f'n_atrophy_iters must be an integer... converting input value from '
              '{config.get("n_atrophy_iters")} to {n_atrophy_iters}')
        
    RAS = True if mode == 'FS' else config.get('RAS') if config.get('RAS') is not None else False 
    MSDD = True if config.get('hemis_template') is not None and not no_MSDD else False

    # Get filenames
    paths_dict = _get_filenames(
        mode=mode,
        FS_subject_ids=FS_subject_ids,
        FS_subjects_dir=FS_subjects_dir,
        **fnames_config
    )

    for _dict in paths_dict:
        # Convert files to compatible types w/ ITK/VTK (e.g., .mgz-->nii.gz, .gii-->.vtp)
        if mode == 'FS':
            input_dir = os.path.join(_dict['output_dir'], 'inputs')
            target_dir = os.path.join(_dict['output_dir'], 'targets')
            os.makedirs(input_dir, exist_ok=True)
            os.makedirs(target_dir, exist_ok=True)

            for key in ['output_images', 'output_GMs', 'output_WMs']:
                _dict[key] = [
                    os.path.join(target_dir, os.path.basename(fname)) for fname in _dict[key]
                ] if _dict[key] is not None else None

            for key in ['parcellation', 'skullstrip', 'hemis_template']:
                _dict[key] = _ensure_filetype_compatability(
                    _symlink(_dict[key], input_dir, key), 'image'
                )

            for key in ['input_images', 'output_images']:
                _dict[key] = [
                    _ensure_filetype_compatability(
                        _symlink(fname, target_dir) if os.path.isfile(fname) else fname, 'image'
                    ) for fname in _dict[key]
                ] if _dict[key] is not None else None

            for key in ['input_GMs', 'input_WMs', 'output_GMs', 'output_WMs']:
                _dict[key] = [
                    _ensure_filetype_compatability(
                        _symlink(fname, target_dir) if os.path.isfile(fname)
                        else fname, 'surface', RAS
                    ) for fname in _dict[key]
                ] if _dict[key] is not None else None

        # Run RBSA
        cmd = './build/bin/RBSA'
        cmd += f' --parcellation {_dict["parcellation"]}'
        cmd += f' --skullstrip {_dict["skullstrip"]}'
        cmd += f' --output_dir {_dict["output_dir"]}'
        
        cmd += ' --target_labels'
        for label in labels_config.get('atrophy_target_labels'):
            cmd += f' {label}'
        cmd += ' --wm_labels'
        for label in labels_config.get('wm_labels'):
            cmd += f' {label}'
        
        cmd += f' --n_atrophy_iters {n_atrophy_iters}'
        cmd += f' --upsampling_factor {upsampling_factor}'

        if MSDD:
            MSDD_dir = os.path.join(_dict['output_dir'], 'MSDD')
            os.makedirs(MSDD_dir, exist_ok=True)
            cmd += f' --MSDD_dir {MSDD_dir}'

            cmd += f' --hemis_template {_dict["hemis_template"]}'
            cmd += ' --lh_GM_template_labels'
            for label in labels_config.get('lh_GM_template_labels'):
                cmd += f' {label}'
                cmd += ' --lh_WM_template_labels'
            for label in labels_config.get('lh_WM_template_labels'):
                cmd += f' {label}'
                cmd += ' --rh_GM_template_labels'
            for label in labels_config.get('rh_GM_template_labels'):
                cmd += f' {label}'
                cmd += ' --rh_WM_template_labels'
            for label in labels_config.get('rh_WM_template_labels'):
                cmd += f' {label}'
        
        if _dict['input_images'] is not None:
            cmd += ' --input_images'
            for fname in _dict['input_images']:
                cmd += f' {fname}'
            cmd += ' --output_images'
            for	fname in _dict['output_images']:	
                cmd += f' {fname}'

        if _dict['input_GMs'] is not None:
            cmd += ' --input_GMs'
            for	fname in _dict['input_GMs']:	
                cmd += f' {fname}'
            cmd += ' --output_GMs'
            for fname in _dict['output_GMs']:
                cmd += f' {fname}'

        if _dict['input_WMs'] is not None:
            cmd += ' --input_WMs'
            for fname in _dict['input_WMs']:
                cmd += f' {fname}'
            cmd += ' --output_WMs'
            for fname in _dict['output_WMs']:
                cmd += f' {fname}'
        
        cmd += ' --RAS' if RAS else ''
        
        print(cmd)
        if os.system(cmd) > 0:
            _error(f"[run_RBSA.py] RBSA :(")



def _define_args(parser):
    parser.add_argument('-config', '--config', type=str,
                        help='Path to .yaml file containing all input parameters/files.')
    parser.add_argument('--no_MSDD', action='store_true',
                        help='Flag to turn off calculation of  mean surface displacement '
                        'difference (MSDD),')
    parser.add_argument('--no_output', action='store_true',
                        help='Flag to turn off writing warps and auxilary data (e.g., surfaces '
                        'used to calculate MSDD) to file. Does not turn off writing target data '
                        'with simulated atrophy.')
    parser.add_argument('-s', '--subject_id', nargs='+', type=str,
                        help='Subject id to process (within FS mode, should be a subdir of the '
                        'SUBJECTS_DIR environment variable or the FS_subjects_dir item within the '
                        'input .yaml config. Providing this as a commandline argument overrides '
                        'the "FS_subject_ids" item in the input config.')
    return parser


def _ensure_filetype_compatability(inpath, data_type=None, RAS=False):
    cmd, ext = (
        ('mri_convert', 'nii.gz') if data_type == 'image'
        else ('mris_convert' + (' --userealras' if RAS else ''), 'vtk') if data_type == 'surface'
        else (None, None)
    )
    if cmd is None:
        _error("Must provide data type (image or surface) to use _ensure_filetype_compatability")
        
    outpath = '.'.join([_split_ext(inpath)[0], ext])
    if os.path.isfile(inpath):
        cmd += f' {inpath} {outpath} >> /dev/null'
        if os.system(cmd) > 0:
            _error(f"{cmd} failed :(")

    return outpath


def _get_filenames(mode='normal',
                   FS_subject_ids=None,
                   FS_subjects_dir=None,
                   parcellation=None,
                   skullstrip=None,
                   output_dir=None,
                   input_images=None,
                   output_images=None,
                   input_GMs=None,
                   output_GMs=None,
                   input_WMs=None,
                   output_WMs=None,
                   hemis_template=None):

    if mode == 'FS':
        # Set SUBJECTS_DIR
        sdir = os.getenv('SDIR') if FS_subjects_dir is None else os.path.abspath(FS_subjects_dir)
        if sdir is None:
            _error('Must set subjects directory with --sd or SDIR env variable.')

        # Get list of subjects
        subjects = FS_subject_ids
        if subjects is None or len(subjects) == 0:
            subjects = glob.glob(f'{sdir}/*/mri')
            subjects = [os.path.basename(x.replace(f'/mri', '')) for x in subjects]
            subjects = [x for x in subjects if x != 'fsaverage']

        # Required inputs
        def _parse_filenames(dirname, subjects, subdir, fbase):
            if not _is_basename(fbase):
                _error(f'{fbase} must be provided as a basename (e.g., {os.path.basename(fbase)}')

            fnames = []
            for s in subjects:
                fname = os.path.join(dirname, s, subdir, fbase)
                if not os.path.isfile(fname):
                    _error(f'{fname} does not exist :(')
                fnames += [fname]
            return fnames

        parcellations = _parse_filenames(sdir, subjects, 'mri', parcellation)
        skullstrips = _parse_filenames(sdir, subjects, 'mri', skullstrip)

        hemis_template_paths = None if hemis_template is None else _parse_filenames(
            sdir, subjects, 'mri', hemis_template
        )

        output_dir = [
            os.path.join(sdir, s, 'RBSA') if output_dir is None
            else os.path.join(output_dir, s) for s in subjects
        ]
        for dirname in output_dir:  os.makedirs(dirname, exist_ok=True)        
        
        # Target data
        in_image_paths = None if input_images is None else list(
            map(list, zip(*[_parse_filenames(sdir, subjects, 'mri', x) for x in input_images]))
        )
        out_image_paths = [ # TO-DO:  add the ability to provide output paths too
            [os.path.join(dirname, _add_tag(x, 'RBSA')) for x in input_images]
            for dirname in output_dir
        ] if input_images is not None else None
        
        in_GM_paths = None if input_GMs is None else list(
            map(list, zip(*[_parse_filenames(sdir, subjects, 'surf', x) for x in input_GMs]))
        )
        out_GM_paths = [
            [os.path.join(dirname, _add_tag(x, 'RBSA')) for x in input_GMs]
            for dirname in output_dir
        ] if input_GMs is not None else None
        
        in_WM_paths = None if input_WMs is None else list(
            map(list, zip(*[_parse_filenames(sdir, subjects, 'surf', x) for x in input_WMs]))
        )
        out_WM_paths = [
            [os.path.join(dirname, _add_tag(x, 'RBSA')) for x in input_WMs]
            for dirname in output_dir
        ] if input_WMs is not None else None
        
        # Compile into list dictionary w/ all filenames (1 dict per subject)
        out_dicts = []
        for n, _ in enumerate(subjects):
            sdict = {}
            sdict['parcellation'] = parcellations[n]
            sdict['skullstrip'] = skullstrips[n]
            sdict['output_dir'] = output_dir[n]
            sdict['input_images'] = in_image_paths[n] if in_image_paths is not None else None
            sdict['input_GMs'] = in_GM_paths[n] if in_GM_paths is not None else None
            sdict['input_WMs'] = in_WM_paths[n] if in_WM_paths is not None else None
            sdict['output_images'] = out_image_paths[n] if out_image_paths is not None else None
            sdict['output_GMs'] = out_GM_paths[n] if out_GM_paths is not None else None
            sdict['output_WMs'] = out_WM_paths[n] if out_WM_paths is not None else None
            sdict['hemis_template'] = (
                hemis_template_paths[n] if hemis_template_paths is not None else None
            )
            out_dicts += [sdict]

        return out_dicts

    else:
        # Running in normal mode
        out_dict = {}

        # Required args
        out_dict['parcellation'] = parcellation if os.path.isfile(parcellation) else _error(
            f'Input parcellation f{parcellation} does not exist'
        )
        out_dict['skullstrip'] = skullstrip if os.path.isfile(skullstrip) else _error(
            f'Input skullstripped image f{skullstrip} does not exist'
        )
        out_dict['hemis_template'] = (
            None if hemis_template is None
            else hemis_template if os.path.isfile(hemis_template)
            else _error(f'Input hemis_template f{hemis_template} does not exist')
        )        

        out_dict['output_dir'] = output_dir
        os.makedirs(output_dir, exist_ok=True)

        # Target data
        out_dict['input_images'] = None if input_images is None else [
            x if os.path.isfile(x) else _error(f'Input target file {x} does not exist')
            for x in input_images
        ]
        out_dict['output_images'] = (
            None if input_images is None
            else output_images if output_images is not None
            else [os.path.join(output_dir, f'{_split_ext(os.path.basename(x))[0]}.RBSA.nii.gz')
                  for x in input_images]
        )

        out_dict['input_GMs'] = None if input_GMs is None else [
            x if os.path.isfile(x) else _error(f'Input target file {x} does not exist')
            for x in input_GMs
        ]
        out_dict['output_GMs'] = (
            None if input_GMs is None
            else output_GMs if output_GMs is not None
            else [os.path.join(output_dir, f'{_split_ext(os.path.basename(x))[0]}.RBSA.vtp')
                  for x in input_GMs
            ]
        )

        out_dict['input_WMs'] = None if input_WMs is None else [
            x if os.path.isfile(x) else _error(f'Input target fle {x} does not exist')
            for	x in input_WMs
        ]
        out_dict['output_WMs'] = (
            None if input_WMs is None
            else output_WMs if output_WMs is not None
            else [os.path.join(output_dir, f'{_split_ext(os.path.basename(x))[0]}.RBSA.vtp')
                  for x in input_WMs
            ]
        )

        return [out_dict]


# --------------------------------------------------------------------------------------------------

def _add_tag(inpath, tag):
    base, ext = _split_ext(inpath)
    return '.'.join([base, tag]) if ext == '' else '.'.join([base, tag, ext])


def _error(message):
    print('Error:', message)
    sys.exit(1)
    return


def _get_FS_defaults():
    _dict = {}
    _dict['parcellation'] = 'aparc+aseg.mgz'
    _dict['skullstrip'] = 'brainmask.mgz'
    _dict['hemis_template'] = 'ribbon.mgz'
    _dict['wm_labels'] = [2, 41]
    _dict['lh_GM_template_labels'] = 3
    _dict['lh_WM_template_labels'] = 2
    _dict['rh_GM_template_labels'] = 42
    _dict['rh_WM_template_labels'] = 41
    return _dict

def _is_basename(inpath):
    return (inpath == os.path.basename(inpath))


def _is_mgh_type_image(inpath):
    for ext in ['.mgz', '.mgz']:
        if inpath.endswith(ext):
            return True
    return False


def _is_vtk_type_surface(inpath):
    for ext in ['.vtk', '.vtp']:
        if inpath.endswith(ext):
            return True
    return False


def _split_ext(inpath):
    exts = ['.asc', '.ico', '.geo', '.gii', '.mgz', '.mgz', '.nifti',
            '.nii', '.nii.gz', '.nrrd', '.stl', '.tri']
    
    for ext in exts:
        if inpath[-len(ext):] == ext:
            return inpath.replace(ext, ''), ext[1:]
    return inpath, ''


def _symlink(src_fname, trg_dir, trg_basename=None):
    fbase, ext = _split_ext(src_fname)
    basename = os.path.basename(fbase) if trg_basename is None else trg_basename
    trg_fname = os.path.join(trg_dir, basename if ext == '' else '.'.join([basename, ext]))

    if os.path.isfile(trg_fname):
        os.unlink(trg_fname)
    os.symlink(src_fname, trg_fname)

    return trg_fname

        
if __name__ == "__main__":
    main()
