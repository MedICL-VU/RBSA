#!/usr/bin/env python

import os
import sys
import argparse
import glob


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
    def _check_args(args):
        for arg in args:
            if arg in sys.argv:
                return True
        return False

    # Set up arg parser
    parser = argparse.ArgumentParser(description=description)
    parser = _define_args(parser)

    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(1)
        
    pargs = parser.parse_args()
    mode = 'normal' if pargs.sd is None and pargs.s is None else 'FS'

    # Sanity checks
    if mode == 'FS':
        if not os.environ.get('FREESURFER_HOME'):
            _error('FREESURFER_HOME is not set. Please source FreeSurfer.')

        if pargs.input_images is not None and pargs.output_images is not None:
            print('Warning: --output_images arg is ignored w/ FS mode... outputs will be written '
                  'to <output_dir>/<basename(input)>.RBSA.nii.gz for each input image')
            
        if pargs.input_GMs is not None and pargs.output_GMs is not None:
            print('Warning: --output_GMs arg is ignored w/ FS mode... outputs will be written '
                  'to <output_dir>/<basename(input)>.RBSA.vtp for each input GM surface)')

        if pargs.input_WMs is not None and pargs.output_WMs is not None:
            print('Warning: --output_WMs arg is ignored w/ FS mode... outputs will be written '
                  'to <output_dir>/<basename(input)>.RBSA.vtp for each input WM surface)')

    else:
        if pargs.output_dir is None:
            _error('output directory (--output_dir) is required')

        if pargs.input_images is None and pargs.output_images is not None:
            _error('--output_images is provided, but --input_images is empty')

        if pargs.input_GMs is None and pargs.output_GMs is not None:
            _error('--output_GMs is provided, but --input_GMs is empty')

        if pargs.input_WMs is None and pargs.output_WMs is not None:
            _error('--output_WMs is provided, but --input_WMs is empty')
            
    RAS = True if mode == 'FS' else pargs.RAS

    MSDD = True if _check_args(['--hemis_template']) else pargs.MSDD
    lh_GM_template_labels = (
        pargs.lh_GM_template_labels if isinstance(pargs.lh_GM_template_labels, list)
        else [pargs.lh_GM_template_labels]
    )
    lh_WM_template_labels = (
        pargs.lh_WM_template_labels if isinstance(pargs.lh_WM_template_labels, list)
        else [pargs.lh_WM_template_labels]
    )
    rh_GM_template_labels = (
        pargs.rh_GM_template_labels if isinstance(pargs.rh_GM_template_labels, list)
        else [pargs.rh_GM_template_labels]
    )
    rh_WM_template_labels = (
        pargs.rh_WM_template_labels if isinstance(pargs.rh_WM_template_labels, list)
        else [pargs.rh_WM_template_labels]
    )

    # Get filenames
    paths_dict = _get_filenames(
        mode=mode,
        FS_subject_ids=pargs.s,
        FS_subjects_dir=pargs.sd,
        parcellation=pargs.parcellation,
        skullstrip=pargs.skullstrip_mask,
        output_dir=pargs.output_dir,
        input_images=pargs.input_images,
        output_images=pargs.output_images,
        input_GMs=pargs.input_GMs,
        output_GMs=pargs.output_GMs,
        input_WMs=pargs.input_WMs,
        output_WMs=pargs.output_WMs,
        hemis_template=pargs.hemis_template if MSDD else None,
    )

    for _dict in paths_dict:
        # Convert files to compatible types w/ ITK/VTK (e.g., .mgz-->nii.gz, .gii-->.vtp)
        if mode == 'FS' and _dict['output_dir'] != pargs.sd or mode == 'normal':
            input_dir = os.path.join(_dict['output_dir'], 'inputs')
            target_dir = os.path.join(_dict['output_dir'], 'targets')
            os.makedirs(input_dir, exist_ok=True)
            os.makedirs(target_dir, exist_ok=True)
            os.makedirs(os.path.join(_dict['output_dir'], 'MSDD'))

        for varname in ['parcellation', 'skullstrip', 'hemis_template']:
            _dict[varname] = _ensure_filetype_compatability(
                _symlink(_dict[varname], input_dir, varname), 'image'
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
                    _symlink(fname, target_dir) if os.path.isfile(fname) else fname, 'surface', RAS
                ) for fname in _dict[key]
            ] if _dict[key] is not None else None
    
        # Run RBSA
        cmd = './RBSA'
        cmd += f' --parcellation {_dict["parcellation"]}'
        cmd += f' --skullstrip_mask {_dict["skullstrip"]}'
        cmd += f' --output_dir {_dict["output_dir"]}'

        cmd += ' --target_labels'
        for label in pargs.target_labels:
            cmd += f' {label}'
        cmd += ' --wm_labels'
        for label in pargs.wm_labels:
            cmd += f' {label}'
        
        cmd += f' --n_atrophy_iters {pargs.n_atrophy_iters}'
        cmd += f' --upsampling_factor {pargs.upsampling_factor}'

        cmd += f' --hemis_template {_dict["hemis_template"]}'
        cmd += ' --lh_GM_template_labels'
        for label in lh_GM_template_labels:
            cmd += f' {label}'
        cmd += ' --lh_WM_template_labels'
        for label in lh_WM_template_labels:
            cmd += f' {label}'
        cmd += ' --rh_GM_template_labels'
        for label in rh_GM_template_labels:
            cmd += f' {label}'
        cmd += ' --rh_WM_template_labels'
        for label in rh_WM_template_labels:
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
    # Required args
    parser.add_argument('-l', '--target_labels', type=int, nargs='+',
                        help='Value of target labels for atrophy induction (always required)')

    # Required (normal mode)
    parser.add_argument('-p', '--parcellation', type=str, default='aparc+aseg.mgz',
                        help='Path to input cortical parcellation image')
    parser.add_argument('-m', '--skullstrip_mask', type=str, default='brainmask.mgz',
                        help='Path to input skull stripped image to create mask')
    parser.add_argument('-o', '--output_dir', type=str,
                        help='Path to output directory (required w/ normal mode)')

    # Required (FS mode)
    parser.add_argument('-s', '--s', nargs='*',
                        help='Process a series of FS recon-all subjects (enables FS mode)')
    parser.add_argument('--sd',
                        help='Set the subjects directory (overrides the SUBJECTS_DIR env variable '
                        'and also enables FS mode)')

    # Erosion morphology parameters
    parser.add_argument('-n', '--n_atrophy_iters', type=int, default=4,
                        help='Number of binary morphology iterations to induce synthetic atrophy')
    parser.add_argument('-f', '--upsampling_factor', type=float, default=4.,
                        help='Factor by which to upsample image resolution for atrophy induction')
    parser.add_argument('-w', '--wm_labels', nargs='*', type=int, default=[2, 41],
                        help='Value(s) of wm label ipsilateral to target label(s)')

    # Target image/surface data
    parser.add_argument('-i', '--input_images', nargs='*', type=str,
                        help='Path(s) to image(s) in which to induce synthetic atrophy')
    parser.add_argument('--input_GMs', nargs='*', type=str,
                        help='Path(s) to gray matter (GM/pial) surface(s) in which to induce '
                        'synthetic atrophy')
    parser.add_argument('--input_WMs', nargs='*', type=str,
                        help='Path(s) to white matter (WM) surface(s) in which to induce synthetic '
                        'atrophy')
    parser.add_argument('--RAS', action='store_true',
                        help='Convert input target surface vertices from RAS to LPS coordinates '
                        '(default = on in FS mode)')

    parser.add_argument('--output_images', nargs='*', type=str,
                        help='Paths to output images with induced synthetic atrophy (overrides '
                        'output_dir, should match number of input images if provided). If none '
                        'provided, or if running in FS mode, outputs will be written to '
                        '<output_dir>/<basename(input)>.RBSA.mgz for each input in input_images.')
    parser.add_argument('--output_GMs', nargs='*', type=str,
                        help='Paths to output gray matter (GM/pial) surfaces with induced '
                        'synthetic atrophy (overrides output_dir, should match number of input GM '
                        'surfaces, if provided). If no paths provided, or if running in FS mode, '
                        'outputs will be written to <output_dir>/<basename(input)>.RBSA.vtp for '
                        'each input in input_GMs')
    parser.add_argument('--output_WMs', nargs='*', type=str,
                        help='Paths to output white matter (WM) surfaces with induced synthetic '
                        'atrophy (overrides output_dir, should match number of input WM surfaces, '
                        'if provided). If no paths provided, or if running in FS mode, outputs '
                        'will be written to <output_dir>/<basename(input)>.RBSA.vtp for each input '
                        'in input_WMs')

    # MSDD inputs
    parser.add_argument('--MSDD', action='store_true',
                        help='Flag to calculate mean surface displacement different (MSDD) in'
                        ' target atrophy labels. Providing an input for --hemi_template will set '
                        'this flag to true as well.')
    parser.add_argument('-t', '--hemis_template', type=str, default='ribbon.mgz',
                        help='Path to input image used to generate GM/WM templates for both the '
                        'left and right hemispheres (required for calculating MSDD, default in '
                        'FreeSurfer mode is ribbon.mgz).')
    parser.add_argument('--lh_GM_template_labels', nargs='+', type=int, default=2,
                        help='Value of left GM label(s) in the input --hemis_template (default is '
                        '2, as in ribbon.mgz)')
    parser.add_argument('--lh_WM_template_labels', nargs='+', type=int, default=41,
                        help='Value of left WM label(s) in the input --hemis_template (default is '
                        '41, as in ribbon.mgz)')
    parser.add_argument('--rh_GM_template_labels', nargs='+', type=int, default=3,
                        help='Value of right GM label(s) in the input --hemis_template (default is '
                        '3, as in ribbon.mgz)')
    parser.add_argument('--rh_WM_template_labels', nargs='+', type=int, default=42,
                        help='Value of right WM label(s) in the input --hemis_template (default is '
                        '42, as in ribbon.mgz)')

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
        out_image_paths = [
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
            f'Input skullstripped image f{skullstrip_mask} does not exist'
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
            else [os.path.join(output_dir, f'{_remove_ext(os.path.basename(x))[0]}.RBSA.nii.gz')
                  for x in input_images]
        )

        out_dict['input_GMs'] = None if input_GMs is None else [
            x if os.path.isfile(x) else _error(f'Input target file {x} does not exist')
            for x in input_GMs
        ]
        out_dict['output_GMs'] = (
            None if input_GMs is None
            else output_GMs if output_GMs is not None
            else [os.path.join(output_dir, f'{_remove_ext(os.path.basename(x))[0]}.RBSA.vtp')
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
