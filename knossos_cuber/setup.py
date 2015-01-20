from distutils.core import setup

setup(
    name = 'knossos_cuber',
    packages = ['knossos_cuber'],
    version = '1.0',
    description = 'A script that converts images into a KNOSSOS-readable format.',
    author = 'Jörgen Kornfeld, Fabian Svara',
    author_email = 'Jörgen Kornfeld <joergen.kornfeld@mpimf-heidelberg.mpg.de>, Fabian Svara <fabian.svara@mpimf-heidelberg.mpg.de>',
    url = 'https://github.com/knossos-project/knossos_cuber',
    download_url = '.../tarball/0.1',
    keywords = ['converter', 'skeletonization', 'segmentation'],
    classifiers = [],
    install_requires = [
        'numpy',
        'scipy',
        'Pillow',
    ],
)