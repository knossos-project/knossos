__author__ = 'Joergen Kornfeld'

import threading
import re
try:
    import cv2
    cv2_available = True
except:
    cv2_available = False
import io
import math
import scipy.ndimage
import numpy as np
import glob
import multiprocessing
import Image
import os
import itertools
import scipy.special
import StringIO
import time
try:
    import fadvise
    fadvise_available = True
except:
    fadvise_available = False


class InvalidCubingConfigException(Exception):
    pass


def get_list_of_all_cubes_in_dataset(dataset_base_path, log_fn):
    all_cubes = []
    ref_time = time.time()
    for root, dirs, files in os.walk(dataset_base_path):
        #if len(files) > 1:
            #print
            # either overlay cubes or different compressions found
        for cur_file in files:
            if '.raw' in cur_file:
                all_cubes.append(os.path.join(root, cur_file))
    log_fn("Cube listing took: {0} s".format(time.time()-ref_time))
    return all_cubes


def write_knossos_conf(data_set_base_folder='',
                       scale=(10., 10., 25.),
                       boundary=(1000, 1000, 1000),
                       exp_name='stack',
                       mag=1):

    if not os.path.exists(data_set_base_folder):
        os.makedirs(data_set_base_folder)

    with open(data_set_base_folder+'knossos.conf', 'w') as conf_file:
        conf_file.write("experiment name \"{0}\";\n".format(exp_name))
        conf_file.write("boundary x {0};\n".format(boundary[0]))
        conf_file.write("boundary y {0};\n".format(boundary[1]))
        conf_file.write("boundary z {0};\n".format(boundary[2]))
        conf_file.write("scale x {0};\n".format(scale[0]))
        conf_file.write("scale y {0};\n".format(scale[1]))
        conf_file.write("scale z {0};\n".format(scale[2]))
        conf_file.write("magnification {0};\n".format(mag))

    return


class downsample_job_info(object):
    """
    Class solely made to pass arguments to the downsample worker pool easier.
    """
    def __init__(self):
        self.src_cube_paths = []
        self.src_cube_local_coords = []
        self.trg_cube_path = ''
        self.cube_edge_len = 128
        self.skip_already_cubed_layers = False

class compression_job_info(object):
    """
    Class solely made to pass arguments to the compression worker pool easier.
    """

    def __init__(self):
        self.src_cube_path = ''
        self.compressor = ''
        self.quality_or_ratio = 0
        self.pre_gauss = 0.0
        self.open_jpeg_bin_path = ''
        self.cube_edge_len = 128


