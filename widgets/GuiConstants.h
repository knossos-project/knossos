/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include <vector>
#include <QString>

// Constants for Widget-identification
const QString ANNOTATION_WIDGET = "tools_widget";
const QString COMMENTS_TAB = "comments_tab";
const QString DATASET_WIDGET = "dataset_widget";
const QString HEIDELBRAIN_INTEGRATION = "heidelbrain_integration";
const QString LAYER_DIALOG_WIDGET = "layer_dialog_widget";
const QString MAIN_WINDOW = "main_window";
const QString PREFERENCES_WIDGET = "preferences_widget";
const QString PYTHON_PROPERTY_WIDGET = "pythonpropertywidget";
const QString SNAPSHOT_WIDGET = "snapshot_widget";
const QString VIEWER = "viewer";
const QString ZOOM_WIDGET = "zoom_and_multires_widget";

// General attributes appropriate for most widgets
const QString GEOMETRY = "geometry";
const QString HEIGHT = "height";
const QString WIDTH  = "width";
const QString POS_X  = "x";
const QString POS_Y  = "y";
const QString STATE = "state";
const QString VISIBLE = "visible";

// Autosave
const QString AUTOINC_FILENAME = "autoinc_filename";
const QString AUTO_SAVING = "auto_saving";
const QString PLY_SAVE_AS_BIN = "ply_save_as_bin";
const QString SAVE_ANNOTATION_TIME = "save_annotation_time";
const QString SAVE_DATASET_PATH = "save_dataset_path";
const QString SAVING_INTERVAL = "saving_interval";

// DataSet Switch
const QString DATASET_CUBE_EDGE = "cube_edge";
const QString DATASET_GEOMETRY = "dataset_geometry";
const QString DATASET_LAST_USED = "dataset_last_used";
const QString DATASET_MRU = "dataset_mru";
const QString DATASET_OVERLAY = "overlay";
const QString DATASET_SUPERCUBE_EDGE = "supercube_edge";

// Zoom and Multires
const QString LOCK_DATASET_TO_CURRENT_MAG = "lock_dataset_to_currentmag";
const QString SKELETON_VIEW = "skeleton_view";

// Preferences Tree Tab
const QString DEPTH_CUTOFF = "depth_cutoff";
const QString HIGHLIGHT_ACTIVE_TREE = "hightlight_active_tree";
const QString HIGHLIGHT_INTERSECTIONS = "highlight_intersections";
const QString LIGHT_EFFECTS = "light_effects";
const QString MSAA_SAMPLES = "samples";
const QString ONLY_SELECTED_TREES_3DVP = "only_selected_trees_3dvp";
const QString ONLY_SELECTED_TREES_ORTHOVPS = "only_selected_trees_orthovps";
const QString RENDERING_QUALITY = "rendering_quality";
const QString SHOW_SKELETON_3DVP = "show_skeleton_3dvp";
const QString SHOW_SKELETON_ORTHOVPS = "show_skeleton_orthovps";
const QString TREE_LUT_FILE = "tree_lut_file";
const QString TREE_LUT_FILE_USED = "tree_lut_file_used";
const QString TREE_VISIBILITY_3DVP = "tree_visibility_3dvp";
const QString TREE_VISIBILITY_ORTHOVPS = "tree_visibility_orthovps";

// Preferensces Meshes Tab
const QString SHOW_MESH_ORTHOVPS = "show_mesh_orthovps";
const QString SHOW_MESH_3DVP = "show_mesh_3dvp";
const QString WARN_DISABLED_MESH_PICKING = "warn_disabled_mesh_picking_on_startup";
const QString MESH_ALPHA_3D = "mesh_alpha_3d";
const QString MESH_ALPHA_SLICING = "mesh_alpha_slicing";

// Preferences Nodes Tab
const QString EDGE_TO_NODE_RADIUS = "edge_to_node_radius";
const QString NODE_ID_DISPLAY = "node_id_display";
const QString NODE_PROPERTY_LUT_PATH = "node_property_lut_path";
const QString NODE_PROPERTY_RADIUS_SCALE = "node_property_radius_scale";
const QString OVERRIDE_NODES_RADIUS_CHECKED = "override_nodes_radius_checked";
const QString OVERRIDE_NODES_RADIUS_VALUE = "override_nodes_radius_value";
const QString SHOW_NODE_COMMENTS = "show_node_comments";

// Preferences Dataset & Segmentation Tab
const QString BIAS = "bias";
const QString DATASET_LINEAR_FILTERING = "dataset_linear_filtering";
const QString DATASET_LUT_FILE = "dataset_lut_file";
const QString DATASET_LUT_FILE_USED = "dataset_lut_file_used";
const QString RANGE_DELTA = "range_delta";
const QString SEGMENTATION_OVERLAY_ALPHA = "segmentation_overlay_alpha";
const QString SEGMENTATITION_HIGHLIGHT_BORDER = "segmentation_border_highlighting";

