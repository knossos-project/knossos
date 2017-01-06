#ifndef GUICONSTANTS_H
#define GUICONSTANTS_H

/* Constants for Widget-identification */
const QString MAIN_WINDOW = "main_window";
const QString COMMENTS_TAB = "comments_tab";
const QString DATASET_WIDGET = "dataset_widget";
const QString DATA_SAVING_WIDGET = "data_saving_widget";
const QString ZOOM_AND_MULTIRES_WIDGET = "zoom_and_multires_widget";
const QString VIEWPORT_SETTINGS_WIDGET = "viewport_settings_widget";
const QString NAVIGATION_WIDGET = "navigation_widget";
const QString TOOLS_WIDGET = "tools_widget";
const QString TRACING_TIME_WIDGET = "tracing_time_widget";
const QString PYTHON_PROPERTY_WIDGET = "pythonpropertywidget";
const QString HEIDELBRAIN_INTEGRATION = "heidelbrain_integration";

/* General attributes appropriate for most widgets */
const QString WIDTH  = "width";
const QString HEIGHT = "height";
const QString POS_X  = "x";
const QString POS_Y  = "y";
const QString VISIBLE = "visible";

/* Autosave */
const QString AUTO_SAVING = "auto_saving";
const QString SAVING_INTERVAL = "saving_interval";
const QString AUTOINC_FILENAME = "autoinc_filename";

/* DataSet Switch */
const QString DATASET_MRU = "dataset_mru";
const QString DATASET_SUPERCUBE_EDGE = "supercube_edge";
const QString DATASET_OVERLAY = "overlay";
const QString DATASET_LAST_USED = "dataset_last_used";

/* Zoom and Multires */
const QString ORTHO_DATA_VIEWPORTS = "orthogonal_data_viewports";
const QString SKELETON_VIEW = "skeleton_view";
const QString LOCK_DATASET_TO_CURRENT_MAG = "lock_dataset_to_currentmag";

/* Viewport Settings General Tab */
const QString LIGHT_EFFECTS = "light_effects";
const QString HIGHLIGHT_ACTIVE_TREE = "hightlight_active_tree";
const QString SHOW_ALL_NODE_ID = "show_all_node_id";
const QString RENDERING_QUALITY = "rendering_quality";
const QString WHOLE_SKELETON = "whole_skeleton";
const QString ONLY_SELECTED_TREES = "only_selected_trees";
const QString HIDE_SKELETON_ORTHOVPS = "hide_skeleton_orthovps";
const QString HIDE_SKELETON_SKELVP = "hide_skeleton_skelvp";
const QString OVERRIDE_NODES_RADIUS_CHECKED = "override_nodes_radius_checked";
const QString OVERRIDE_NODES_RADIUS_VALUE = "override_nodes_radius_value";
const QString EDGE_TO_NODE_RADIUS = "edge_to_node_radius";
const QString SHOW_VP_DECORATION = "show_vp_decoration";
const QString VP_DEFAULT_POS_SIZE = "vp_default_pos_size";
const QString VPXY_COORD = "vpxy_coord";
const QString VPXY_SIZE = "vpxy_size";
const QString VPXZ_COORD = "vpxz_coord";
const QString VPXZ_SIZE = "vpxz_size";
const QString VPYZ_COORD = "vpyz_coord";
const QString VPYZ_SIZE = "vpyz_size";
const QString VPSKEL_COORD = "vpskel_coord";
const QString VPSKEL_SIZE = "vpskel_size";
const QString VP_TAB_INDEX = "vp_tab_index";
const QString VP_LOCK_ORIENTATION = "lock_vp_orientation";

/* Viewport Settings Slice Plane Tab */
const QString HIGHLIGHT_INTERSECTIONS = "highlight_intersections";
const QString DATASET_LINEAR_FILTERING = "dataset_linear_filtering";
const QString DEPTH_CUTOFF = "depth_cutoff";
const QString SHOW_NODE_COMMENTS = "show_node_comments";
const QString BIAS = "bias";
const QString RANGE_DELTA = "range_delta";
const QString SEGMENTATION_OVERLAY_ALPHA = "segmentation_overlay_alpha";
const QString SEGMENTATION_BORDER_ALPHA = "segmentation_border_alpha";
const QString DRAW_INTERSECTIONS_CROSSHAIRS = "draw_intersections_crosshairs";
const QString SHOW_VIEWPORT_SIZE = "draw_viewport_size";
const QString TREE_LUT_FILE = "tree_lut_file";
const QString TREE_LUT_FILE_USED = "tree_lut_file_used";
const QString DATASET_LUT_FILE = "dataset_lut_file";
const QString DATASET_LUT_FILE_USED = "dataset_lut_file_used";