def downsample_dataset(src_mag,
                       trg_mag,
                       target_path,
                       scaling,
                       boundaries,
                       exp_name,
                       num_workers,
                       buffer_size_in_cubes_downsampling,
                       num_io_threads,
                       cube_edge_len,
                       skip_already_cubed_layers,
                       log_fn,
                       dataset_base_path=''):

    # check if src mag is available
    subdirs = [name for name in os.listdir(dataset_base_path) if os.path.isdir(
        os.path.join(dataset_base_path, name))]

    mag_matcher = re.compile(r'.*mag(?P<magID>\d+)')

    found_mags = {}

    for subdir in subdirs:
        mobj = mag_matcher.search(subdir)
        try:
            found_mags[int(mobj.group('magID'))] = subdir
        except:
            log_fn("Subdirectory found in the base folder that does not comply "
                  "with the KNOSSOS dataset standard: {0}".format(subdir))

    if not src_mag in found_mags.keys():
        raise Exception('The src mag folder could not be found in the base '
                        'path folder.')

    log_fn("Analysing source dataset...")
    # we walk through the dataset structure and collect all available cubes
    all_cubes = get_list_of_all_cubes_in_dataset(
        dataset_base_path + '/' + found_mags[src_mag], log_fn)

    # -> extract x,y,z src dataset dimensions in cubes

    cube_coord_matcher = re.compile(r'.*x(?P<x>\d+)_y(?P<y>\d+)_z('
                                    r'?P<z>\d+)\.raw$')

    max_x = 0
    max_y = 0
    max_z = 0

    path_hash = {}
    for this_cube_path in all_cubes:
        mobj = cube_coord_matcher.search(this_cube_path)
        try:
            x = int(mobj.group('x'))
            y = int(mobj.group('y'))
            z = int(mobj.group('z'))

        except:
            raise Exception("Error: Corrupt cube filename in list: {0}".format(
                this_cube_path))

        path_hash[(x, y, z)] = this_cube_path

        if x > max_x:
            max_x = int(mobj.group('x'))
        if y > max_y:
            max_y = int(mobj.group('y'))
        if z > max_z:
            max_z = int(mobj.group('z'))

    if max_x < 4 and max_y < 4 and max_z < 4:
        # nothing to downsample, stopping
        log_fn("Further downsampling is useless, stopping.")
        return False

    out_path = dataset_base_path + '/mag' + str(trg_mag) + '/'
    log_fn("Downsampling to {0}".format(out_path))

    log_fn("Src dataset cube dimensions: x {0}, y {1}, z {2}".format(max_x,
                                                                    max_y,
                                                                    max_z))

    magf = float(trg_mag)

    write_knossos_conf(data_set_base_folder=target_path + '/mag' + str(
                   trg_mag) + '/',
                   scale=(scaling[0]*magf, scaling[1]*magf, scaling[2]*magf),
                   boundary=(boundaries[0]/trg_mag,
                             boundaries[1]/trg_mag,
                             boundaries[2]/trg_mag),
                   exp_name=exp_name+'_mag'+str(trg_mag),
                   mag=trg_mag)

    # compile the 8 cubes that belong together, no overlap, set to 'bogus' at
    # the incomplete borders

    downsampling_job_info = []
    for cur_x, cur_y, cur_z in itertools.product(range(0, max_x+2, 2),
                                                 range(0, max_y+2, 2),
                                                 range(0, max_z+2, 2)):

        if cur_x > max_x or cur_y > max_y or cur_z > max_z:
            path_hash[(cur_x,cur_y,cur_z)] = 'bogus'

        these_cubes = []
        these_cubes_local_coords = []
        for lx,ly,lz in itertools.product([0,1],[0,1],[0,1]):

            # fill up the borders with black
            if not path_hash.has_key((cur_x+lx, cur_y+ly, cur_z+lz)):
                path_hash[(cur_x+lx, cur_y+ly, cur_z+lz)] = 'bogus'

            these_cubes.append(path_hash[(cur_x+lx, cur_y+ly, cur_z+lz)])
            these_cubes_local_coords.append((lx, ly, lz))

        this_job_info = downsample_job_info()
        this_job_info.src_cube_paths = these_cubes
        this_job_info.src_cube_local_coords = these_cubes_local_coords
        this_job_info.skip_already_cubed_layers = skip_already_cubed_layers
        this_job_info.cube_edge_len = cube_edge_len

        out_path = path_hash[(cur_x/2, cur_y/2, cur_z/2)]
        out_path = out_path.replace('mag'+str(src_mag), 'mag'+str(trg_mag))
        this_job_info.trg_cube_path = out_path

        downsampling_job_info.append(this_job_info)

    #downsample_cube(downsampling_job_info[0])

    # Split up the downsampling into chunks that can be hold in memory. This
    # allows us to separate reading and writing from the storage,
    # often massively increasing the IO performance

    if len(downsampling_job_info) > buffer_size_in_cubes_downsampling:
        # split the jobs in chunks

        # how many chunks do we need?
        chunks_required = len(downsampling_job_info) / buffer_size_in_cubes_downsampling
        chunked_jobs = np.array_split(downsampling_job_info, chunks_required)
        # convert back to python list
        chunked_jobs = [el.tolist() for el in chunked_jobs]
    else:
        chunked_jobs = [downsampling_job_info]

    log_queue = multiprocessing.Queue()
    for this_job_chunk in chunked_jobs:

        log_fn("Starting {0} workers...".format(num_workers))

        log_fn("First cube in this chunk: {0}".format(this_job_chunk[
                                                     0].trg_cube_path))
        ref_time = time.time()
        worker_pool = multiprocessing.Pool(num_workers, downsample_cube_init,
                                           [log_queue])
        #worker_pool = multiprocessing_threads.Pool(num_workers)

        cubes = worker_pool.map(downsample_cube, this_job_chunk, chunksize=10)
        worker_pool.close()

        while not log_queue.empty():
            log_output = log_queue.get()
            log_fn(log_output)
        #cubes = map(downsample_cube, this_job_chunk)

        worker_pool.join()

        log_fn("Downsampling took on avg per cube: {0} s".format((time.time()-
                                                                 ref_time)/
                                                                len(this_job_chunk)))

        write_times = []
        write_threads = []

        # start writing the cubes
        for cube_data, job_info in itertools.izip(cubes, this_job_chunk):
            prefix = os.path.dirname(job_info.trg_cube_path)
            cube_full_path = job_info.trg_cube_path
            ref_time = time.time()
            if cube_data == 'skipped':
                write_times.append(time.time()-ref_time)
                #print("Skipped cube {0}".format(job_info.trg_cube_path))
                continue

            # One could also try multiprocessing or multiprocessing dummy
            if threading.active_count() < num_io_threads:
                this_thread = threading.Thread(target=write_cube,
                                          args=[cube_data, prefix,
                                                cube_full_path])

                write_threads.append(this_thread)
                this_thread.start()

            else:
                while threading.active_count() >= num_io_threads:
                    time.sleep(0.001)
                this_thread = threading.Thread(target=write_cube,
                                          args=[cube_data, prefix,
                                                cube_full_path])

                write_threads.append(this_thread)
                this_thread.start()

            write_times.append(time.time()-ref_time)

        log_fn("Writing took on avg per cube: {0} s".format(np.mean(
            write_times)))

        # wait until all writes are finished
        [x.join() for x in write_threads]

    #raise()
    return True


