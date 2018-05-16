Tiles are extracted from whole slide images using a grid based overlapping partitioning scheme. Each data file consisting of single tile is supplied to a “map” function, where they are processed producing segmented boundaries.

## API Interface

The generate_multi_images function is located in the tileImage.c file and the signature of the function is:
generate_multi_images(input_type, input_image_paths, tile_width, tile_overlap, output_path, compression_type, parallelization_option)
The function parameter details are as follows:

### input_type

input_type is either Hadoop HDFS, Database (DB2/Postgres) or local disk.

### input_images_paths

This is the path locations of where the images are stored.

### tile_width

This is the width of one square tile. It is set to default to 4096 if it is not passed as a parameter.

### tile_overlap

This is the buffer width. It is set to default to 5 if it is not passed as a parameter.

### output_path

This is the path to output the results to. 

### compression_type

This is the type of compression that is used. It can be either zlib or no compression.

### parallelization_option

This is the type of parallelization that is used to run the method. We can use either Spark, Hadoop, or multi-threading.

The generate_multi_images method is for a set of all images and is a wrapper that calls generate_tile for every image in the path(s).

## Acknowledgments

* Fusheng Wang, Associate Professor, Department of Biomedical Informatics, Department of Computer Science, Stony Brook University, Stony Brook, NY 11790
* Hoang Vo, PHD Student, Department of Computer Science, Stony Brook University, Stony Brook, NY 11790
