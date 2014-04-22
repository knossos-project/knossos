from PythonQt.Qt import *
from PythonQt.QtGui import *
import IPython

class Async(QtCore.QRunnable):
	def __init__(self):
		super(Async, self).__init__()

	def run(self):
		self.embed = IPython.start_kernel()
	

async = Async()
async.setAutoDelete(False)
pool = QThreadPool()


