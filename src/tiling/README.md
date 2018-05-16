Tiles are extracted from whole slide images using a grid based overlapping partitioning scheme. Each data file consisting of single tile is supplied to a “map” function, where they are processed producing segmented boundaries.

The generate_multi_images method is for a set of all images and is a wrapper that calls generate_tile for every image in the path(s).

## API Interface

The generate_tile function is located in the tileImage.c file under the name of 'thread_func' and the signature of the function is:
generate_tile(input_type, input_image_paths, output_path, tile_width, tile_overlap, parallelization_option)

Since the program currently only supports input_type as the local disk, and parallielization_option is always multi-threading, the signature of the function is actually this:

generate_tile(input_image_paths, output_path, tile_width, tile_overlap, thread_num)

The function parameter details are as follows:

### input_images_paths

This is the path locations of where the images are stored.

### output_path

This is the path to output the results to. 

### tile_width

This is the width of one square tile. It is set to default to 4096 if it is not passed as a parameter.

### tile_overlap

This is the buffer width. It is set to default to 5 if it is not passed as a parameter.

### thread_num

This is the number of threads you want to run. Default is 0, max is 5.

### How to run the file

To run the file, make sure you first compile the C file with the following command:
gcc -I/usr/openslide -L/usr/local/lib -lopenslide -o tileimage tileImage.c

Make sure the openslide library is present in your local disk, otherwise the link would not build

After that, run the program using either: ./tileimage or ./tileimage ('input_path', 'output_path', tile_width, tile_overlap, thread_num)
And it generate the tiles into the output_path.

## Acknowledgments

* Fusheng Wang, Associate Professor, Department of Biomedical Informatics, Department of Computer Science, Stony Brook University, Stony Brook, NY 11790
* Hoang Vo, PHD Student, Department of Computer Science, Stony Brook University, Stony Brook, NY 11790
