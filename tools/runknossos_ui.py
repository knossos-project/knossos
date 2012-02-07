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

try:
    import tkinter, tkinter.messagebox
    interface = tkinter
    messageBox = tkinter.messagebox
except ImportError:
    import Tkinter, tkMessageBox
    interface = Tkinter
    messageBox = tkMessageBox
import threading
import kconfig
from kiki import KikiServer

class RunKnossosUI():
    def __init__(self, root):
        # dataset name -> [dataset name,
        #                  path ""]
        self.datasets = {}
        self.globalConf = {}
        self.datasetsAdded = 0
        
        # Global configuration options
        # default values
        self.globalConf["Autoconnect"] = interface.IntVar()
        self.globalConf["Supercube edge"] = 5
        self.globalConf["Profile name"] = ""

        # Start the kiki synchronization server
        # setdaemon ensures that KikiServer will not block
        # the shutdown of kuku
        self.kserver = threading.Thread(None, KikiServer().go, "syncserver", ())
        self.kserver.setDaemon(True)
        self.kserver.start()

        self.config = kconfig.KnossosConfig()
        self.root = root

        # Widget Initialization
        self.dataset_list = interface.Listbox(root,
            selectmode = interface.EXTENDED,
        )
        self.add_button = interface.Button(root,
            text = "Add",
        )
        self.del_button = interface.Button(root,
            text = "Remove",
        )
        self.run_button = interface.Button(root,
            text = "Run",
        )
        self.profilename_labelframe = interface.LabelFrame(root,
            text = "Profile"
        )
        self.profilename_stringentry = interface.Entry(self.profilename_labelframe,
            width = 35
        )
        self.globalconfig_labelframe = interface.LabelFrame(root,
            text = "Global configuration options"
        )
        self.m_label = interface.Label(self.globalconfig_labelframe,
            text = "Supercube edge"
        )
        self.m_intentry = IntEntry(self.globalconfig_labelframe
        )
        self.synchronize_checkbutton = interface.Checkbutton(self.globalconfig_labelframe,
            text = "Synchronize on start",
            variable = self.globalConf["Autoconnect"]
        )

        # widget commands

        self.add_button.configure(
            command = self.add_button_command
        )
        self.run_button.configure(
            command = self.run_button_command
        )
        self.del_button.configure(
            command = self.del_button_command
        )
        self.m_intentry.bind("<KeyRelease>", self.updateConfigInfo)
        self.profilename_stringentry.bind("<KeyRelease>", self.updateConfigInfo)

        # Geometry Management
        self.dataset_list.grid(
            in_    = root,
            column = 1,
            row    = 1,
            columnspan = 2,
            ipadx = 0,
            ipady = 0,
            padx = 0,
            pady = 0,
            rowspan = 1,
            sticky = "WENS"
        )

        self.add_button.grid(
            in_    = root,
            column = 1,
            row    = 2,
            columnspan = 1,
            ipadx = 0,
            ipady = 0,
            padx = 0,
            pady = 0,
            rowspan = 1,
            sticky = "WE"
        )
        self.del_button.grid(
            in_    = root,
            column = 2,
            row    = 2,
            columnspan = 1,
            ipadx = 0,
            ipady = 0,
            padx = 0,
            pady = 0,
            rowspan = 1,
            sticky = "WE"
        )
        self.run_button.grid(
            in_    = root,
            column = 1,
            row    = 3,
            columnspan = 2,
            ipadx = 0,
            ipady = 0,
            padx = 0,
            pady = 0,
            rowspan = 1,
            sticky = "WE"
        )
        self.profilename_labelframe.grid(
            in_ = root,
            column = 1,
            row = 7,
            columnspan = 2,
            sticky = "WENS"
        )
        self.profilename_stringentry.grid(
            in_  = self.profilename_labelframe,
            column = 1,
            row = 1,
            sticky = "WENS"
        )
        self.globalconfig_labelframe.grid(
            in_ = root,
            column = 1,
            row = 10,
            columnspan = 2,
            sticky = "WENS"
        )
        self.synchronize_checkbutton.grid(
            in_    = self.globalconfig_labelframe,
            column = 1,
            row    = 1,
            sticky = ""
        ) 
        self.m_intentry.grid(
            in_ = self.globalconfig_labelframe,
            column = 1,
            row = 2,
            sticky = ""
        )
        self.m_intentry.insert(0, self.globalConf["Supercube edge"])

        self.m_label.grid(
            in_ = self.globalconfig_labelframe,
            column = 2,
            row = 2,
            sticky = ""
        )

class IntEntry(interface.Entry):
    def __init__(self, parent):
        interface.Entry.__init__(self, parent, validate="focusout", validatecommand=self.validate_int)

    def validate_int(self, *args):
        if self.get():
            try:
                v = int(self.get())
                return True
            except ValueError:
                messageBox.showwarning("Incorrect data type", "Please input an integer")
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
                messageBox.showwarning("Incorrect data type", "Please input a floating point number (like 22.0)")
                self.delete(0, interface.END)
                return False
        else:
            return True