def downsample_cube(job_info):
    ref_time=time.time()
    # the first cube in the list contains the new coordinate of the created
    # downsampled out-cube

    cube_edge_len = job_info.cube_edge_len
    skip_already_cubed_layers = job_info.skip_already_cubed_layers

    down_block = np.zeros([cube_edge_len*2,cube_edge_len*2,cube_edge_len*2],
                          dtype=np.uint8, order='F')

    if skip_already_cubed_layers:
        if os.path.exists(job_info.trg_cube_path):
            if os.path.getsize(job_info.trg_cube_path) == cube_edge_len**3:
                return 'skipped'

    if fadvise_available:
        for src_path in job_info.src_cube_paths:
            fadvise.willneed(src_path)

        #time.sleep(0.2)

    for path_to_src_cube, src_coord in zip(job_info.src_cube_paths,
                                            job_info.src_cube_local_coords):
        if path_to_src_cube == 'bogus':
            continue

        # Yes, I know the numpy fromfile function - this is significantly
        # faster on our cluster
        content = ''
        #buffersize=131072*2
        fd = io.open(path_to_src_cube, 'rb')
                    # buffering = buffersize)
        #for i in range(0, (cube_edge_len**3 / buffersize) + 1):
        #    content += fd.read(buffersize)
        content = fd.read(-1)
        fd.close()

        this_cube = np.fromstring(content, dtype=np.uint8).reshape([
        cube_edge_len,cube_edge_len,cube_edge_len], order='F')


        #this_cube = np.fromfile(path_to_src_cube, dtype=np.uint8).reshape([
        #    cube_edge_len,cube_edge_len,cube_edge_len], order='F')

        this_cube = np.swapaxes(this_cube, 0, 2)
        #this_cube = np.swapaxes(this_cube, 1, 2)

        down_block[src_coord[2]*cube_edge_len:\
                   src_coord[2]*cube_edge_len+cube_edge_len,
                   src_coord[1]*cube_edge_len:\
                   src_coord[1]*cube_edge_len+cube_edge_len,
                   src_coord[0]*cube_edge_len:\
                   src_coord[0]*cube_edge_len+cube_edge_len] = this_cube

        #down_block = np.swapaxes(down_block, 0, 1)
    #raise()

    # It is not clear to me whether this zooming function does actually the
    # right thing. One should
    # first filter the data and then
    # re-sample to avoid aliasing. The prefilter setting is possibly not
    # working correctly, as the scipy documentation appears to be not in
    # agreement with the actual source code, so that pre-filtering is only
    # triggered for orders > 1, even if set to True. I assume that bilinear
    # or higher order re-sampling itself is "filtering" and is "good
    # enough" for our case.
    # This website by Stephan Saalfeld has some interesting details,
    # but there is a ton of material coming from the photography community.
    # http://fiji.sc/Downsample
    # My personal experience: Avoid nearest neighbor (ie naive
    # re-sampling without any filtering), especially
    # for noisy images. On top of that, the gains of more sophisticated
    # filters become less clear, and data and scaling factor dependent.
    down_block = scipy.ndimage.interpolation.zoom(down_block, 0.5,
                                                  output=np.uint8,
                                                  order=1,
                                                  # 1: bilinear
                                                  # 2: bicubic
                                                  mode='nearest', # this does
                                                  # not mean nearest
                                                  # interpolation,
                                                  # it corresponds to how the
                                                  # borders are treated.                                           # treat the borders.
                                                  prefilter=True)

    # extract directory of out_path
    #if not os.path.exists(os.path.dirname(job_info.trg_cube_path)):
    #    os.makedirs(os.path.dirname(job_info.trg_cube_path))

    downsample_cube.log_queue.put("Downsampling took: {0} s for {1}".format(
        time.time()-ref_time, job_info.trg_cube_path))

    #down_block.tofile(job_info.trg_cube_path)

    return down_block


