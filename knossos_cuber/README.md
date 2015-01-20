knossos_cuber
=============

A Python script that converts images into a **Knossos**-readable format.

Prerequisites
-------------

There are two possible usages of the script:

1.  From the command-line, using `knossos_cuber.py`.
2.  With a GUI, using `knossos_cuber_gui.py`.

    If you use the GUI, make sure to have *PyQt* (version 4) installed.

The script itself depends on the following modules:

*   NumPy
*   SciPy
*   Pillow

These dependencies can be installed using `pip`/`easy_install`.

*PyQt* has to be installed using your operating system's package manager (`pacman`, `apt-get`, `brew`, etc.), or downloaded from [here](http://www.riverbankcomputing.com/software/pyqt/download).

Usage
-----

1.  From the command-line:

    If you run `python knossos_cuber.py` without any arguments, you get the following output:

        usage: knossos_cuber.py [-h] [--format FORMAT] [--config CONFIG]
                        source_dir target_dir
        knossos_cuber.py: error: too few arguments

    The script expects at least 3 arguments:

    *   `--format`/`-f`: The format of your image files. Currently, the options `png`, `tif` and `jpg` are supported.
    *   `source_dir`: The path to the directory where your images are located in.
    *   `target_dir`: Path to the output directory.

    For example: `python knossos_cuber.py -f png input_dir output_dir`

    If you run the script like this, `knossos_cuber` will use sane default parameters for the dataset generation. These parameters can be found in `config.ini`, and can be overridden by supplying an own configuration file using the `--config`/`-c` argument:

    For example: `python knossos_cuber.py -f png -c my_conf.ini input_dir output_dir`

    For a graphical configuration of this script, run `python knossos_cuber_gui.py`. This script accepts an additional argument, `--config`/`-c`, that should be the path to another configuration file.

    You will need to have *PyQt4* installed to be able to run the graphical mode.

2.  Pre-packaged binaries for Windows and Mac are currently in development.


Afterword
---------

`knossos_cuber` is part of **Knossos**. Head to **Knossos'** [main site](http://www.knossostool.org) or our [Github Repository](https://github.com/knossos-project/knossos) for more information.