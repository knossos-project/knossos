#
#   This file is a part of KNOSSOS.
# 
#   (C) Copyright 2007-2011
#   Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
# 
#   KNOSSOS is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2 of
#   the License as published by the Free Software Foundation.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
#
#
#   For further information, visit http://www.knossostool.org or contact
#       Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
#       Fabian.Svara@mpimf-heidelberg.mpg.de
#

#!/usr/bin/env python

from runknossos_ui import RunKnossosUI
from kiki import KikiServer
from kconfig import DataError
try:
    import tkinter, tkinter.messagebox, tkinter.filedialog
    filedialog = tkinter.filedialog
    interface = tkinter
    messageBox = tkinter.messagebox
except ImportError:
    import Tkinter, tkMessageBox, tkFileDialog
    filedialog = tkFileDialog
    interface = Tkinter
    messageBox = tkMessageBox
import subprocess, os, sys

class RunKnossos(RunKnossosUI):        
    def __init__(self, root):
        RunKnossosUI.__init__(self, root)

        # Add cached datasets to available datasets
        try:
            kukupath = self.configFilePath()
            kukuConf = open(kukupath, "r") 
            kukuConfString = kukuConf.read()
            kukuConf.close()
            
            for currentPath in kukuConfString.split('\n'):
                if currentPath:
                    currentPath = os.path.abspath(currentPath)
                    if os.path.isdir(currentPath):
                        self.addDataset(currentPath)
        except IOError:
            pass

        self.updateList()

    # Add button
    #
    def add_button_command(self, *args):
        path = filedialog.askdirectory()
        if not path:
            return

        self.addDataset(os.path.abspath(path))

    def del_button_command(self, *args):
        try:
            items = map(int, self.dataset_list.curselection())
        except ValueError:
            pass

        for currentDataset in items:
            self.delDataset(self.dataset_list.get(currentDataset))
        self.updateList()

    # Run button
    #
    def run_button_command(self, *args):
        try:
            items = map(int, self.dataset_list.curselection())
        except ValueError:
            pass

        if len(items) == 0:
            tkMessageBox.showerror("No dataset selected",
                                   "Please select the dataset(s) that you want to load.")
 
        for currentDataset in items:
            self.runDataset(self.dataset_list.get(currentDataset))

    def addDataset(self, file):
        currentDataset = {}

        try:
            configInfo = self.config.read(file)
        except DataError:
            tkMessageBox.showerror("Could not detect valid knossos data",
                                   "The directory specified does not appear "
                                   "to contain a valid knossos dataset.")
            return False

        currentDataset["Name"] = configInfo[1]
        currentDataset["Path"] = configInfo[2]
        currentDataset["Scales"] = configInfo[3]
        currentDataset["Boundaries"] = configInfo[4]
        currentDataset["Magnification"] = configInfo[5]
        currentDataset["Source"] = configInfo[6]

        if self.config.check(currentDataset):
            tkMessageBox.showerror("Incomplete configuration",
                                  "The configuration for this dataset "
                                  "is incomplete. Run the configuration "
                                  "tool on this dataset and try again.")
            return False

        if self.datasets.has_key(currentDataset["Name"]):
            tkMessageBox.showerror("Dataset already added",
                                   "A dataset with the name \"%s\" has already been\n"
                                   "added. Two datasets cannot have the same name." \
                                   % currentDataset["Name"] )
        
        self.datasets[currentDataset["Name"]] = {}
        self.datasets[currentDataset["Name"]]["Name"] = currentDataset["Name"]
        self.datasets[currentDataset["Name"]]["Path"] = currentDataset["Path"]

        self.updateList()

    def delDataset(self, dataset):
        del self.datasets[dataset]

    def runDataset(self, name):
        try:
            path = os.path.abspath(self.datasets[name]["Path"])
        except KeyError:
            tkMessageBox.showerror("Path to dataset does not exist",
                                   "The dataset named \"%s\" could not be found." \
                                    % name )
            return True

        if not self.kserver.isAlive():
            tkMessageBox.showerror("Synchronization server problem",
                                   "The synchronization server for this kuku "
                                   "session is not running. This either means "
                                   "the server crashed and you should restart "
                                   "kuku or you are running the server "
                                   "independently of this instance of kuku and "
                                   "you can disregard this message.")

        knossosPath = os.path.abspath(sys.path[0] + "/../knossos")
        if not os.path.exists(knossosPath):
            knossosPath = os.path.abspath(sys.path[0] + "/../knossos.exe")
            if not os.path.exists(knossosPath):
                tkMessageBox.showerror("Error calling knossos",
                                       "Could not call the knossos executable. It "
                                       "should be placed one directory above the "
                                       "kuku script and be executable by the user "
                                       "running kuku.")
                return False

        arguments = [knossosPath]
        arguments.append("--data-path=" + path + "/")

        knossosWD = os.path.abspath(sys.path[0] + "/../")

        # Add the configuration options to the command line
        # There are many more cli options that are documented
        # in knossos.c
        self.updateConfigInfo()

        if(self.globalConf["Autoconnect"].get()):
            arguments.append("--connect-asap")

        arguments.append("--supercube-edge=" + str(self.globalConf["Supercube edge"]))

        if(self.globalConf["Profile name"]):
            arguments.append("--profile=" + str(self.globalConf["Profile name"]))

        print(arguments)

        try:
            subprocess.Popen(arguments, cwd=knossosWD)
        except:
            tkMessageBox.showerror("Error calling knossos",
                                   "There was an error running the knossos "
                                   "executable. Check if you have the "
                                   "required permissions.")

    def updateList(self):
        self.dataset_list.delete(0, interface.END)
        
        try:
            for currentDataset in self.datasets.iterkeys():
                self.dataset_list.insert(interface.END, currentDataset)
        except AttributeError:
            for currentDataset in self.datasets.keys():
                self.dataset_list.insert(interface.END, currentDataset)

    def updateConfigInfo(self, *args):
        # Synchronize configuration data structures with UI
        # configuration options

        try:
            self.globalConf["Supercube edge"] = int(self.m_intentry.get())
        except ValueError:
            pass

        try:
            self.globalConf["Profile name"] = self.profilename_stringentry.get()
        except ValueError:
            pass

    def configFilePath(self):
        if "APPDATA" in os.environ:         #Set directory to store conf file
            configdir = os.environ["APPDATA"]
        elif "XDG_CONFIG_HOME" in os.environ:
            configdir = os.environ["XDG_CONFIG_HOME"]
        else:
            configdir = os.path.abspath(sys.path[0])

        configpath = os.path.join(configdir,"kuku.conf")
        return configpath

    def exit(self):
        pathListString = ""
        try:
            for currentDataset in self.datasets.iterkeys():
                currentPath = self.datasets[currentDataset]["Path"]
                pathListString = pathListString + currentPath + '\n'
        except AttributeError:
            for currentDataset in self.datasets.keys():
                currentPath = self.datasets[currentDataset]["Path"]
                pathListString = pathListString + currentPath + '\n'

        kukupath = self.configFilePath()
        
        if pathListString:
            try:
                # Write kuku.conf to the current directory
                
                kukuFile = open(kukupath, "w+")

                kukuFile.write(pathListString)
                kukuFile.close()
            except IOError:
                pass
        else:
            if os.path.isfile(kukupath):
                os.remove(kukupath)
                
        self.root.destroy()
        pass


def main():
    root = interface.Tk()
    runknossos = RunKnossos(root)
    root.title('Knossos starter')
    try:
        run()
    except NameError:
        pass
    root.protocol('WM_DELETE_WINDOW', runknossos.exit)
    root.mainloop()

if __name__ == '__main__': main()