def downsample_cube_init(log_queue):
    downsample_cube.log_queue = log_queue


def compress_dataset(dataset_base_path,
                     compressor,
                     quality,
                     num_workers,
                     pre_comp_gauss_filter,
                     log_fn):

    mag_dirs = [dataset_base_path + '/' + name
                for name in os.listdir(dataset_base_path)
                if os.path.isdir(os.path.join(dataset_base_path, name))]

    log_fn("Analysing source dataset...")

    list_of_all_cubes = []
    for mag_dir in mag_dirs:
        list_of_all_cubes.extend(get_list_of_all_cubes_in_dataset(
            mag_dir, log_fn))

    compress_job_infos = []
    for cube_path in list_of_all_cubes:
        this_job_info = compression_job_info()

        this_job_info.compressor = compressor
        this_job_info.quality_or_ratio = quality
        this_job_info.src_cube_path = cube_path
        this_job_info.pre_gauss = pre_comp_gauss_filter

        compress_job_infos.append(this_job_info)

    log_fn("Starting {0} workers...".format(num_workers))
    log_queue = multiprocessing.Queue()
    worker_pool = multiprocessing.Pool(num_workers,
                                       initializer=compress_cube_init,
                                       initargs=[log_queue])
    # distribute cubes to worker pool

    async_result = worker_pool.map_async(compress_cube, compress_job_infos,
                                 chunksize=10)
    worker_pool.close()

    while not async_result.ready():
        log_output = log_queue.get()
        log_fn(log_output)
    worker_pool.join()


def compress_cube(job_info):
    ref_time=time.time()
    cube_edge_len = job_info.cube_edge_len
    open_jpeg_bin_path = job_info.open_jpeg_bin_path

    if job_info.compressor == 'jpeg':
        if job_info.quality_or_ratio < 40:
            raise Exception('Improbable quality value set for jpeg as '
                            'compressor: Use values between 50 and 90 for '
                            'reasonable results. Higher value->better quality.')
    elif job_info.compressor == 'j2k':
        if job_info.quality_or_ratio > 20:
            raise Exception('Improbable quality value set for j2k as '
                            'compressor: Use values between 2 and 10 for '
                            'reasonable results. Lower value->better quality.')

    cube_path_without_ending = os.path.splitext(job_info.src_cube_path)[0]

    if fadvise_available:
        fadvise.willneed(job_info.src_cube_path)

    if job_info.compressor == 'jpeg':
        cube_raw = np.fromfile(job_info.src_cube_path, dtype=np.uint8)
        cube_raw = cube_raw.reshape(cube_edge_len*cube_edge_len, cube_edge_len)

        if job_info.pre_gauss > 0.0:
            # blur only in 2d
            if cv2_available:
                cv2.GaussianBlur(cube_raw, (5,5), job_info.pre_gauss, cube_raw)
            else:
                cube_raw = scipy.ndimage.filters.gaussian_filter(cube_raw,
                                                            job_info.pre_gauss)

        # the exact shape of the 2d representation for compression is
        # actually important!
        # PIL performs reasonably fast; one could try libjpeg-turbo to make
        # it even faster, but IO is the bottleneck anyway
        cube_img = Image.fromarray(cube_raw)

        cube_img.save(cube_path_without_ending+'.jpg',
                      quality=job_info.quality_or_ratio)

    elif job_info.compressor == 'j2k':
        cmd_string = open_jpeg_bin_path + \
                     ' -i ' + job_info.src_cube_path +\
                     ' -o ' + cube_path_without_ending + '.jp2' +\
                     ' -r ' + str(job_info.quality_or_ratio) +\
                     ' -b 64,64 -s 1,1 -n 3 ' +\
                     ' -F ' + str(cube_edge_len) + ',' +\
                     str(cube_edge_len*cube_edge_len) + ',1,8,u'
        os.system(cmd_string)

    # print here, not log_fn, because log_fn may not be able to write to some
    # data structure from multiple processes.
    compress_cube.log_queue.put("Compress, writing of {0} took: {1} s".format(
        job_info.src_cube_path,
                                                        time.time()-ref_time))

    return


def compress_cube_init(log_queue):
    compress_cube.log_queue = log_queue


