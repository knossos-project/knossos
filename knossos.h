/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2012
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

//static uint32_t isPathString(char *string);
//static uint32_t printUsage();
static int32_t initStates();
static int32_t printConfigValues(); // YES
static uint32_t cleanUpMain(); // YES
static int32_t tempConfigDefaults(); // YES
static struct stateInfo *emptyState(); // YES
static int32_t readDataConfAndLocalConf(); // YES
static int32_t stripNewlines(char *string); // YES
static int32_t configFromCli(int argCount, char *arguments[]); // YES
static int32_t loadNeutralDatasetLUT(GLuint *lut); // YES

int32_t readConfigFile(char *path); // YES
static int32_t findAndRegisterAvailableDatasets(); // YES
#ifdef LINUX
static int32_t catchSegfault(int signum);
#endif


/*
2. Funktion zum durch-switchen von nodes ohne comment, auch branchpoints ohne comment sollen dabei gefunden werden (fuer offizielle Version nicht noetig)
1. Menu schlieﬂen bei klick irgendwo rein
2. cursor wechsel resize
3. allg. cursor Fadenkreuz

*/
