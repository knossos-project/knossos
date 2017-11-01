# KNOSSOS 5.1

Release Date: 27th October 2017

## New Features:

### Annotation

* Mesh slice in orthogonal viewports. Previously the mesh was only visible in the 3D view.
* Mesh transparency can be adjusted.
* Skeleton and Segmentation table entries are now copyable to clipboard.
* Skeleton nodes table: A new context menu entry lets you select all trees that contain selected nodes.
* Segmentation: Rendering of borders of segmentation objects can be toggled off now.

### Settings

* Settings per annotation file: Just add a settings.ini to your .k.zip.
* Beginning with this version, settings are maintained and compatible between different versions.

### Datasets

* Support for new NeuroData Store url naming scheme (previous scheme still works): /ocp/ca/ ↦ /nd/sd
* We added a new default dataset

### Other

* Cheatsheet: An informative side panel with an overview of the active work mode including shortcuts and links to related settings.
* Snapshot preview: Let’s you copy to clipboard, no need to save to file.
* There’s now a progress bar indicating network activity.

## Bug Fixes: 

* Merging trees now correctly merges their meshes, too.
* The display of translucent meshes has been improved.
* Segmentation bucket fill: Buggy behavior at vp and movement boundaries have been fixed. Performance of the bucket fill has been improved.
* Some issues when filtering in the annotation tables have also been fixed.


# KNOSSOS 5.0.1

Release Date: 18th March 2017

## Enhancements
- Improved performance of operations on large number of trees/nodes
- Keep orientation when switching between equivalent datasets
- Adapted scale bar length to display »rounder« values
- Added base grid for 3D viewport’s boundary box

## Fixes
- Node view selection only worked in one work mode
- Corrected task management display
- Fixed arbitrary viewport regression (viewport sometimes went mostly black)
- Fixed zoom-in shortcuts

## Platform Notes
- The AppImage does not run on Ubuntu versions < 16 LTS and on Debian versions < stretch.


# KNOSSOS 5

Release Date: 16th January 2017

## Features

### Dataset:

The dataset loading mechanism was updated to support a wider range of dataset configurations, including
- local jpeg-compressed raw datasets (additionally to remote ones)
- customizable dataset cube sizes
- various dataset backends. For example a dataset url containing “/ocp/ca/” will be interpreted as an open connectome dataset.

### Mesh:

It is now possible to load triangle meshes into KNOSSOS. Meshes are a set of vertices, colors and indices. In KNOSSOS they are associated to one skeleton tree and are displayed inside the 3D viewport.

Simply insert a mesh file (`.ply`) into a KNOSSOS annotation file (`.k.zip`) and it will be loaded with it.