def write_cube(cube_data, prefix, cube_full_path):
    #ref_time=time.time()
    if not os.path.exists(prefix):
        os.makedirs(prefix)

    try:
        cube_data.tofile(cube_full_path)
        #print("writing took: {0}s".format(time.time()-ref_time))
    except IOError:
        # no log_fn due to multithreading
        print("Could not write cube: {0}".format(cube_full_path))

    return


def init_from_source_dir(user_config, log_fn):
    """
    Compute certain cubing parameters from the set of parameters specified by the user.

    Parameters
    ---------

    config : dict
        See cube_dataset method docstring


    log_fn : function with str parameter

    Returns
    -------

    num_x_cubes_per_pass : int

    num_y_cubes : int

    num_z_cubes : int

    all_source_files : list of str

    num_passes_per_cube_layer : int

    """

    # get sorted list of source files
    all_source_files = glob.glob(
        user_config['source_path'] + '/*.' + user_config['source_format'])
    all_source_files.sort()
    num_z = len(all_source_files)
    # open the first image and extract the relevant information - all images are
    # assumed to have equal dimensions!
    if user_config['source_format'] != 'raw':
        test_img = Image.open(all_source_files[0])
        test_data = np.array(test_img)
        # knossos uses swapped xy axes relative to images
        test_data = np.swapaxes(test_data, 0, 1)
        user_config['source_dims'] = test_data.shape
        user_config['source_dtype'] = test_data.dtype

    # determine the number of passes required for each cube layer - if several
    # passes are required, we split the xy plane up in X cube_edge_len chunks,
    # always with full y height
    num_x_cubes = int(math.ceil(
        user_config['source_dims'][0] / float(user_config['cube_edge_len'])))
    num_y_cubes = int(math.ceil(
        user_config['source_dims'][1] / float(user_config['cube_edge_len'])))
    num_z_cubes = int(math.ceil(
        num_z / float(user_config['cube_edge_len'])))

    if num_x_cubes * num_y_cubes < user_config['buffer_size_in_cubes']:
        log_fn('Buffer size sufficient for a single pass per z cube layer')
        num_passes_per_cube_layer = 1
        num_x_cubes_per_pass = num_x_cubes
    else:
        log_fn('Buffer size not sufficient for single pass per z cube layer - '
              'either increase the buffer size or accept the longer cubing '
              'time due to IO overhead.')
        num_passes_per_cube_layer = int(math.ceil(
            user_config['buffer_size_in_cubes'] / num_y_cubes))
        num_x_cubes_per_pass = int(math.floor(
            num_x_cubes / num_passes_per_cube_layer))

    return num_x_cubes_per_pass, num_y_cubes, num_z_cubes, all_source_files, \
        num_passes_per_cube_layer


