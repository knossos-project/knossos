from PythonQt.QtGui import *

""" This script makes a screenshot from a viewport	
	Note: before executing this file make sure that your mousepointer is hovering any viewport 
"""

widget = QWidget()
viewport = app.widgetAt(QCursor.pos())
image = viewport.grabFrameBuffer()

image.save("invers.png")	 