// Preferences Viewports Tab
const QString ADD_ARB_VP = "add_arb_vp";
const QString DRAW_INTERSECTIONS_CROSSHAIRS = "draw_intersections_crosshairs";
const QString RENDER_VOLUME = "render_volume";
const QString ROTATION_CENTER = "rotation_center2";// rotation_center was used with different values in 4.1.2 ini
const QString SHOW_ARB_PLANE = "show_arb_plane";
const QString SHOW_PHYSICAL_BOUNDARIES = "show_physical_boundaries";
const QString SHOW_VP_PLANES = "show_vp_planes";
const QString SHOW_SCALEBAR = "show_scalebar";
const QString SHOW_VP_DECORATION = "show_vp_decoration";
const QString SHOW_XY_PLANE = "show_xy_plane";
const QString SHOW_XZ_PLANE = "show_xz_plane";
const QString SHOW_ZY_PLANE = "show_zy_plane";
const QString VOLUME_ALPHA = "volume_alpha";
const QString VOLUME_BACKGROUND_COLOR = "volume_background_color";
const QString VP_TAB_INDEX = "vp_tab_index";

// Mainwindow
const QString GUI_MODE = "gui_mode";
const QString VP_DEFAULT_POS_SIZE = "vp_default_pos_size";
const QString VP_I_DOCKED = "vp%1_docked";
const QString VP_I_DOCKED_POS = "vp%1_pos";
const QString VP_I_DOCKED_SIZE = "vp%1_size";
const QString VP_I_FLOAT_FULL_ORIG_DOCKED = "vp%1_float_fullscreen_originally_docked";
const QString VP_I_FLOAT_FULLSCREEN = "vp%1_float_fullscreen";
const QString VP_I_FLOAT_POS = "vp%1_floatpos";
const QString VP_I_FLOAT_SIZE = "vp%1_floatsize";
const QString VP_I_VISIBLE = "vp%1_visible";
const QString VP_ORDER = "vp_order";

// Navigation Widget
const QString LOCKED_BUTTON = "locked_button";
const QString JUMP_FRAMES = "jump_frames";
const QString MOVEMENT_SPEED = "movement_speed";
const QString NUMBER_OF_STEPS = "number_of_steps";
const QString RECENTERING = "recentering";
const QString SHOW_CUBE_COORDS = "show_cube_coordinates";
const QString OUTSIDE_AREA_VISIBILITY = "outside_area_visibility";
const QString WALK_FRAMES = "walk_frames";

// Tools Widget QuickTab
const QString SEARCH_FOR_NODE = "search_for_node";
const QString SEARCH_FOR_TREE = "search_for_tree";

// Comments Highlighting
const QString CUSTOM_COMMENT_APPEND = "append_comment";
const QString CUSTOM_COMMENT_NODECOLOR = "use_custom_comment_node_color";
const QString CUSTOM_COMMENT_NODERADIUS = "use_custom_comment_node_radius";

const QString ANNOTATION_MODE = "annotation_mode";
const QString OPEN_FILE_DIALOG_DIRECTORY = "open_file_dialog_directory";
const QString SAVE_FILE_DIALOG_DIRECTORY = "save_file_dialog_directory";
const QString SEGMENT_STATE = "segment_state";

// Python Terminal Widget
const QString PYTHON_TERMINAL_WIDGET = "python_terminal_widget";

// Python Property Widget
const QString PYTHON_PLUGIN_PATH = "plugin_path";
const QString PYTHON_PLUGIN_VERSION = "plugin_version";
const QString PYTHON_CUSTOM_PATHS = "python_custom_paths";
const QString PYTHON_PLUGIN_FOLDER = "python_plugin_folder";
const QString PYTHON_REGISTERED_PLUGINS = "python_registered_plugins";
const QString PYTHON_WORKING_DIRECTORY = "python_working_directory";

// Heidelbrain Integration
const QString HEIDELBRAIN_COOKIES = "heidelbrain_cookies";
const QString HEIDELBRAIN_HOST = "heidelbrain_host_v2";

// Snapshot settings
const QString SAVE_DIR = "save_dir";
const QString VIEWPORT = "viewport";
const QString WITH_AXES = "with_axes";
const QString WITH_BOX = "with_box";
const QString WITH_MESHES = "with_meshes";
const QString WITH_OVERLAY = "with_overlay";
const QString WITH_SCALE = "with_scale";
const QString WITH_SKELETON = "with_skeleton";
const QString WITH_VP_PLANES = "with_vp_planes";