def make_mag1_cubes_from_z_stack(user_config,
                                 all_source_files,
                                 num_x_cubes_per_pass,
                                 num_y_cubes,
                                 num_z_cubes,
                                 num_passes_per_cube_layer,
                                 log_fn):
    target_path = user_config['target_path']
    exp_name = user_config['exp_name']
    skip_already_cubed_layers = user_config['skip_already_cubed_layers']
    cube_edge_len = user_config['cube_edge_len']
    source_dtype = user_config['source_dtype']
    source_dims = user_config['source_dims']
    same_knossos_as_tif_stack_xy_orientation = \
        user_config['same_knossos_as_tif_stack_xy_orientation']
    num_io_threads = user_config['num_io_threads']

    # we iterate over the z cubes and handle cube layer after cube layer
    for cur_z in range(0, num_z_cubes):
        if skip_already_cubed_layers:
            # test whether this layer contains already "cubes"
            prefix = os.path.normpath(os.path.abspath(target_path + '/mag1' +
                                                          '/x%04d/y%04d/z%04d/'
                                                          % (1,
                                                             1,
                                                             cur_z)))

            cube_full_path = os.path.normpath(prefix +
                                         '/%s_mag%d_x%04d_y%04d_z%04d.raw'
                                         % (exp_name, 1,  # 1 indicates mag1
                                            1,
                                            1,
                                            cur_z))
            if os.path.exists(cube_full_path):
                log_fn("Skipping cube layer: {0}".format(cur_z))
                continue

        for cur_pass in range(0, num_passes_per_cube_layer):
            # allocate memory for this layer
            this_layer_out_block = np.zeros([cube_edge_len,
                                             num_x_cubes_per_pass * cube_edge_len,
                                             num_y_cubes * cube_edge_len],
                                            dtype=source_dtype)

            # create a src binary copy mask to speed-up the following buffer-filling
            # process; this mask is made for the source_dims
            copy_mask_src = np.zeros([source_dims[0], source_dims[1]])
            this_pass_x_start = cur_pass * num_x_cubes_per_pass * cube_edge_len
            this_pass_x_end = (cur_pass+1) * num_x_cubes_per_pass * cube_edge_len

            if this_pass_x_end > source_dims[0]:
                this_pass_x_end = source_dims[0]

            copy_mask_src[this_pass_x_start:this_pass_x_end,:] = 1
            copy_mask_src = (copy_mask_src == 1)

            # create a trg binary copy mask to speed-up the following buffer-filling
            # process; this mask is made for the target buffer dims
            #copy_mask_trg = np.zeros([num_x_cubes*cube_edge_len,
            #                             num_y_cubes*cube_edge_len])
            #copy_mask_trg[0:num_x_cubes_per_pass*cube_edge_len, :] = 1
            #copy_mask_trg = (copy_mask_trg == 1)
            #if cur_z > 2:
            #    raise()

            # tell the kernel that we will need the next layer; not sure how much
            # we gain here, this needs to be evaluated
            #if fadvise_available:
            #    print("Running fadvise for this layer...")
            #    for z in range(cur_z*cube_edge_len, (cur_z+1)*cube_edge_len):
            #        fadvise.willneed(all_source_files[z])


            # fill the buffer with data
            local_z = 0
            for z in range(cur_z*cube_edge_len, (cur_z+1)*cube_edge_len):
                try:
                    log_fn("Loading {0}".format(all_source_files[z]))
                except:
                    log_fn("No more tifs available.")
                    break

                ref_time = time.time()

                #if source_format == 'raw':
                #    this_layer = np.fromfile(all_source_files[z],
                #                             shape=[source_dims[0],
                #                                    source_dims[1]],
                #                             dtype=source_dtype)
                #else:
                #ref_time = time.time()
                fsize = os.stat(all_source_files[z]).st_size
                buffersize = 524288/2 # optimal for soma cluster
                content = ''
                # This is optimized code, do not think that a single line
                # would be faster. At least on the soma MPI cluster,
                # the default buffering values (read entire file into buffer
                # instead of smaller chunks) leads to delays and slowness.
                fd = io.open(all_source_files[z], 'r+b', buffering = buffersize)
                for i in range(0, (fsize / buffersize) + 1):
                    content += fd.read(buffersize)
                fd.close()

                PIL_image = Image.open(StringIO.StringIO(content))
                this_layer = np.array(PIL_image)

                # This stupid swap axes call costs us 50% of the image loading
                # time. Not sure how to speed it up. np.swapaxes generates a
                # view, it hurts only then when you access the actual data. One
                # could try to swap the entire out memory buffer later on,
                # this might be faster.
                if same_knossos_as_tif_stack_xy_orientation:
                    this_layer = np.swapaxes(this_layer, 0, 1)

                # copy the data for this pass into the output buffer
                if num_passes_per_cube_layer > 1:
                    this_layer_out_block[z, :, :] = this_layer[copy_mask_src]
                else:
                    # single buffer fill - this_layer_out_block is larger than
                    # the individual data files due to the rounding to
                    # cube_edge_len chunks, we have to avoid a dimension mismatch
                    # therefore; it is crucial that the slowest changing index,
                    # z, is at the first index (c-style order). The time
                    # difference is 100x for big amounts of data!

                    this_layer_out_block[local_z,
                                         0:this_layer.shape[0],
                                         0:this_layer.shape[1]] = this_layer

                local_z += 1
                log_fn("reading took {0}".format(time.time()-ref_time))

            write_times = []
            write_threads = []

            # write out the cubes for this z-cube layer and buffer
            for cur_x in range(0, num_x_cubes_per_pass):
                for cur_y in range(0, num_y_cubes):
                    ref_time = time.time()
                    glob_cur_x_cube = cur_x + cur_pass * num_passes_per_cube_layer
                    glob_cur_y_cube = cur_y
                    glob_cur_z_cube = cur_z

                    # slice cube_data out of buffer
                    x_start = cur_x*cube_edge_len
                    x_end = (cur_x+1)*cube_edge_len

                    y_start = cur_y*cube_edge_len
                    y_end = (cur_y+1)*cube_edge_len

                    cube_data = this_layer_out_block[
                        :, x_start:x_end, y_start:y_end]

                    cube_data = np.swapaxes(cube_data, 1, 2)

                    prefix = os.path.normpath(os.path.abspath(target_path +
                                                              '/mag1' +
                                                              '/x%04d/y%04d/z%04d/'
                                                              % (glob_cur_x_cube,
                                                                 glob_cur_y_cube,
                                                                 glob_cur_z_cube)))

                    cube_full_path = os.path.normpath(prefix +
                                                 '/%s_mag%d_x%04d_y%04d_z%04d.raw'
                                                 % (exp_name, 1,  # 1 indicates mag1
                                                    glob_cur_x_cube,
                                                    glob_cur_y_cube,
                                                    glob_cur_z_cube))
                    log_fn("Writing cube {0}".format(cube_full_path))

                    # threaded cube writing gave a speed up of a factor of 10(!!)
                    if threading.active_count() < num_io_threads:
                        this_thread = threading.Thread(
                            target=write_cube,
                            args=[cube_data,
                                  prefix,
                                  cube_full_path])

                        write_threads.append(this_thread)
                        this_thread.start()

                    else:
                        while threading.active_count() >= num_io_threads:
                            time.sleep(0.001)
                        this_thread = threading.Thread(
                            target=write_cube,
                            args=[cube_data,
                                  prefix,
                                  cube_full_path])

                        write_threads.append(this_thread)
                        this_thread.start()

                    write_times.append(time.time() - ref_time)

            log_fn("writing took on avg per cube: {0} s".format(np.mean(
                write_times)))

            # wait until all writes are finished for this layer
            [x.join() for x in write_threads]