Visit our [wiki](https://github.com/knossos-project/knossos/wiki/Annotation-Files#triangle-mesh-ply) to read about the exact file format.

Inside KNOSSOS you can jump to specific areas of interest: Just click on the mesh location and press `S`.

![mesh_screenshot](https://cloud.githubusercontent.com/assets/3898364/21959514/746d271a-dac9-11e6-9711-81d0297b18c3.png)

### Skeletonization:
-   Trees are now traversed along the edges (depth-first-search) with `X` and `Shift+X` (previously, traversal order was defined by node ID).
-   You can now enable anti-aliasing for a smoother skeleton visualization.
-   Node comments can be displayed directly inside viewports. This should improve your orientation.

#### Synapses feature:

For neural annotations you can now create synapses with a synaptic cleft as well as pre- and postsynapse:
1.  Simply perform your ordinary skeletonization.
2.  On encountering a synapse, end the current tree with `Shift+C`. The last placed node is marked as _pre-synapse_.
3.  Now annotate the _synaptic cleft_ by placing nodes along it as usual. When you are finished, press `C` to end the cleft.
4.  The next placed node will be marked as _post-synapse_.

![screenshot from 2016-12-31 16-04-10](https://cloud.githubusercontent.com/assets/3898364/21959523/ca9e3f20-dac9-11e6-8364-b733ec84edeb.png)

#### Properties feature:

Until now additional information about trees and nodes had to be crammed into the respective object’s comment section. This became unreadable and hard to parse very quickly.

So we introduced customizable and visualizable properties. Properties are either simple text or textual or numerical key value pairs.

Numerical node properties can be highlighted inside the viewports: You can choose to scale radius by property value or to map property value to color map value, or both.

### Segmentation:
- You can now perform bucket fills, with the viewport being its maximum scope (middle mouse button)
- Segmentation  operations are now possible in all magnifications instead of only in mag 1.
-  The form of the segmentation brush is now aligned with the image pixels.
-  The color of segmentation objects can be changed.

### Annotation in general
-   You can use the dropdown in the toolbar to quickly switch between different work modes, e.g.:
  -   _“Tracing Advanced“:_ More complex tracing operations such as unlinked nodes or multiple trees have been moved into this separate work mode, so that ordinary tracing has become simpler and less error-prone.
  -   _“Merge Tracing”:_ In this mode skeletonization and segmentation are performed simultaneously: Place nodes on segmentation sub-objects to automatically merge them together. The result will be a classical skeletonization in which each tree corresponds to one merged segmentation object.
  -   _“Review”:_ A segmentation proof-reading mode in which painting and merging are disabled to prevent accidental modifications. Skeletonization serves as review annotation.
-   A movement area can be defined to restrict your work to a specific location of interest. Everything outside this area is darkened to an adjustable degree and cannot be modified.
-   Spacebar hide: The viewport is often cluttered with nodes, edges, segmentation overlay and viewport tools. Hold the spacebar and everything except the raw dataset will be hidden for this duration.

### Python Plugin Manager:

KNOSSOS now has a plugin manager, allowing you to write and share plugins written in Python in an online repository.

For example you can write custom GUI elements, create annotations programmatically or utilize your own algorithms. The idea is to customize KNOSSOS and make it fit your unique workflows.

As a starting point and reference we have a set of plugins available at [https://github.com/knossos-project/knossos-plugins](https://github.com/knossos-project/knossos-plugins).

The Manager can be found under the _“Scripting”_ menu entry.

### Viewports:
- You can now take viewport snapshots of varying resolutions. The snapshots can be configured to decide what should be visible in them, e.g. only raw data, segmentation, skeletonization etc.
- Viewports can be undocked and positioned independently of the KNOSSOS window. Especially, multi-screen workplaces might find it useful to place a fullscreen viewport onto one screen.
- A scalebar in every viewport shows the dataset’s physical dimensions.
- The 3D viewport now supports wiggle stereoscopy, just hold W. Wiggle stereoscopy is a computer graphics method to make the 3-D nature of structures better recognizable.
- Inside the plane viewports KNOSSOS now shows information about what’s underneath your mouse cursor:
  - The position inside the dataset (shown in the lower left corner)
  - The segmentation object id at that position (shown below the segmentation table in the annotation widget)
- A fourth viewport with arbitrary orientation can be turned on. One of the purposes of an arbitrary view is to automatically orient itself orthogonally to the skeleton while tracing. This feature is still experimental.

### User Interface:
-   In the Annotation widget we replaced the previous skeleton view that caused performance problems for large skeletons. The new table also supports sorting.
-   Skeleton as well as segmentation tables consume less space. There is only one segmentation table, but if overlapping objects are selected, they are shown in an extra table.
-   We moved various options into preferences widget to reduce the number of floating windows in KNOSSOS.
-   When loading a dataset, you can specify a field of view for the viewports.
-   We tried to make all shortcuts visible in the user interface.
-   Comment shortcuts have moved from the F keys to the number keys.

#### Zoom:
-   The zoom interface now supports dynamic switching between magnifications.
  A value of 100% represent the highest magnification.
-   In the 3D viewport KNOSSOS now zooms in at the cursor position. To inspect
  something closer, simply let the mouse cursor hover over it while zooming.
-   In conformance with most other user interfaces, `Ctrl++`, `Ctrl+-` and
  `Ctrl+0` are now the shortcuts for zooming in, out or to reset zoom.

## Other improvements
-   KNOSSOS does not consume as much computing power anymore while running in the
  background.


# KNOSSOS 4.1.2

Release Date: 23rd April 2015

#### Noteworthy Changes
- merging trees resulted in a crash
- range delta and bias did not work properly
- more intuitive node selection
- performance improvements
- internal and visual improvements to the Heidelbrain integration

The `*.deb` package apparently doesn’t work on many systems. 
If it doesn’t, try the previous release instead or quickly install either the release version or a git snapshot of KNOSSOS [using junest](https://github.com/knossos-project/knossos/wiki/Build-Instructions#step-by-step-setting-up-a-linux-build-environment). 


# KNOSSOS 4.1.1

Release Date: 24th February 2015

### Fixed issues
- Segmentation overlay is disabled by default (less resource consumption).
- Extracting a connected component as a new tree works again.
- Jumping to next node now respects the comment filter from the annotation window.
- Lower idle cpu usage
- No broken movement after loading a new dataset while in magnification > 1
- The 3D segmentation brush is now visible.
- Dataset loading widget is more intuitive.
- File handling on Mac
- Node selection on MacBook Retina

`win64-standalone` and `win32-standalone` require an already installed python distribution. 
`win64-Setup` and `win32-Setup` contain an official python 2.7 installer. 
An [AUR package](https://aur.archlinux.org/packages/knossos/) is also available.


# KNOSSOS 4.1

## General Information

Release Date: 28th January 2015

Homepage: https://knossostool.org/
Project host: https://github.com/knossos-project/

`win64-standalone` and `win32-standalone` require an already installed python distribution. 
`win64-Setup` and `win32-Setup` contain an official python 2.7 installer. 

## Upcoming features and improvements:

- 3D visualization of segmentation with volume rendering
- Improved loader with support for several different dataset sources
- GPU-based data slicing (major perfomance increase, better quality for arbitrary views)

## Bug fixes

- Annotation time will not reduce anymore when running KNOSSOS too long.
- Resolved several crash conditions. 
- KNOSSOS now works with node IDs up to 4e9.
- Node selection difficulties fixed.
- Comment modification working properly again.

## Features

### Dataset:
- A new KNOSSOS cuber in Python is now available. It allows easy conversion from image stacks to KNOSSOS datasets. Currently supported file formats: tif, png and jpeg.
- KNOSSOS now remembers your last used datasets and displays identifying information about them.
- Datasets can now be loaded by specifying a .conf file. Specifying a dataset folder is still supported.
- Two streaming datasets of the mouse retina are now shipped with KNOSSOS.
- Changing the Supercube edge size does not require a restart anymore.

### Segmentation:

The segmentation feature allows to annotate the volume of neurons where skeletonization only reconstructs morphology and connectivity.
- Activate this feature at 'Edit Skeleton' -> 'Segmentation Mode'. The settings for it can be found at 'Windows' -> 'Annotation Window' -> 'Segmentation'.
- The Brush tool is used to perform original segmentation or to correct existing segmentations.
    - Edited cubes can be saved and loaded as part of the annotation file. 
- The Merge tool is used to connect or disconnect existing 64bit int segmentation objects. 
    - The result can be saved and loaded as a ›mergelist‹ as part of the annotation file. 
- We support segmentation data in the form of raw 64-bit matrices optionally compressed with google-snappy and zip.

### Skeletonization:

- The new Simple Tracing Mode prevents annotation mistakes through reduced functionality.
  Turned-off Features are: creating a new tree, placing unconnected nodes and creating cycles within the skeleton.
  You can deactivate Simple Tracing Mode at 'Edit Skeleton'.
- KNOSSOS now supports >5e6 nodes (loading, saving, and parallel visualization of up to 500k nodes).
- Node comments can be displayed in the orthogonal viewports by enabling 'show node comments' at 'Windows' -> 'Viewport Settings' -> 'Slice Plane Viewports'.

### Python plugin interface (experimental):

- KNOSSOS is now fully scriptable through PythonQT and additional functionality can be added with pure Python code.
- A Python plugin repository will be available on the KNOSSOS website soon.

## Changes

- Skeleton files (.nml-files) will still be supported continuously, but the new KNOSSOS annotation file (.k.zip) is now the default file type.
- The former 'Comment Settings' Window has been moved to the 'Comments' tab in the Annotation Window. You can define comment shortcuts from F1 - F10.
- The table row size within the Annotation Window has been reduced to display more rows.
- The annotation time is now displayed at the bottom right corner.
- Arbitrary viewport orientation now needs to be enabled at 'Windows' -> 'Viewport Settings' -> 'Slice Plane Viewports'.
- Both tabs for the heidelbrain integration have been merged.

# KNOSSOS 4.0.1

Release Date: 26th September 2014

### New Mac Version

We now have a Mac release!
Please note following differences between the Mac version and the other versions:
- For the actions under "Edit Skeleton" you need to hold the "fn" key additionaly. For example, to add a new tree you would press: **fn + C**.
- There are no Buttons in the Skeleton Viewport, so in order to rotate there, open the context menu with: **Cmd + right click**.
- Generally, commands that require to hold the "Ctrl" key require to hold the "Cmd" key instead, e.g. to move around a viewport perform **Cmd + Alt + left mouse click and move**.

We are still working on unifying the behaviour on all platforms. Please let us know if you find any more unexpected behaviour by opening an issue here:

https://github.com/knossos-project/knossos/issues

### Changelog

- press `5` to temporarily display the uncompressed data of compressed remote dataset. Current compression is visible in the `Zoom and Multiresolution` widget
- fixed a major performance problem on systems, where vsync was not disabled
- downloaded files from remote datasets are now deleted after loading


# KNOSSOS 4

Release Date: 4th June 2014 

KNOSSOS 4.0 is a milestone release with a completely reworked graphical user interface based on the QT framework.

Homepage: http://www.knossostool.org/
Project host: https://github.com/knossos-project/

## Roadmap:

v4.1:

- Interactive Python scripting
- Volume annotation and visualization of large scale segmentation data

## Known Bugs

## UI Improvements

- Nicer look and feel
- Native file dialogs and ui elements
- Restructured menu:
  - Icons and shortcut information in the menu bar
  - Current workmodes are marked with a check
  - A Toolbar with a collection of important widgets
  - Restructured Viewport Settings Widget
- Tooltips
- Integrated documentation
- Unsaved changes are indicated with an (*) in the titlebar

## Features

## Tracing:

- Integration of client-server based task management (with a Python django backend), that allows annotators to efficiently submit their tracings to a database.
- Extensive filtering and listing of trees and nodes (with regular expression support) based on comments and other properties.
- Visual selection and operations on groups of nodes in all viewports.
- Show only selected trees.

## Dataset:

- Real time arbitrary angle slicing for voxel data (use eg l or k for rotations).
- Datasets can now be streamed from remote server locations (http based, works well for connection speeds > 10 MBit/s with compression)
- Datasets can be compressed (each datacube individually) with JPG2000 or JPEG with real time decompression during the navigation (multi-threaded). Typical values for 3D EM datasets without obvious compression artefacts are ~1 bit / voxel. Lossless compression is also supported.
- On-the-fly dataset changes possible under File->Load Dataset, no more Python Launcher required.
- KNOSSOS remembers the last used dataset and loads it on launching.

## Skeleton:

- Multiple skeletons can now be loaded at the same time (select more than one in the file dialog).
- Load Skeletons by drag'n dropping them into KNOSSOS.

## Changes

- The Python Launcher has been removed.
- Moving Viewports around now works with Ctrl+AltLeft mouse drag
- The synchonization of multiple KNOSSOS instances is currently not supported any more
- The format for custom preferences is now saved in a key value file structure (.conf)
- Default Folder for Skeleton Files is now in the standard data location (i.e for Windows: %AppData%/MPIMF and for Linux: /home/user/.local/share/data/MPIMF)
- A Mac OS X Version (>= version 10.9) will be available soon.
- Debian Package for Linux (Tested on Ubuntu [i386/amd64](>= version 12))


# KNOSSOS V3.4.2

Release Date: 9th July 2013

- Comments tab for quick placements of node comments with shortcuts F1 - F5.
- Some bug fixes


# KNOSSOS V3.4.1

## General Information

Release Date: 8th June, 2013

Homepage: http://www.knossostool.org/ and http://code.google.com/p/knossos-skeletonizer/

## Known Issues

- Delay after launching the installer

  Windows only:
  There are reports that the installer sometimes requires several minutes to launch. Please just wait a few minutes or restart Windows in case nothing happens after double clicking it. We are investigating the problem.

## Bugfixes

- Fixed issue of Knossos not launching if a configuration file was corrupted
- Fixed incorrect incrementation/decrementation in the "Active Node ID"-widget
- Fixed crashes of synchronised Knossos instances

## New Features / Enhancements

- New shortcuts for "walking" and "jumping" through the dataset:

  - [d]/[f]: move x steps backwards/forwards in the direction of the active viewport. 
    x can be specified in the walkframes widget under "Navigation Settings"
  - [d]/[f]+[SHIFT]: move 10 steps backwards/forwards in the direction of the active viewport 
  - [e]/[r]: move walkframes*y steps backwards/forwards in the direction of the active viewport.
    y can be specified in the jumpframes widget under "Navigation Settings" 

- Added SHA-2 checksums for skeleton time and idle time

- Added widgets to the nodes tab for modifying the active node's coordinates

- Adjusted idle time tolerance.


# KNOSSOS V3.4

## General Information

Release Date: April 19, 2013

Homepage: http://www.knossostool.org/ and http://code.google.com/p/knossos-skeletonizer/

## Known Issues

- Delay after launching the installer
  
  Windows only:
  There are reports that the installer sometimes requires several minutes to launch. Please
  just wait a few minutes in case nothing happens after double clicking it. We are investigating
  the problem.

## Important new Features

- New Rendering Engine

  Skeletons are displayed in KNOSSOS V3.4 with a new rendering engine that allows the fluent handling of skeletons with several 100k nodes. It is based on a heuristic level-of-detail algorithm that downsamples the spatial skeleton graph dynamically, dependent on the current zoom level applied by the user. The new rendering engine should also make KNOSSOS much more stable on older graphics cards (esp. compared to 3.2).

- Better Rotation in Skeleton Viewport

  Rotating the 3D skeleton with the mouse is now based on a rolling-ball rotation model.

- Tracing Time

  The tracing time is now obfuscated in skeleton files.


# KNOSSOS V3.3 Beta

## General Information

Release Date: February 22, 2013

Homepage: http://www.knossostool.org/ and http://code.google.com/p/knossos-skeletonizer/

## Bugfixes

- Removed many memory leaks
- Fixed OpenGL display list problems that lead to unstable behavior on many computers
- Fixed defective recentering on node
- Fixed incorrect incrementation/decrementation in the "Active Tree ID"-widget 

## New Features / Enhancements

- Massive performance improvements and better visual quality while handling large skeletons (5k-500k nodes) through a completely reworked rendering engine (view frustum culling, dynamic level-of-detail algorithm, large vertex batches). The user has direct GUI control over the rendering quality with the new option: "Rendering quality".
- Adjustable position and size of viewports
- Conditional radii and highlighting (coloring) of nodes based on a set of definable comment strings
- Python starter now remembers the last opened folder or starts in the Knossos folder
- Improvement of Skeleton Viewport rotation, behaves now as expected.
- Added Buttons to rotate Skeleton Viewport by 90 and 180 degrees.
- Recentering on a very distant node is performed by a jump instead of a smooth move.


# KNOSSOS V3.2

## General Information

Release Date: October 11, 2012

Homepage: http://www.knossostool.org/ and http://code.google.com/p/knossos-skeletonizer/

## Important new Features

### Advanced Tracing Modes

KNOSSOS V3.2 provides three new tracing modes in which the user is automatically moved
futher along the current tracing direction when a new node is added.
The options for this new feature can be found in "Preferences" -> "Navigation Settings".

1. Additional viewport direction move mode KNOSSOS moves a specified distance orthogonal to the clicked viewport
2. Additional mirrored move mode KNOSSOS moves in the direction given by the vector between the previous node and the active node, by the same amount as the previous move
3. Additional tracing direction move mode: same as 2), but with a user-specified distance.

### Shortcuts

1. The user can move to the next or previous node via [x] / [SHIFT] + [x] and to the next or previous tree via [y] / [SHIFT] + [y].
2. The Skeleton Viewport zoom factor can be changed via the [i] and [o] keys when the mouse pointer is inside the Skeleton Viewport.
3. Nodes can be connected or disconnected via [ALT] + [left-click]. The possibility of connecting and disconnecting nodes with the mousewheel is now simplified to the key combination [SHIFT] + [middle-click].
4. The [e] and [r] keys move by a distance of 10 orthogonally to the viewport that contains the mouse pointer. The speed can be set through "Recentering Time orthog. [ms]", which can be found under "Navigation Settings".
5. The Skeleton File can be saved via [CTRL] + [s].
6. KNOSSOS can be quit via [ALT] + [F4].

### Default Preferences

1. The user has the possibility to restore the default preferences of KNOSSOS by clicking on "Preferences" -> "Default Preferences" in the menu bar.
2. The reset button in the skeleton viewport resets to the default view.

- Comment Shortcuts
  Up to 5 frequently used comments can be bound to the [F1] - [F5] keys.  The comment shortcuts window can be opened by clicking "Windows" -> "Comment Shortcuts" in the menu bar.

- Tracing Time
  This feature allows a better time management of the tracing sessions.

  1. Running time This is a continuous counter which displays how long KNOSSOS has been running. It starts when KNOSSOS starts.
  2. Tracing time This is the time which shows the working time of the user.
  3. Idle time This time shows how long the user has not been working while KNOSSOS was running.


- Multiresolution
  KNOSSOS now supports dynamic switching between datasets sampled at different resolutions. This allows zooming out much further than before to get an overview of a complete large dataset. This feature requires all datasets to reside in folders with identical names except for the trailing magnification specifier _magX, where X is an integer denoting the factor by which the data has been downsampled. KNOSSOS searches for datasets following this naming convention in the same folder as the current dataset. The feature is disabled by default and can be enabled by unchecking "Lock dataset to current mag" in "View" -> "Zoom and Multires".
