#!/usr/bin/env python

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

from configknossos_ui import ConfigKnossosUI
from kconfig import DataError
import re, subprocess, os, sys
try:
    import tkinter, tkinter.filedialog
    interface = tkinter
    fileDialog = tkinter.filedialog
except ImportError:
    import Tkinter, tkFileDialog
    interface = Tkinter
    fileDialog = tkFileDialog
    

class ConfigKnossos(ConfigKnossosUI):
    def __init__(self, root):
        ConfigKnossosUI.__init__(self, root)

    def openDataset(self, path):       
        # knossosconfig.read() will try to open a knossos.conf file
        # and, if it is absent, try to guess some of the
        # parameters.
        try:
            configInfo = self.knossosconfig.read(path)
        except DataError:
            messageBox.showerror("Error detecting configuration parameters",
                                   "The directory specified does not appear "
                                   "to contain a valid knossos dataset.")
            return False

        self.config["Name"] = configInfo[1]
        self.config["Path"] = configInfo[2]
        self.config["Scales"] = configInfo[3]
        self.config["Boundaries"] = configInfo[4]
        self.config["Magnification"] = configInfo[5]
        self.config["Source"] = configInfo[6]

        # check() is False is we do not need further configuration
        # and True otherwise.
        if self.knossosconfig.check(self.config):
            messageBox.showinfo("Incomplete configuration",
                                  "The configuration information for this "
                                  "dataset is incomplete.\n"
                                  "Please finalize the configuration and "
                                  "save before running Knossos with this "
                                  "dataset.")

        # Clear currently displayed configuration
        # options
        self.path_entry.configure(state = interface.NORMAL)

        self.path_entry.delete(0, interface.END)
        self.name_entry.delete(0, interface.END)
        self.scalex_floatentry.delete(0, interface.END)
        self.scaley_floatentry.delete(0, interface.END)
        self.scalez_floatentry.delete(0, interface.END)
        self.boundaryx_intentry.delete(0, interface.END)
        self.boundaryy_intentry.delete(0, interface.END)
        self.boundaryz_intentry.delete(0, interface.END)
        self.magnification_intentry.delete(0, interface.END)

        # Fill in the values for the current dataset
        self.name_entry.insert(0, self.config["Name"])
        self.path_entry.insert(0, self.config["Path"])
        self.path_entry.configure(state = "readonly")

        if self.config["Scales"][0]:
            self.scalex_floatentry.insert(0, str(self.config["Scales"][0]))
        if self.config["Scales"][1]:
            self.scaley_floatentry.insert(0, str(self.config["Scales"][1]))        
        if self.config["Scales"][2]:
            self.scalez_floatentry.insert(0, str(self.config["Scales"][2]))

        if self.config["Boundaries"][0]:
            self.boundaryx_intentry.insert(0, str(self.config["Boundaries"][0]))
        if self.config["Boundaries"][1]:             
            self.boundaryy_intentry.insert(0, str(self.config["Boundaries"][1]))
        if self.config["Boundaries"][2]: 
            self.boundaryz_intentry.insert(0, str(self.config["Boundaries"][2]))

        if self.config["Magnification"]:
            self.magnification_intentry.insert(0, str(self.config["Magnification"]))

    def updateDataset(self):
        name = self.name_entry.get()

        try:
            scalex = float(self.scalex_floatentry.get())
            scaley = float(self.scaley_floatentry.get())
            scalez = float(self.scalez_floatentry.get())
            boundaryx = int(self.boundaryx_intentry.get())
            boundaryy = int(self.boundaryy_intentry.get())
            boundaryz = int(self.boundaryz_intentry.get())
            magnification = int(self.magnification_intentry.get())
        except ValueError:
            return False
        
        scale = (scalex, scaley, scalez)
        boundary = (boundaryx, boundaryy, boundaryz)

        self.config["Name"] = name
        self.config["Scales"] = scale
        self.config["Boundaries"] = boundary
        self.config["Magnification"] = magnification

    def has_unsaved_changes(self):
        name = self.name_entry.get()

        try:
            scalex = float(self.scalex_floatentry.get())
        except:
            scalex = None
        try:             
            scaley = float(self.scaley_floatentry.get())
        except:
            scaley = None
        try:
            scalez = float(self.scalez_floatentry.get())
        except:
            scalez = None
        try:
            boundaryx = int(self.boundaryx_intentry.get())
        except:
            boundaryx = None
        try:
            boundaryy = int(self.boundaryy_intentry.get())
        except:
            boundaryy = None
        try:
            boundaryz = int(self.boundaryz_intentry.get())
        except:
            boundaryz = None
        try:
            magnification = int(self.magnification_intentry.get())
        except:
            magnification = None

        scales = (scalex, scaley, scalez)
        boundaries = (boundaryx, boundaryy, boundaryz)

        try:
            if (scales != self.config["Scales"]) \
                or (boundaries != self.config["Boundaries"]) \
                or (magnification != self.config["Magnification"]):
                return True
            else:
                return False
        except KeyError:
            # In this case, we didn't even load a dataset, yet.
            return False

    # Open button
    #
    def open_button_command(self):
        path = os.path.abspath(fileDialog.askdirectory())
        self.openDataset(path)

    # Save configuration to disk button
    #
    def save_button_command(self):
        if self.updateDataset() == False:
            messageBox.showerror("Not saving data",
                                 "Please fill out all blank fields.\n")
            return False

        if self.knossosconfig.check(self.config):
            messageBox.showerror("Not saving data",
                                 "The configuration parameters given "
                                 "are not sufficient. You need to "
                                 "specifiy at least the experiment name "
                                 "and the scale values.")
            return False 

        try:
            self.knossosconfig.write(self.config)
        except IOError:
            messageBox.showerror("Error saving data",
                                 "Error while trying to save the configuration.")
            return False

    def exit(self):
        if self.has_unsaved_changes():
            if messageBox.askyesno("Unsaved changes",
                                   "There are unsaved changes.\nSave now?"):
                
                if self.save_button_command() == False:
                    return False

        self.root.destroy()

def main():
    try:
        userinit()
    except NameError:
        pass
    root = interface.Tk()
    configknossos = ConfigKnossos(root)
    root.title('Configure KNOSSOS data')
    try: run()
    except NameError:
        pass
    root.protocol('WM_DELETE_WINDOW', configknossos.exit)
    root.mainloop()

if __name__ == '__main__': main()