def knossos_cuber(user_config, log_fn):
    """
    Cube a dataset.

    Parameters
    ----------

    log_fn : function with single str parameter
        Function with str parameter that processes log / debug output.

    user_config : dict
        Configuration for the cubing job.

        Keys / values are as follows:

        'open_jpeg_bin_path' : str
            Path to JPEG2000 compression executable, only needed when using
            JPEG2000.

        'source_format' : str
            Whatever image format PIL can read in, or 'raw' for faster cubing.
            Must be identical to the file name extension of input data.

        'source_dims' : length 2 list
            Input image dimensions. Required for raw images, otherwise
            determined directly from input images.

        'source_dtype' : numpy datatype
            Input image pixel data type, required only for raw. E.g.
            numpy.uint8.

        'source_path' : str
            Path to raw image input data

        'target_path' : str
            Path for cube data output

        'exp_name' : str
            Arbitrary experiment name identifier

        'scaling' : 3-tuple of float
            Voxel scaling, physical units per voxel

        'boundaries' : 3-tuple of int
            Edge length of whole dataset, only required if not starting from
            2D images.

        'perform_mag1_cubing' : bool
            Whether to generate mag1 cubes from 2D images

        'skip_already_cubed_layers' : bool
            Skip layers / cubes that contain already cubes or are already
            existent. Useful to recover from long runs that were killed.

        'perform_downsampling' : bool
            Whether to generate downsampled cube sets.

        'mags_to_gen' : int
            Generates log_2(mags_to_gen) downsampled 3D pyramids levels

        'perform_compression' : bool
            Whether to generate compressed cubes, important for web streaming.

        'compression_algo' : str
            'jpeg' (JPEG) or 'j2k' (JPEG2000)

        'out_comp_quality' : int
            0-100 for jpg (100 is best), 1-10 for j2k (1 == lossles?)

        'cube_edge_len' : int
            Edge length for the cubes, everything except for 128 is untested.

        'pre_comp_gauss_filter' : float
            Gaussian filter before compression to reduce artefacts, this really
            helps a lot for noisy data. Do NOT use it for near-noise-free data
            (I would say SNR > 10).

        'same_knossos_as_tif_stack_xy_orientation' : bool
            Whether to swap axes.  If yes, performs costly xy axes swap; this
            swap takes currently about as long as reading in each image file.

        'buffer_size_in_cubes' : int
            How many cubes will be used. Ideally, this number should be greater
            than the number of cubes in a single z-layer of cubes.

        'num_donwsampling_cores' : int
            Number of CPU cores to use for downsampling.

        'num_compression_cores' : int

        'num_io_threads' : int
            Use more than cores available; 20 is good if you're on a local RAID
            array, higher numbers might help if you read / write from NFS.
    """

    boundaries = user_config['boundaries']

    if user_config['perform_mag1_cubing']:
        num_x_cubes_per_pass, num_y_cubes, num_z_cubes, all_source_files, \
            num_passes_per_cube_layer = init_from_source_dir(user_config,
                                                             log_fn)

        mag1_ref_time = time.time()

        make_mag1_cubes_from_z_stack(
            user_config,
            all_source_files,
            num_x_cubes_per_pass,
            num_y_cubes,
            num_z_cubes,
            num_passes_per_cube_layer,
            log_fn)

        boundaries = (
            user_config['source_dims'][0],
            user_config['source_dims'][1],
            len(all_source_files))

        write_knossos_conf(data_set_base_folder=user_config['target_path'] +
                                                '/mag1/',
                           scale=user_config['scaling'],
                           boundary=boundaries,
                           exp_name=user_config['exp_name']+'_mag1',
                           mag=1)

        total_mag1_time = time.time() - mag1_ref_time
        log_fn("Mag 1 succesfully cubed. Took {0} h".format(
            total_mag1_time/3600))

    if user_config['perform_downsampling']:
        total_down_ref_time = time.time()
        curr_mag = 2
        while curr_mag <= user_config['mags_to_gen']:
            worked = downsample_dataset(
                curr_mag/2,
                curr_mag,
                target_path=user_config['dataset_base_path'],
                scaling=user_config['scaling'],
                boundaries=boundaries,
                exp_name=user_config['exp_name'],
                num_workers=user_config['num_downsampling_cores'],
                buffer_size_in_cubes_downsampling=user_config[
                    'buffer_size_in_cubes_downsampling'],
                num_io_threads=user_config['num_io_threads'],
                cube_edge_len=user_config['cube_edge_len'],
                skip_already_cubed_layers=user_config['skip_already_cubed_layers'],
                log_fn=log_fn,
                dataset_base_path=user_config['target_path'])

            if worked:
                log_fn("Mag {0} succesfully cubed.".format(curr_mag))
                curr_mag *= 2
            else:
                log_fn("Done with downsampling.")
                break

        log_fn("All mags generated. Took {0} h.".format((time.time()-
                                                        total_down_ref_time)/3600))

    if user_config['perform_compression']:
        total_comp_ref_time = time.time()
        compress_dataset(
            user_config['target_path'],
            user_config['compression_algo'],
            user_config['out_comp_quality'],
            user_config['num_compression_cores'],
            user_config['pre_comp_gauss_filter'],
            log_fn)
        log_fn("Compression done. Took {0} h.".format((time.time()-
                                                     total_comp_ref_time)/3600))

    log_fn('All done.')

