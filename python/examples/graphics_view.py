

from PythonQt import *
	
widget = QtGui.QWidget()
scene = QtGui.QGraphicsScene()
view = QtGui.QGraphicsView(scene)
view.setInteractive(True)
view.setBackgroundBrush(QtGui.QBrush(QtGui.QColor(0, 0, 0), QtGui.QPixmap("ni.jpg")))	
view.setSceneRect(-768, -512, 2000, 2000)	
splitter = QtGui.QSplitter(widget)
toolbox = QtGui.QToolBox()

splitter.addWidget(toolbox)
splitter.addWidget(view) 			
	
widget.setWindowTitle("CGExample")
widget.setGeometry(100, 100, 1000, 800)

tex = scene.addPixmap(QtGui.QPixmap("e1088_xy.png"))
tex.setPos(0., 0.)
rect = Qt.QRectF(-64, -64, 64, 128)
pen = QtGui.QColor(1, 1, 1)
brush = QtGui.QBrush(QtGui.QColor(0, 0, 0))
widget.show()
