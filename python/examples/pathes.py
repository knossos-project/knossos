""" 
	This file explains how you can extend the python system path and the current working directory and what KNOSSOS else could do for you. 
	This is for example important if you have a collection of python modules and packages as
	they are not automatically visible for the interpreter from the KNOSSOS installation directory. This can be easily modified.  
"""

your_sys_path = ' ' # adjust this path 

import sys
print sys.path # to check which pathes have been already added
sys.path.append(your_sys_path) # to extend the path to another directory. Pay attention for operating system dependent path delimeter. ('\'' on Windows, '/' on unix')

""" 
	This solves the problem but you'll still have to type in the full path to your script directory.
	If you want to change the current working directory use the following commands.
"""

your_working_directory = ' ' # adjust this path 

import os
print os.getcwd() # to check what's your current working directory. Default value is again the installation path of KNOSSOS.
os.chdir(path) # to change you current working directory.  

"""
	Another possibility is to tell KNOSSOS that the these pathes should be saved. This expects KNOSSOS has already started and you 
	executed the script from the KNOSSOS scripting console.	The pathes are automatically loaded the next time you start KNOSSOS.  
"""	
knossos.save_sys_path(your_sys_path)
knossos.save_working_directory(your_working_directory)

"""
   script files via scripting console can be called with the following command. 
"""

script_path =  ' ' # adjust this path

execfile(script_path)