/* Viewport Settings Skeleton Viewport Tab */
const QString SHOW_XY_PLANE = "show_xy_plane";
const QString SHOW_XZ_PLANE = "show_xz_plane";
const QString SHOW_YZ_PLANE = "show_yz_plane";
const QString ROTATE_AROUND_ACTIVE_NODE = "rotate_around_active_node";

/* Navigation Widget */
const QString MOVEMENT_SPEED = "movement_speed";
const QString JUMP_FRAMES = "jump_frames";
const QString WALK_FRAMES = "walk_frames";
const QString RECENTERING_TIME_PARALLEL = "recentering_time_parellel";
const QString RECENTERING_TIME_ORTHO = "recentering_time_ortho";
const QString NORMAL_MODE = "normal_mode";
const QString ADDITIONAL_VIEWPORT_DIRECTION_MOVE = "additional_viewport_direction_move";
const QString ADDITIONAL_TRACING_DIRECTION_MOVE = "additional_tracing_direction_move";
const QString ADDITIONAL_MIRRORED_MOVE = "additional_mirrored_move";
const QString DELAY_TIME_PER_STEP = "delay_time_per_step";
const QString NUMBER_OF_STEPS = "number_of_steps";

/* Tools Widget QuickTab */
const QString ACTIVE_TREE_ID = "active_tree_id";
const QString ACTIVE_NODE_ID = "active_node_id";
const QString COMMENT = "comment";
const QString SEARCH_FOR = "search_for";
const QString SEARCH_FOR_TREE = "search_for_tree";
const QString SEARCH_FOR_NODE = "search_for_node";
const QString TOOLS_TAB_INDEX = "tools_tab_index";

/* Tools Widget TreeTab */
const QString ID1 = "id1";
const QString ID2 = "id2";
const QString TREE_COMMENT = "tree_comment";
const QString R = "r";
const QString G = "g";
const QString B = "b";
const QString A = "a";

/* Tools Widget NodeTab */
const QString USE_LAST_RADIUS_AS_DEFAULT = "use_last_radius_as_default";
const QString ACTIVE_NODE_RADIUS = "active_node_radius";
const QString DEFAULT_NODE_RADIUS = "default_node_radius";
const QString LOCKING_RADIUS = "locking_radius";
const QString ENABLE_COMMENT_LOCKING = "enable_comment_locking";
const QString LOCK_TO_NODES_WITH_COMMENT = "lock_to_nodes_with_comment";

/* Comments Highlighting */
const QString CUSTOM_COMMENT_COLOR = "use_custom_comment_color";
const QString CUSTOM_COMMENT_NODERADIUS = "use_custom_comment_node_radius";
const QString CUSTOM_COMMENT_APPEND = "append_comment";

/* Tracing Time Widget */
const QString RUNNING_TIME = "running_time";

/* MainWindow recent files */

const QString LOADED_FILE1 = "loaded_file1";
const QString LOADED_FILE2 = "loaded_file2";
const QString LOADED_FILE3 = "loaded_file3";
const QString LOADED_FILE4 = "loaded_file4";
const QString LOADED_FILE5 = "loaded_file5";
const QString LOADED_FILE6 = "loaded_file6";
const QString LOADED_FILE7 = "loaded_file7";
const QString LOADED_FILE8 = "loaded_file8";
const QString LOADED_FILE9 = "loaded_file9";
const QString LOADED_FILE10 = "loaded_file10";

const QString OPEN_FILE_DIALOG_DIRECTORY = "open_file_dialog_directory";
const QString SAVE_FILE_DIALOG_DIRECTORY = "save_file_dialog_directory";
const QString ANNOTATION_MODE = "annotation_mode";
const QString TRACING_MODE = "tracing_mode";
const QString SIMPLE_TRACING = "simple_tracing";

/* Python Property Widget */
const QString PYTHON_INTERPRETER = "python_interpreter";
const QString PYTHON_AUTOSTART_FOLDER = "python_autostart_folder";
const QString PYTHON_AUTOSTART_TERMINAL = "python_autostart_terminal";
const QString PYTHON_WORKING_DIRECTORY = "python_working_directory";

// Heidelbrain Integration
const QString HEIDELBRAIN_HOST = "heidelbrain_host";
const QString HEIDELBRAIN_COOKIES = "heidelbrain_cookies";

#endif // GUICONSTANTS_H
