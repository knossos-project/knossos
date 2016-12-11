/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef GUICONSTANTS_H
#define GUICONSTANTS_H

#include <vector>
#include <QString>

// Constants for Widget-identification
const QString MAIN_WINDOW = "main_window";
const QString VIEWER = "viewer";
const QString COMMENTS_TAB = "comments_tab";
const QString DATASET_WIDGET = "dataset_widget";
const QString DATASET_OPTIONS_WIDGET = "zoom_and_multires_widget";
const QString PREFERENCES_WIDGET = "preferences_widget";
const QString SNAPSHOT_WIDGET = "snapshot_widget";
const QString ANNOTATION_WIDGET = "tools_widget";
const QString PYTHON_PROPERTY_WIDGET = "pythonpropertywidget";
const QString HEIDELBRAIN_INTEGRATION = "heidelbrain_integration";

// General attributes appropriate for most widgets
const QString GEOMETRY = "geometry";
const QString STATE = "state";
const QString WIDTH  = "width";
const QString HEIGHT = "height";
const QString POS_X  = "x";
const QString POS_Y  = "y";
const QString VISIBLE = "visible";

// Autosave
const QString AUTO_SAVING = "auto_saving";
const QString SAVING_INTERVAL = "saving_interval";
const QString AUTOINC_FILENAME = "autoinc_filename";

// DataSet Switch
const QString DATASET_GEOMETRY = "dataset_geometry";
const QString DATASET_MRU = "dataset_mru";
const QString DATASET_CUBE_EDGE = "cube_edge";
const QString DATASET_SUPERCUBE_EDGE = "supercube_edge";
const QString DATASET_OVERLAY = "overlay";
const QString DATASET_LAST_USED = "dataset_last_used";

// Zoom and Multires
const QString SKELETON_VIEW = "skeleton_view";
const QString LOCK_DATASET_TO_CURRENT_MAG = "lock_dataset_to_currentmag";

// Preferences Tree Tab
const QString HIGHLIGHT_ACTIVE_TREE = "hightlight_active_tree";
const QString HIGHLIGHT_INTERSECTIONS = "highlight_intersections";
const QString LIGHT_EFFECTS = "light_effects";
const QString MSAA_SAMPLES = "samples";
const QString TREE_LUT_FILE = "tree_lut_file";
const QString TREE_LUT_FILE_USED = "tree_lut_file_used";
const QString DEPTH_CUTOFF = "depth_cutoff";
const QString RENDERING_QUALITY = "rendering_quality";
const QString WHOLE_SKELETON = "whole_skeleton";
const QString ONLY_SELECTED_TREES = "only_selected_trees";
const QString SHOW_SKELETON_ORTHOVPS = "show_skeleton_orthovps";
const QString SHOW_SKELETON_SKELVP = "Show_skeleton_skelvp";

// Preferences Nodes Tab
const QString EDGE_TO_NODE_RADIUS = "edge_to_node_radius";
const QString NODE_PROPERTY_LUT_PATH = "node_property_lut_path";
const QString NODE_PROPERTY_MAP_MIN = "node_property_map_min";
const QString NODE_PROPERTY_MAP_MAX = "node_property_map_max";
const QString NODE_PROPERTY_RADIUS_SCALE = "node_property_radius_scale";
const QString OVERRIDE_NODES_RADIUS_CHECKED = "override_nodes_radius_checked";
const QString OVERRIDE_NODES_RADIUS_VALUE = "override_nodes_radius_value";
const QString NODE_ID_DISPLAY = "node_id_display";
const QString SHOW_NODE_COMMENTS = "show_node_comments";

// Preferences Dataset & Segmentation Tab
const QString DATASET_LINEAR_FILTERING = "dataset_linear_filtering";
const QString DATASET_LUT_FILE = "dataset_lut_file";
const QString DATASET_LUT_FILE_USED = "dataset_lut_file_used";
const QString BIAS = "bias";
const QString RANGE_DELTA = "range_delta";
const QString SEGMENTATION_OVERLAY_ALPHA = "segmentation_overlay_alpha";

