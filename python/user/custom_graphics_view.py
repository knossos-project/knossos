#!/home/amos/anaconda/bin/python

from PythonQt.Qt import *
from PythonQt.QtGui import *

class CGScene(QGraphicsScene):
	def __init__(self, parent = None):
		super(CGScene, self).__init__(self)
		
	

class Texture(QGraphicsItem):
	def __init__(self, pos, tex):
		super(Texture, self).__init__(self)
		self.tex = tex
		self.setPos(pos)

	def paint(self, painter, item, widget):
		painter.drawPixmap(self.x(), self.y(), self.tex)	

	def mousePressEvent(self, event):			
		QGraphicsItem.mousePressEvent(self.event)
		QGraphicsItem.update()	
		
	
class CGView(QGraphicsView):
	def __init__(self, scene):
		super(CGView, self).__init__(scene)
			
	def wheelEvent(self, event):		
		if event.delta() > 0:
			self.zoom(1.2)
		else:
			self.zoom(1 / 1.2)

	def zoom(self, factor):
		scaling = self.transform().scale(factor, factor).mapRect(QRectF(0, 0, 1, 1)).width()		
		if scaling < 0.07 or scaling > 100:
			return
		self.scale(factor, factor)
		
scene = CGScene()
view = CGView(scene)
view.setInteractive(True)
view.setBackgroundBrush(QBrush(QColor(0, 0, 0), QPixmap("python/user/images/ni.jpg")))
view.setSceneRect(-768, -512, 2000, 2000)
view.setOptimizationFlags(QGraphicsView.DontClipPainter | QGraphicsView.DontAdjustForAntialiasing);
view.setDragMode(QGraphicsView.ScrollHandDrag)
view.setCacheMode(QGraphicsView.CacheBackground);
view.setViewportUpdateMode(QGraphicsView.FullViewportUpdate);
view.setTransformationAnchor(QGraphicsView.AnchorUnderMouse);
view.setRenderHints(QPainter.SmoothPixmapTransform)

widget = QWidget()	
widget.setWindowTitle("CGExample")
widget.setGeometry(100, 100, 800, 800)
	
splitter = QSplitter(widget)
toolbox = QToolBox()

splitter.addWidget(toolbox)
splitter.addWidget(view) 			



tex = Texture(QPointF(0, 0), QPixmap("python/user/images/e1088_xy.png"))
tex.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemSendsScenePositionChanges)
tex.setAcceptedMouseButtons(Qt.LeftButton | Qt.RightButton);
tex.setAcceptHoverEvents(True);
scene.addItem(tex)

label = QLabel()
label.setPixmap(QPixmap(":/images/splash.png"))
proxy = scene.addWidget(label)
proxy.setScale(0.5)

group_box = QGroupBox()
layout = QHBoxLayout(group_box)
group_proxy = scene.addWidget(group_box)
group_proxy.setPos(0, 0)



rect = QRectF(-64, -64, 64, 128)
pen = QColor(1, 1, 1)
brush = QBrush(QColor(0, 0, 0))

widget.show()


"""
class Watcher(QtCore.QRunnable):
    def __init__(self):
        super(Watcher, self).__init__()
        print "init"
    	
    def run(self):
	pass
        #view.rotate(5)

	

watcher = Watcher()
timer = QTimer()
timer.setInterval(100)
timer.timeout.connect(watcher.run)
timer.start(100)


pool = QThreadPool()
"""


