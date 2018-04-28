from PythonQt import QtGui, Qt
import KnossosModule
import numpy, traceback, re, time
from scipy import ndimage
from skimage.morphology import watershed

class watershedSplitterHybridModeWidget(QtGui.QWidget):
    SUBOBJECT_TABLE_GROUP_STR = "Subobject Table"
    SUBOBJECT_ID_COLUMN_STR = "ID"
    SUBOBJECT_COORD_COLUMN_STR = "Coordinate"
    SUBOBJECT_MORE_COORDS_COLUMN_STR = "Subseeds Coordinates"
    OBJECT_LIST_COLUMNS = [SUBOBJECT_ID_COLUMN_STR, SUBOBJECT_COORD_COLUMN_STR, SUBOBJECT_MORE_COORDS_COLUMN_STR]

    class BusyCursorScope:
        def __init__(self):
            Qt.QApplication.setOverrideCursor(QtGui.QCursor(Qt.Qt.WaitCursor))
            Qt.QApplication.processEvents()
            return

        def __del__(self):
            Qt.QApplication.restoreOverrideCursor()
            Qt.QApplication.processEvents()
            return
        pass

    class MyTableWidget(QtGui.QTableWidget):
        def __init__(self, delF, parent=None):
            QtGui.QTableWidget.__init__(self,parent)
            self._delF = delF
            return

        def keyPressEvent(self, event):
            if event.key() == Qt.Qt.Key_Delete:
                self._delF()
            return QtGui.QTableWidget.keyPressEvent(self,event)
        pass

    def initGUI(self):
        self.setWindowTitle("Watershed Splitter Hybrid Mode Widget")
        widgetLayout = QtGui.QVBoxLayout()
        self.setLayout(widgetLayout)
        self.markerRadiusEdit = QtGui.QLineEdit()
        self.baseSubObjIdEdit = QtGui.QLineEdit()
        self.workAreaSizeEdit = QtGui.QLineEdit()
        opButtonsLayout = QtGui.QHBoxLayout()
        widgetLayout.addLayout(opButtonsLayout)
        self.resetButton = QtGui.QPushButton("Reset")
        self.resetButton.enabled = False
        self.resetButton.clicked.connect(self.resetButtonClicked)
        opButtonsLayout.addWidget(self.resetButton)
        self.finishButton = QtGui.QPushButton("Finish")
        self.finishButton.enabled = False
        self.finishButton.clicked.connect(self.finishButtonClicked)
        opButtonsLayout.addWidget(self.finishButton)
        self.subObjTableGroupBox = QtGui.QGroupBox("SubObjects")
        subObjTableLayout = QtGui.QVBoxLayout()
        widgetLayout.addLayout(subObjTableLayout)
        self.subObjTableGroupBox.setLayout(subObjTableLayout)
        self.subObjTable = self.MyTableWidget(self.subObjTableDel)
        subObjTableWidget = QtGui.QWidget()
        subObjTableLayout.addWidget(subObjTableWidget)
        subObjTableLayout = QtGui.QVBoxLayout()
        subObjTableWidget.setLayout(subObjTableLayout)
        subObjTableLayout.addWidget(self.subObjTable)
        self.setTableHeaders(self.subObjTable, self.OBJECT_LIST_COLUMNS)
        self.finalizeTable(self.subObjTable)
        # Invisibles
        self.widgetWidthEdit = QtGui.QLineEdit()
        self.widgetHeightEdit = QtGui.QLineEdit()
        self.widgetLeftEdit = QtGui.QLineEdit()
        self.widgetTopEdit = QtGui.QLineEdit()
        self.curFont = QtGui.QFont()
        # Window settings
        self.setWindowFlags(Qt.Qt.Window) # Yes, this has to be a called separately, or else the next call works wrong
        self.setWindowFlags((self.windowFlags() | Qt.Qt.CustomizeWindowHint) & ~Qt.Qt.WindowCloseButtonHint)
        self.resize(0,0)
        return

    def __init__(self, parent=KnossosModule.knossos_global_mainwindow):
        super(main_class, self).__init__(parent, Qt.Qt.WA_DeleteOnClose)
        KnossosModule.plugin_container[main_class.__name__] = self
        self.initGUI()
        self.initLogic()
        return

    def finalizeTable(self, table):
        table.horizontalHeader().setStretchLastSection(True)
        self.resizeTable(table)
        table.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        table.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
        return

    def applyGuiConfig(self):
        width = int(self.widgetWidthEdit.text)
        height = int(self.widgetHeightEdit.text)
        left = int(self.widgetLeftEdit.text)
        top = int(self.widgetTopEdit.text)
        self.resize(width,height)
        if (left != 0) and (top != 0):
            self.pos = Qt.QPoint(left,top)
        return

    def generateGuiConfig(self):
        self.widgetWidthEdit.text = str(self.size.width())
        self.widgetHeightEdit.text = str(self.size.height())
        self.widgetLeftEdit.text = str(self.pos.x())
        self.widgetTopEdit.text = str(self.pos.y())
        self.baseSubObjIdEdit.text = str(self.baseSubObjId)
        return

    def loadConfig(self):
        settings = Qt.QSettings()
        settings.beginGroup(self.pluginConf)
        for (widget,key,default) in self.settings:
            val = settings.value(key)
            if (val == None) or (str(val)==""):
                val_str = default
            else:
                val_str = str(val)
            if type(default)==type("a"):
                widget.text = val_str
            elif type(default)==type(True):
                widget.setChecked(bool(int(val_str)))
        settings.endGroup()
        return

    def saveConfig(self):
        settings = Qt.QSettings()
        settings.beginGroup(self.pluginConf)
        for (widget,key,default) in self.settings:
            if type(default)==type("a"):
                val_str = str(widget.text)
            elif type(default)==type(True):
                val_str = str(int(widget.isChecked()))
            settings.setValue(key,val_str)
        settings.endGroup()
        return

    def signalsConnect(self):
        for (signal, slot) in self.signalConns:
            signal.connect(slot)
        return

    def signalsDisonnect(self):
        for (signal, slot) in self.signalConns:
            signal.disconnect(slot)
        return

    def initLogic(self):
        self.active = False
        self.pluginConf = "Plugin_watershedSplitterHybridModeWidget"
        self.settings = [(self.baseSubObjIdEdit,"BASE_SUB_OBJ_ID","1000000000"), \
                        (self.workAreaSizeEdit,"WORK_AREA_SIZE","250,250,250"), \
                        (self.markerRadiusEdit,"MARKER_RADIUS","3"), \
                       (self.widgetWidthEdit,"WIDGET_WIDTH", "0"), \
                       (self.widgetHeightEdit,"WIDGET_HEIGHT", "0"), \
                        (self.widgetLeftEdit,"WIDGET_LEFT", "0"), \
                         (self.widgetTopEdit,"WIDGET_TOP", "0")]
        self.signalConns = []
        self.signalConns.append((KnossosModule.signalRelay.Signal_EventModel_handleMouseReleaseMiddle, self.handleMouseReleaseMiddle))
        self.signalsConnect()
        return

    def setTableHeaders(self, table, columnNames):
        columnNum = len(columnNames)
        table.setColumnCount(columnNum)
        for i in range(columnNum):
            twi = QtGui.QTableWidgetItem(columnNames[i])
            table.setHorizontalHeaderItem(i, twi)
        return

    def clearTable(self):
        table = self.subObjTable
        table.clearContents()
        table.setRowCount(0)
        return

    def resizeTable(self, table):
        table.resizeColumnsToContents()
        table.resizeRowsToContents()
        return

    def addTableRow(self, table, columnTexts, isSlack):
        rowIndex = 0
        table.insertRow(rowIndex)
        for i in range(len(columnTexts)):
            twi = QtGui.QTableWidgetItem(columnTexts[i])
            twi.setFlags(twi.flags() & (~Qt.Qt.ItemIsEditable))
            self.curFont.setItalic(isSlack)
            twi.setFont(self.curFont)
            table.setItem(rowIndex, i, twi)
        self.resizeTable(table)
        return

    def str2tripint(self, s):
        tripint = list(map(int, re.findall(r"[\w']+", s)))
        assert(len(tripint) == 3)
        return tripint

    def nextId(self):
        return max(list(self.mapIdToCoord.keys()) + [self.baseSubObjId - 1]) + 1

    def IdFromRow(self, row):
        table = self.subObjTable
        if (row > table.rowCount) or (row < 0):
            return self.invalidId
        return int(table.item(row,0).text())

    def RowFromId(self, Id):
        table = self.subObjTable
        for row in range(table.rowCount):
            if self.IdFromRow(row) == Id:
                return row
        return self.invalidRow

    def getTableSelectedRow(self):
        table = self.subObjTable
        return [x.row() for x in table.selectionModel().selectedRows()]

    def TreeIdById(self,Id):
        if Id in self.mapIdToTreeId:
            return self.mapIdToTreeId[Id]
        treeId = KnossosModule.skeleton.findAvailableTreeID()
        KnossosModule.skeleton.add_tree(treeId)
        self.mapIdToTreeId[Id] = treeId
        return treeId

    def addMoreCoords(self, coord, coord_offset, vpId):
        self.moreCoords.append((coord,coord_offset,vpId))
        self.addNode(coord, self.TreeIdById(self.nextId()), vpId)
        return

    def addNode(self,coord,treeId,vpId):
        nodeId = KnossosModule.skeleton.findAvailableNodeID()
        KnossosModule.skeleton.add_node(*((nodeId,)+coord+(treeId,self.markerRadius,vpId,)))
        return nodeId

    def displayCoord(self,coord):
        return tuple(numpy.array(coord)+1)

    def matrixDelId(self,Id,isSlack):
        for coordTuple in self.mapIdToSeedTuples[Id]:
            coord_offset = coordTuple[1]
            if isSlack:
                self.memPredPad[tuple(numpy.array(coord_offset)+self.pad)] = True
            else:
                self.seedMatrix[coord_offset] = 0
        del self.mapIdToSeedTuples[Id]
        return

    def matrixSetId(self,coordTuples,Id,isSlack):
        for coordTuple in coordTuples:
            coord_offset = coordTuple[1]
            if isSlack:
                self.memPredPad[tuple(numpy.array(coord_offset)+self.pad)] = False
            else:
                self.seedMatrix[coord_offset] = Id
        self.mapIdToSeedTuples[Id] = coordTuples
        return

    def addSeed(self, coord, coord_offset, vpId, isSlack=False):
        Id = self.nextId()
        coordTuples = [(coord, coord_offset, vpId)] + self.moreCoords
        self.matrixSetId(coordTuples,Id,isSlack)
        self.mapIdToSlack[Id] = isSlack
        self.mapIdToCoord[Id] = coord
        self.mapIdToNodeId[Id] = self.addNode(coord,self.TreeIdById(Id),vpId)
        self.mapIdToMoreCoords[Id] = [curCoord[0] for curCoord in self.moreCoords]
        self.refreshTable()
        self.calcWS()
        if not isSlack:
            KnossosModule.segmentation.changeComment(self.ObjIndexFromId(Id),"WatershedSplitter")
        self.moreCoords = []
        self.updateFinishButton()
        return

    def refreshTable(self):
        table = self.subObjTable
        self.clearTable()
        for (Id,coord) in self.getSortedMapItems():
            self.addTableRow(table, [str(Id), str(self.displayCoord(coord)), str(list(map(self.displayCoord,self.mapIdToMoreCoords[Id])))], self.mapIdToSlack[Id])
        return

    def updateFinishButton(self):
        self.finishButton.enabled = len(self.nonSlacks()) > 1
        return

    def removeSeeds(self,Ids):
        if len(Ids) == 0:
            return
        for Id in Ids:
            KnossosModule.segmentation.removeObject(self.ObjIndexFromId(Id))
            self.matrixDelId(Id,self.mapIdToSlack[Id])
            del self.mapIdToCoord[Id]
            KnossosModule.skeleton.delete_tree(self.mapIdToTreeId[Id])
            del self.mapIdToTreeId[Id]
            del self.mapIdToNodeId[Id]
            del self.mapIdToSlack[Id]
        self.refreshTable()
        self.calcWS()
        self.updateFinishButton()
        return

    def subObjTableDel(self):
        rows = self.getTableSelectedRow()
        if len(rows) == 0:
            return
        self.removeSeeds([self.IdFromRow(row) for row in rows])
        return

    def getSortedMapItems(self):
        IdCoordTuples = list(self.mapIdToCoord.items())
        IdCoordTuples.sort()
        return IdCoordTuples

    def waitForLoader(self):
        busyScope = self.BusyCursorScope()
        while not KnossosModule.knossos_global_loader.isFinished():
            Qt.QApplication.processEvents()
            time.sleep(0)
        return

    def coordOffset(self,coord):
        return tuple(numpy.array(coord) - self.beginCoord_arr)

    def nonSlacks(self):
        h = self.mapIdToSlack
        return [x for x in h if not h[x]]

    def calcWS(self):
        busyScope = self.BusyCursorScope()
        nonSlackIds = self.nonSlacks()
        nonSlackCount = len(nonSlackIds)
        if nonSlackCount == 0:
            self.WS_masked[self.WS_mask] = self.curObjId
        elif nonSlackCount == 1:
            self.WS_masked[self.WS_mask] = nonSlackIds[0]
        else:
            self.distMemPred = -ndimage.distance_transform_edt(self.memPredPad)
            pad = self.pad
            self.distMemPred = self.distMemPred[pad:-pad,pad:-pad,pad:-pad]
            self.distMemPred = self.scaleMatrix(self.distMemPred,0,1)
            seededDist = self.distMemPred-((self.seedMatrix > 0)*1.0)
            ws = watershed(seededDist, self.seedMatrix, None, None, self.WS_mask)
            self.WS_masked[self.WS_mask] = ws[self.WS_mask]
        self.writeMatrix(self.WS_masked)
        return

    def setObjId(self,Id):
        busyScope = self.BusyCursorScope()
        self.curObjId = Id
        self.WS_mask = self.orig == self.curObjId
        self.WS_masked[self.WS_mask] = self.orig[self.WS_mask]
        pad = self.pad
        self.memPredPad = numpy.pad(self.WS_mask,((pad,pad),)*3,'constant',constant_values=((False,False),)*3)
        self.writeMatrix(self.WS_masked)
        return

    def handleMouseReleaseMiddle(self, eocd, clickedCoord, vpId, event):
        coord = tuple(clickedCoord.vector())
        if not self.active:
            self.begin(coord)
        coord_offset = self.coordOffset(coord)
        mods = event.modifiers()
        if self.curObjId == self.invalidId:
            if mods == 0:
                self.setObjId(self.orig[coord_offset])
            else:
                QtGui.QMessageBox.information(0, "Error", "First select object!")
            return
        if self.WS_mask[coord_offset] == False:
            QtGui.QMessageBox.information(0, "Error", "Click inside mask!")
            return
        if mods == 0:
            self.addSeed(coord,coord_offset,vpId)
        elif mods == Qt.Qt.ShiftModifier:
            self.addMoreCoords(coord,coord_offset,vpId)
        elif mods == Qt.Qt.ControlModifier:
            self.addSeed(coord,coord_offset,vpId,isSlack=True)
        return

    def npDataPtr(self, matrix):
        return matrix.__array_interface__["data"][0]

    def accessMatrix(self, matrix, isWrite):
        self.waitForLoader()
        return KnossosModule.knossos.processRegionByStridedBufProxy(list(self.beginCoord_arr), list(self.dims_arr), self.npDataPtr(matrix), matrix.strides, isWrite, True)

    def writeMatrix(self, matrix):
        self.accessMatrix(matrix, True)
        return

    def newMatrix(self,dims=None,dtype=None):
        if dims == None:
            dims = self.dims_arr
        if dtype == None:
            dtype = "uint64"
        return numpy.ndarray(shape=dims, dtype=dtype)

    def newValMatrix(self, val, dtype=None):
        matrix = self.newMatrix(dtype=dtype)
        matrix.fill(val)
        return matrix

    def newTrueMatrix(self):
        return self.newValMatrix(True,dtype="bool")

    def readMatrix(self, matrix):
        self.accessMatrix(matrix, False)
        return

    def commonEnd(self):
        self.active = False
        self.clearTable()
        for treeId in list(self.mapIdToTreeId.values()):
            KnossosModule.skeleton.delete_tree(treeId)
        self.guiEnd()
        self.endMatrices()
        if not self.confined:
            KnossosModule.knossos.resetMovementArea()
        self.generateGuiConfig()
        self.saveConfig()
        if KnossosModule.segmentation.isRenderAllObjs():
            KnossosModule.segmentation.setRenderAllObjs(self.prevRenderAllObjs)
        return

    def guiBegin(self):
        self.resetButton.enabled = True
        self.show()
        Qt.QApplication.processEvents()
        return

    def guiEnd(self):
        self.hide()
        Qt.QApplication.processEvents()
        self.resetButton.enabled = False
        self.finishButton.enabled = False
        return

    def scaleMatrix(self,m,minVal,maxVal):
        curMinVal = m.min()
        curRange = m.max() - curMinVal
        newRange = maxVal - minVal
        return ((m - curMinVal)*(newRange/curRange))+minVal

    def beginMatrices(self):
        self.orig = self.newMatrix(dims=self.dims_arr)
        self.readMatrix(self.orig)
        self.seedMatrix = self.newValMatrix(0)
        self.WS_masked = self.newValMatrix(0)
        self.distMemPred = self.newValMatrix(0)
        return

    def endMatrices(self):
        del self.orig
        del self.seedMatrix
        del self.WS_masked
        del self.distMemPred
        if self.curObjId == self.invalidId:
            return
        del self.WS_mask
        del self.memPredPad
        return

    def ObjIndexFromId(self,Id):
        coord = self.mapIdToCoord[Id]
        KnossosModule.segmentation.subobjectFromId(Id, coord)
        return KnossosModule.segmentation.largestObjectContainingSubobject(Id,(0,0,0))

    def resetSubObjs(self):
        for Id in self.nonSlacks():
            KnossosModule.segmentation.removeObject(self.ObjIndexFromId(Id))
        return

    def beginSeeds(self):
        self.curObjId = self.invalidId
        self.mapIdToCoord = {}
        self.mapIdToTreeId = {}
        self.moreCoords = []
        self.mapIdToMoreCoords = {}
        self.mapIdToNodeId = {}
        self.mapIdToSlack = {}
        self.mapIdToTodo = {}
        self.mapIdToSeedTuples = {}
        return

    def isSizeSmaller(self,smallSize,refSize):
        return not (True in (smallSize > refSize))

    def setPositionWrap(self, coord):
        KnossosModule.knossos.setPosition(coord)
        self.waitForLoader()
        return

    def begin(self,coord):
        retVal = True
        try:
            self.loadConfig()
            self.applyGuiConfig()
            self.invalidId = 0
            self.invalidRow = -1
            self.pad = 1
            # parse edits
            self.dims_arr = numpy.array(self.str2tripint(str(self.workAreaSizeEdit.text)))
            self.baseSubObjId = int(str(self.baseSubObjIdEdit.text))
            self.markerRadius = int(self.markerRadiusEdit.text)
            movementArea_arr = numpy.array(KnossosModule.knossos.getMovementArea())
            self.movementAreaBegin_arr, self.movementAreaEnd_arr = movementArea_arr[:3], movementArea_arr[3:]+1
            self.movementAreaSize_arr = self.movementAreaEnd_arr - self.movementAreaBegin_arr
            if self.isSizeSmaller(self.movementAreaSize_arr, self.dims_arr):
                self.confined = True
                self.dims_arr = self.movementAreaSize_arr
                self.beginCoord_arr = self.movementAreaBegin_arr
                self.endCoord_arr = self.movementAreaEnd_arr
            else:
                self.confined = False
                self.setPositionWrap(coord)
                self.middleCoord_arr = numpy.array(KnossosModule.knossos.getPosition())
                self.beginCoord_arr = self.middleCoord_arr - (self.dims_arr/2)
                self.endCoord_arr = self.beginCoord_arr + self.dims_arr - 1
                KnossosModule.knossos.setMovementArea(list(self.beginCoord_arr), list(self.endCoord_arr))
            self.beginSeeds()
            self.beginMatrices()
            self.guiBegin()
            self.prevRenderAllObjs = KnossosModule.segmentation.isRenderAllObjs()
            KnossosModule.segmentation.setRenderAllObjs(True)
            self.active = True
        except:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            inf = "\n".join(traceback.format_exception(exc_type, exc_value, exc_traceback))
            QtGui.QMessageBox.information(0, "Error", "Exception caught!\n" + inf)
            retVal = False
        return retVal

    def resetButtonClicked(self):
        self.resetSubObjs()
        self.writeMatrix(self.orig)
        self.commonEnd()
        return

    def finishButtonClicked(self):
        self.baseSubObjId = 1 + max(self.nonSlacks())
        self.orig[self.WS_mask] = self.WS_masked[self.WS_mask]
        self.writeMatrix(self.orig)
        self.commonEnd()
        return

main_class = watershedSplitterHybridModeWidget
main_class()