def set_config_defaults():
    """
    Return a config dictionary that contains sensible default values for
    a cubing run starting from a set of TIFF files.

    Returns
    -------

    config : dict
        See knossos_cuber docstring.
    """

    user_config = {
        'open_jpeg_bin_path': '/usr/local/openjpeg/bin/image_to_j2k',
        'source_format': 'tif',
        'scaling': (1.0, 1.0, 1.0),
        'perform_mag1_cubing': True,
        'skip_already_cubed_layers': True,
        'perform_downsampling': True,
        'mags_to_gen': 2**20,
        'boundaries': None,
        'perform_compression': True,
        'cube_edge_len': 128,
        'compression_algo': 'jpeg',
        'out_comp_quality': 70,
        'pre_comp_gauss_filter': 0.5,
        'same_knossos_as_tif_stack_xy_orientation': False,
        'buffer_size_in_cubes': 1000,
        'buffer_size_in_cubes_downsampling': 1000,
        'num_downsampling_cores': 10,
        'num_compression_cores': 20,
        'num_io_threads': 20,
        'source_dims': None,
        'source_dtype': np.uint8,
    }

    return user_config


def validate_config(user_config):
    if not user_config['perform_mag1_cubing'] and not user_config['boundaries']:
        raise InvalidCubingConfigException(
            'When starting from mag1 cubes, the dataset boundaries must be'
            'specified.')

    if user_config['source_format'].lower() == 'raw':
        raise InvalidCubingConfigException('Raw images are currently '
                                           'unsupported.')
    #   if not user_config['source_dims'] or not user_config['source_dtype']:
    #       raise InvalidCubingConfigException(
    #           'When reading from 2D raw images, source image size and data'
    #           'type must be specified.')


if __name__ == '__main__':
    config = set_config_defaults()

    config['source_path'] = '/home/svara/tmp/test_tiff_raw/'
    config['target_path'] = '/home/svara/tmp/test_tiff_raw/cubes/'
    config['dataset_base_path'] = '/home/svara/tmp/test_tiff_raw/cubes/'
    config['exp_name'] = 'test'
    config['scaling'] = (9.0, 9.0, 21.0)
    #config['perform_mag1_cubing'] = False
    #config['boundaries'] = [1024, 1024, 516]

    config['source_format'] = 'raw'
    config['source_dims'] = (1024, 1024)

    validate_config(config)

    def logger_fn(text):
        print text

    knossos_cuber(config, logger_fn)