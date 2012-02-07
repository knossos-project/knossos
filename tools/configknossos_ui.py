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

import Pmw
try:
    import tkinter
    interface = tkinter
except ImportError:
    import Tkinter
    interface = Tkinter
import os
import kconfig

class ConfigKnossosUI():
    def __init__(self, root):
        self.config = {}

        self.knossosconfig = kconfig.KnossosConfig()
        self.root = root

        # Widget Initialization

        # GUI elements to open / save configs
        #
        self.open_button = interface.Button(root,
            text = "Open",
        )
        self.save_button = interface.Button(root,
            text="Save"
        )
        self.path_entry = interface.Entry(root,
            state="readonly",
        )

        # GUI elements to change configs
        #
        self.dataconfig_labelframe = interface.LabelFrame(root,
            text = "Configuration options for dataset"
        )
        self.name_label = interface.Label(self.dataconfig_labelframe,
                text = "Name"
        )
        self.scalex_label = interface.Label(self.dataconfig_labelframe,
                text = "Scale X"
        )
        self.scaley_label = interface.Label(self.dataconfig_labelframe,
                text = "Scale Y"
        )
        self.scalez_label = interface.Label(self.dataconfig_labelframe,
                text = "Scale Z"
        )
        self.boundaryx_label = interface.Label(self.dataconfig_labelframe,
                text = "Boundary X"
        )
        self.boundaryy_label = interface.Label(self.dataconfig_labelframe,
                text = "Boundary Y"
        )
        self.boundaryz_label = interface.Label(self.dataconfig_labelframe,
                text = "Boundary Z"
        )
        self.magnification_label = interface.Label(self.dataconfig_labelframe,
                text = "Magnification"
        )
        self.name_entry = interface.Entry(self.dataconfig_labelframe,
                text = "Name"
        )
        self.scalex_floatentry = FloatEntry(self.dataconfig_labelframe
        )
        self.scaley_floatentry = FloatEntry(self.dataconfig_labelframe
        )
        self.scalez_floatentry = FloatEntry(self.dataconfig_labelframe
        )
        self.boundaryx_intentry = IntEntry(self.dataconfig_labelframe
        )
        self.boundaryy_intentry = IntEntry(self.dataconfig_labelframe
        )
        self.boundaryz_intentry = IntEntry(self.dataconfig_labelframe
        )
        self.magnification_intentry = IntEntry(self.dataconfig_labelframe
        )

        # widget commands

        self.open_button.configure(
            command = self.open_button_command
        )
        self.save_button.configure(
            command = self.save_button_command
        )

        # Geometry Management

        # GUI elements to open / save dataset configs
        #

        self.path_entry.grid(
            in_ = root,
            column = 1,
            row = 1,
            sticky = "WE"
        )
        self.open_button.grid(
            in_    = root,
            column = 2,
            row    = 1,
            sticky = "WE"
        )
        self.save_button.grid(
            in_    = root,
            column = 3,
            row    = 1,
            sticky = "WE"
        )

        # GUI elements for changing config options
        #
        self.dataconfig_labelframe.grid(
            in_    = root,
            column = 1,
            row    = 20,
            columnspan = 3,
            ipadx = 0,
            ipady = 0,
            padx = 0,
            pady = 0,
            rowspan = 1,
            sticky = "WENS"
        )
        self.name_label.grid(
            in_ = self.dataconfig_labelframe,
            column = 0,
            row = 5
        )
        self.scalex_label.grid(
            in_ = self.dataconfig_labelframe,
            column = 0,
            row = 6
        )
        self.scaley_label.grid(
            in_ = self.dataconfig_labelframe,
            column = 0,
            row = 7
        )
        self.scalez_label.grid(
            in_ = self.dataconfig_labelframe,
            column = 0,
            row = 8
        )
        self.boundaryx_label.grid(
            in_ = self.dataconfig_labelframe,
            column = 2,
            row = 5
        )
        self.boundaryy_label.grid(
            in_ = self.dataconfig_labelframe,
            column = 2,
            row = 6
        )
        self.boundaryz_label.grid(
            in_ = self.dataconfig_labelframe,
            column = 2,
            row = 7
        )
        self.name_entry.grid(
            in_ = self.dataconfig_labelframe,
            column = 1,
            row = 5
        )
        self.scalex_floatentry.grid(
            in_ = self.dataconfig_labelframe,
            column = 1,
            row = 6
        )
        self.scaley_floatentry.grid(
            in_ = self.dataconfig_labelframe,
            column = 1,
            row = 7
        )
        self.scalez_floatentry.grid(
            in_ = self.dataconfig_labelframe,
            column = 1,
            row = 8
        )
        self.boundaryx_intentry.grid(
            in_ = self.dataconfig_labelframe,
            column = 3,
            row = 5
        )
        self.boundaryy_intentry.grid(
            in_ = self.dataconfig_labelframe,
            column = 3,
            row = 6
        )
        self.boundaryz_intentry.grid(
            in_ = self.dataconfig_labelframe,
            column = 3,
            row = 7
        )
        self.magnification_label.grid(
            in_ = self.dataconfig_labelframe,
            column = 2,
            row = 8
        )
        self.magnification_intentry.grid(
            in_ = self.dataconfig_labelframe,
            column = 3,
            row = 8
        )

        # Resize Behavior
        root.grid_columnconfigure(1, weight = 0, minsize = 44, pad = 0)
        root.grid_columnconfigure(2, weight = 0, minsize = 69, pad = 0)
        root.grid_columnconfigure(3, weight = 0, minsize = 40, pad = 0)

class IntEntry(interface.Entry):
    def __init__(self, parent):
        interface.Entry.__init__(self, parent, validate="focusout", validatecommand=self.validate_int)

    def validate_int(self, *args):
        if self.get():
            try:
                v = int(self.get())
                return True
            except ValueError:
                tkMessageBox.showwarning("Incorrect data type", "Please input an integer")
                self.delete(0, interface.END)
                return False
        else:
            return True

class FloatEntry(interface.Entry):
    def __init__(self, parent):
        interface.Entry.__init__(self, parent, validate="focusout", validatecommand=self.validate_float)

    def validate_float(self, *args):
        if self.get():
            try:
                v = float(self.get())
                return True
            except ValueError:
                tkMessageBox.showwarning("Incorrect data type", "Please input a floating point number (like 22.0)")
                self.delete(0, interface.END)
                return False
        else:
            return True
