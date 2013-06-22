/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */


void setGUIcolors();
/* general callbacks */
void OkfileDlgOpenSkel(AG_Event *event);
void OkfileDlgSaveAsSkel(AG_Event *event);
void OkfileDlgLoadDatasetLUT(AG_Event *event);
void OkfileDlgLoadTreeLUT(AG_Event *event);
void OkfileDlgOpenPrefs(AG_Event *event);
void OkfileDlgSavePrefsAs(AG_Event *event);
static void CancelFileDlg(AG_Event *event);


static void UI_checkQuitKnossos();
static void drawXyViewport(AG_Event *event);
static void drawXzViewport(AG_Event *event);
static void drawYzViewport(AG_Event *event);
static void drawSkelViewport(AG_Event *event);
static void resizeCallback(uint32_t newWinLenX, uint32_t newWinLenY);
static void resizeWindows();
static void agInputWdgtGainedFocus(AG_Event *event);
static void agInputWdgtLostFocus(AG_Event *event);

// enable/disable comment coloring and comment node radii
static void commentColorWdgtSwitched();
static void commentNodeRadiusWdgtSwitched();

// color/radius chosen for a comment node
static void commentColorSelected(AG_Event *event);
static void commentColorRemoved(AG_Event *event);

static void currPosWdgtModified(AG_Event *event);
static void currNodePosWdgtModified(AG_Event *event);
static void actNodeIDWdgtModified(AG_Event *event);
static void actTreeIDWdgtModified(AG_Event *event);
static void actTreeColorWdgtModified(AG_Event *event);
static void actNodeCommentWdgtModified(AG_Event *event);
static void actTreeCommentWdgtModified(AG_Event *event);
static void agFilterTextboxModified(AG_Event *event);
static void UI_filterBranchNodesOnlyModified(AG_Event *event);

/* menu callbacks */
static void fileOpenSkelFile(AG_Event *event);
static void fileSaveAsSkelFile(AG_Event *event);

//static void viewDataSetStats(AG_Event *event);
static void viewZooming(AG_Event *event);
static void viewTracingTime();
static void viewComments();
//static void viewLoadImgJTable(AG_Event *event);

static void prefNavOptions(AG_Event *event);
static void prefLoadCustomPrefs(AG_Event *event);
static void prefSaveCustomPrefsAs(AG_Event *event);
static void prefSyncOptions(AG_Event *event);
//static void prefRenderingQualityOptions(AG_Event *event);
//static void prefVolTracingOptions(AG_Event *event);
//static void prefSpatialLockingOptions(AG_Event *event);
//static void prefAGoptions(AG_Event *event);
static void prefViewportPrefs();
static void prefSaveOptions();

//static void winShowNavigator(AG_Event *event);
static void winShowTools(AG_Event *event);
static void winShowConsole(AG_Event *event);

static void yesNoPromptHitYes();
static void yesNoPromptHitNo();

/* functions generating gui elements */


static void createVpXyWin();
static void createVpSkelWin();
static void createVpYzWin();
static void createVpXzWin();

static void createOpenFileDlgWin();
static void createSaveAsFileDlgWin();

 void createCurrPosWdgt(AG_Window *parent);
 void createSkeletonVpToolsWdgt(AG_Window *parent);
 void createActNodeWdgt(AG_Widget *parent);
static void datasetColorAdjustmentsChanged();

static void createOpenCustomPrefsDlgWin();
static void createSaveCustomPrefsAsDlgWin();

/*
 *  Wrapper functions around KNOSSOS internals for use by the UI
 * (GUI / Keyboard / (Mouse))
 *
 */

static void WRAP_loadSkeleton();
static void WRAP_saveSkeleton();

static void UI_clearSkeleton();
static void WRAP_clearSkeleton();
static void UI_setViewModeDrag();
static void UI_setViewModeRecenter();
static void UI_unimplemented();
static void UI_lockActiveNodeBtnPressed();
static void UI_disableLockingBtnPressed();
static void UI_commentLockWdgtModified(AG_Event *event);
static void UI_setDefaultZoom();
static void UI_SyncConnect();
static void UI_SyncDisconnect();
static void UI_findNextBtnPressed();
static void UI_findPrevBtnPressed();
static void UI_deleteNodeBtnPressed();
static void UI_jumpToNodeBtnPressed(AG_Event *event);
static void UI_jumpToActiveNodeBtnPressed();
static void UI_linkActiveNodeWithBtnPressed();
static void UI_actNodeRadiusWdgtModified();
static void UI_deleteTreeBtnPressed();
static void UI_newTreeBtnPressed();
static void UI_splitTreeBtnPressed();
static void UI_mergeTreesBtnPressed();
static void UI_newTreeBtnPressed();
static void UI_helpDeleteTree();
static void UI_renderModelRadioModified();
static void UI_displayModeRadioModified();
static void UI_setHighlightActiveTree();
static void UI_setShowVPLabels();
static void UI_setShowNodeIDs();
static void UI_skeletonChanged();
static void UI_enableSliceVPOverlayModified();
static void UI_pushBranchBtnPressed();
static void UI_popBranchBtnPressed();
static void UI_enableLinearFilteringModified();
static void UI_helpShowAbout();
static void UI_loadSettings();
static void UI_setSkeletonPerspective(AG_Event *event);
static void UI_orthoVPzoomSliderModified();
static void UI_lockCurrentMagModified(AG_Event *event);
static void UI_deleteCommentBoxesBtnPressed();
static void UI_deleteCommentBoxes();
static void UI_showPosSizButtons();

static Coordinate *parseRawCoordinateString(char *string);
static void prefDefaultPrefsWindow();
static void prefDefaultPrefs();
static void resetVpPosSize();
static void createVpPosSizWin(int i);
static void setDataSizeWinPositions();
static void focusViewport(int VPfound);
