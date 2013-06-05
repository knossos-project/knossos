#ifndef GUICONSTANTS_H
#define GUICONSTANTS_H

/* Constants for Widget-identification */
const QString MAIN_WINDOW = "main_window";
const QString COMMENTS_WIDGET = "comments_widget";
const QString CONSOLE_WIDGET = "console_widget";
const QString DATA_SAVING_WIDGET = "data_saving_widget";
const QString ZOOM_AND_MULTIRES_WIDGET = "zoom_and_multires_widget";
const QString VIEWPORT_SETTINGS_WIDGET = "viewport_settings_widget";
const QString NAVIGATION_WIDGET = "navigation_widget";
const QString TOOLS_WIDGET = "tools_widget";

/* General attributes appropriate for most widgets */
const QString WIDTH  = "width";
const QString HEIGHT = "height";
const QString POS_X  = "x";
const QString POS_Y  = "y";
const QString VISIBLE = "visible";

/* Comments shortcuts */
const QString COMMENT1 = "comment1";
const QString COMMENT2 = "comment2";
const QString COMMENT3 = "comment3";
const QString COMMENT4 = "comment4";
const QString COMMENT5 = "comment5";

/* Autosave */
const QString AUTO_SAVING = "auto_saving";
const QString SAVING_INTERVAL = "saving_interval";
const QString AUTOINC_FILENAME = "autoinc_filename";

/* Zoom and Multires */
const QString ORTHO_DATA_VIEWPORTS = "orthogonal_data_viewports";
const QString SKELETON_VIEW = "skeleton_view";
const QString LOCK_DATASET_TO_CURRENT_MAG = "lock_dataset_to_currentmag";

/* Viewport Settings General Tab */
const QString LIGHT_EFFECTS = "light_effects";
const QString HIGHLIGHT_ACTIVE_TREE = "hightlight_active_tree";
const QString SHOW_ALL_NODE_ID = "show_all_node_id";

/* Viewport Settings Slice Plane Tab */
const QString ENABLE_OVERLAY = "enable_overlay";
const QString HIGHLIGHT_INTERSECTIONS = "highlight_intersections";
const QString DATASET_LINEAR_FILTERING = "dataset_linear_filtering";
const QString DEPTH_CUTOFF = "depth_cutoff";
const QString BIAS = "bias";
const QString RANGE_DELTA = "range_delta";
const QString ENABLE_COLOR_OVERLAY = "enable_color_overlay";
const QString DRAW_INTERSECTIONS_CROSSHAIRS = "draw_intersections_crosshairs";
const QString SHOW_VIEWPORT_SIZE = "draw_viewport_size";

/* Viewport Settings Skeleton Viewport Tab */
const QString SHOW_XY_PLANE = "show_xy_plane";
const QString SHOW_XZ_PLANE = "show_xz_plane";
const QString SHOW_YZ_PLANE = "show_yz_plane";
const QString ROTATE_AROUND_ACTIVE_NODE = "rotate_around_active_node";
const QString WHOLE_SKELETON = "whole_skeleton";
const QString ONLY_ACTIVE_TREE = "only_active_tree";
const QString HIDE_SKELETON = "hide_skeleton";

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



#endif // GUICONSTANTS_H