// Preferences Viewports Tab
const QString SHOW_SCALEBAR = "show_scalebar";
const QString SHOW_VP_DECORATION = "show_vp_decoration";
const QString DRAW_INTERSECTIONS_CROSSHAIRS = "draw_intersections_crosshairs";
const QString ADD_ARB_VP = "add_arb_vp";
const QString SHOW_XY_PLANE = "show_xy_plane";
const QString SHOW_XZ_PLANE = "show_xz_plane";
const QString SHOW_ZY_PLANE = "show_zy_plane";
const QString SHOW_ARB_PLANE = "show_arb_plane";
const QString SHOW_PHYSICAL_BOUNDARIES = "show_physical_boundaries";
const QString ROTATION_CENTER = "rotation_center2";// rotation_center was used with different values in 4.1.2 ini
const QString RENDER_VOLUME = "render_volume";
const QString VOLUME_ALPHA = "volume_alpha";
const QString VOLUME_BACKGROUND_COLOR = "volume_background_color";
const QString VP_TAB_INDEX = "vp_tab_index";

// Mainwindow
const QString GUI_MODE = "gui_mode";
const QString VP_ORDER = "vp_order";
const QString VP_I_DOCKED = "vp%1_docked";
const QString VP_I_DOCKED_POS = "vp%1_pos";
const QString VP_I_DOCKED_SIZE = "vp%1_size";
const QString VP_I_FLOAT_POS = "vp%1_floatpos";
const QString VP_I_FLOAT_SIZE = "vp%1_floatsize";
const QString VP_I_FLOAT_FULLSCREEN = "vp%1_float_fullscreen";
const QString VP_I_FLOAT_FULL_ORIG_DOCKED = "vp%1_float_fullscreen_originally_docked";
const QString VP_I_VISIBLE = "vp%1_visible";
const QString VP_DEFAULT_POS_SIZE = "vp_default_pos_size";

// Navigation Widget
const QString OUTSIDE_AREA_VISIBILITY = "outside_area_visibility";
const QString MOVEMENT_SPEED = "movement_speed";
const QString JUMP_FRAMES = "jump_frames";
const QString WALK_FRAMES = "walk_frames";
const QString RECENTERING = "recentering";
const QString NUMBER_OF_STEPS = "number_of_steps";

// Tools Widget QuickTab
const QString SEARCH_FOR_TREE = "search_for_tree";
const QString SEARCH_FOR_NODE = "search_for_node";

// Comments Highlighting
const QString CUSTOM_COMMENT_NODECOLOR = "use_custom_comment_node_color";
const QString CUSTOM_COMMENT_NODERADIUS = "use_custom_comment_node_radius";
const QString CUSTOM_COMMENT_APPEND = "append_comment";

const QString OPEN_FILE_DIALOG_DIRECTORY = "open_file_dialog_directory";
const QString SAVE_FILE_DIALOG_DIRECTORY = "save_file_dialog_directory";
const QString ANNOTATION_MODE = "annotation_mode";
const QString SEGMENT_STATE = "segment_state";

// Python Terminal Widget
const QString PYTHON_TERMINAL_WIDGET = "python_terminal_widget";

// Python Property Widget
const QString PYTHON_AUTOSTART_FOLDER = "python_autostart_folder";
const QString PYTHON_WORKING_DIRECTORY = "python_working_directory";
const QString PYTHON_CUSTOM_PATHS = "python_custom_paths";

// Heidelbrain Integration
const QString HEIDELBRAIN_HOST = "heidelbrain_host";
const QString HEIDELBRAIN_COOKIES = "heidelbrain_cookies";

// Snapshot settings
const QString VIEWPORT = "viewport";
const QString WITH_AXES = "with_axes";
const QString WITH_BOX = "with_box";
const QString WITH_OVERLAY = "with_overlay";
const QString WITH_SKELETON = "with_skeleton";
const QString WITH_SCALE = "with_scale";
const QString WITH_VP_PLANES = "with_vp_planes";
const QString SAVE_DIR = "save_dir";

#endif // GUICONSTANTS_H
