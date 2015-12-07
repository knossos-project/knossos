from PythonQt import QtGui, Qt, QtCore
import KnossosModule
import urllib, urllib2, os, urlparse, glob, time, inspect, tempfile

#KNOSSOS_PLUGIN	Version	1
#KNOSSOS_PLUGIN	Description	Plugin manager is a plugin manager

class main_class(QtGui.QWidget):
    LOCAL_REPO = "<LOCAL>"
    PLUGIN_COLUMN_NAMES = ["Name","Repo","Local","Remote","Description","URL"]
    PLUGIN_NAME_COLUMN_INDEX = PLUGIN_COLUMN_NAMES.index("Name")
    DEFAULT_REPO_LIST_URL = "http://knossos-project.github.io/knossos-plugins/repo_list.txt"
    SETTING_GROUP_NAME = "Plugin_pluginMgr"
    SETTING_NAMES = {"SETTING_OVERWRITE_SAME_NAME":"OverwriteSame",
    "SETTING_QUIET_MODE_NAME":"QuietMode",
    "SETTING_OFFLINE_MODE_NAME":"OfflineMode",
    "SETTING_GEOMETRY":"Geometry",
    "SETTING_PLUGIN_TABLE_SPLIT_SIZES_NAME":"PluginTableSplitSizes",
    "SETTING_PANEL_SPLIT_SIZES_NAME":"PanelSplitSizes",
    "SETTING_REPO_LIST_URL_NAME":"RepoListURL"}
    METADATA_KEYWORD = "#KNOSSOS_PLUGIN"
    PLUGIN_DIR_LABEL_PREFIX = "Plugin Dir: "
    def initGUI(self):
        self.twiHeadersList = []
        self.twiHash = {}
        self.setWindowTitle("Plugin Manager")
        # Plugin Table
        layout = QtGui.QVBoxLayout()
        self.setLayout(layout)
        self.pluginDirLabel = QtGui.QLabel(self.PLUGIN_DIR_LABEL_PREFIX)
        layout.addWidget(self.pluginDirLabel)
        self.panelSplit = QtGui.QSplitter()
        self.panelSplit.setOrientation(Qt.Qt.Vertical)
        layout.addWidget(self.panelSplit)
        pluginsGroupBox = QtGui.QGroupBox("Plugins")
        self.panelSplit.addWidget(pluginsGroupBox)
        pluginsLayout = QtGui.QVBoxLayout()
        pluginsGroupBox.setLayout(pluginsLayout)
        self.tableSplit = QtGui.QSplitter()
        self.tableSplit.setOrientation(Qt.Qt.Horizontal)
        pluginsLayout.addWidget(self.tableSplit)
        self.pluginTable = QtGui.QTableWidget()
        self.tableSplit.addWidget(self.pluginTable)
        self.setTableHeaders(self.pluginTable, self.PLUGIN_COLUMN_NAMES)
        self.pluginTable.cellClicked.connect(self.pluginTableCellClicked)
        self.pluginTable.cellDoubleClicked.connect(self.pluginTableCellDoubleClicked)
        self.finalizeTable(self.pluginTable)
        self.metaDataTable = QtGui.QTableWidget()
        self.tableSplit.addWidget(self.metaDataTable)
        self.setTableHeaders(self.metaDataTable, ["Key","Value"])
        self.finalizeTable(self.metaDataTable)
        # Action
        actionLayout = QtGui.QHBoxLayout()
        pluginsLayout.addLayout(actionLayout)
        refreshButton = QtGui.QPushButton("Refresh")
        refreshButton.clicked.connect(self.refreshButtonClicked)
        actionLayout.addWidget(refreshButton)
        updateAllButton = QtGui.QPushButton("Update All")
        updateAllButton.clicked.connect(self.updateAllButtonClicked)
        actionLayout.addWidget(updateAllButton)
        updateSelectedButton = QtGui.QPushButton("Update Selected")
        updateSelectedButton.clicked.connect(self.updateSelectedButtonClicked)
        actionLayout.addWidget(updateSelectedButton)
        # Log
        logGroupBox = QtGui.QGroupBox("Log")
        self.panelSplit.addWidget(logGroupBox)
        logLayout = QtGui.QVBoxLayout()
        logGroupBox.setLayout(logLayout)
        self.logTable = QtGui.QTableWidget()
        logLayout.addWidget(self.logTable)
        self.setTableHeaders(self.logTable, ["Date/Time", "Title", "Text"])
        self.finalizeTable(self.logTable)
        # Options
        optionsGroupBox = QtGui.QGroupBox("Options")
        self.panelSplit.addWidget(optionsGroupBox)
        optionsLayout = QtGui.QVBoxLayout()
        optionsGroupBox.setLayout(optionsLayout)
        urlLayout = QtGui.QHBoxLayout()
        optionsLayout.addLayout(urlLayout)
        urlLayout.addWidget(QtGui.QLabel("Repo List URL"))
        self.repoLineEdit = QtGui.QLineEdit()
        urlLayout.addWidget(self.repoLineEdit)
        mostOptionsLayout = QtGui.QHBoxLayout()
        optionsLayout.addLayout(mostOptionsLayout)
        self.quietModeCheckBox = QtGui.QCheckBox("Quiet")
        mostOptionsLayout.addWidget(self.quietModeCheckBox)
        self.offlineModeCheckBox = QtGui.QCheckBox("Offline")
        self.offlineModeCheckBox.stateChanged.connect(self.offlineModeCheckBoxChanged)
        mostOptionsLayout.addWidget(self.offlineModeCheckBox)
        self.overwriteSameCheckBox = QtGui.QCheckBox("Overwrite Same Version")
        mostOptionsLayout.addWidget(self.overwriteSameCheckBox)
        pluginDirButton = QtGui.QPushButton("Plugin Dir")
        pluginDirButton.clicked.connect(self.pluginDirButtonClicked)
        mostOptionsLayout.addWidget(pluginDirButton)
        self.clearLogButton = QtGui.QPushButton("Clear Log")
        self.clearLogButton.clicked.connect(self.clearLogButtonClicked)
        mostOptionsLayout.addWidget(self.clearLogButton)
        self.saveLogButton = QtGui.QPushButton("Save Log")
        self.saveLogButton.clicked.connect(self.saveLogButtonClicked)
        mostOptionsLayout.addWidget(self.saveLogButton)
        defaultsButton = QtGui.QPushButton("Defaults")
        defaultsButton.clicked.connect(self.defaultsButtonClicked)
        mostOptionsLayout.addWidget(defaultsButton)
        helpButton = QtGui.QPushButton("Help")
        helpButton.clicked.connect(self.helpButtonClicked)
        mostOptionsLayout.addWidget(helpButton)
        # Show
        self.setWindowFlags(Qt.Qt.Window)
        self.show()
        Qt.QApplication.processEvents()
        self.resize(self.size)
        return

    def __init__(self, parent=KnossosModule.knossos_global_mainwindow):
        super(main_class, self).__init__(parent)
        exec(KnossosModule.scripting.getInstanceInContainerStr(__name__) + " = self")
        self.guiDone = False
        self.initGUI()
        self.loadSettings()
        self.guiDone = True
        self.initLogic()
        return

    def finalizeTable(self, table):
        table.horizontalHeader().setStretchLastSection(True)
        self.resizeTable(table)
        table.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        table.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        return

    def initLogic(self):
        self.log("start", "plugin manager started")
        self.refreshButtonClicked()
        return

    def helpButtonClicked(self):
        instructionsWidget = QtGui.QDialog(self)
        instructionsWidget.setModal(True)
        instructionsWidget.setWindowTitle("Plugin Manager Help")
        instructionsLayout = QtGui.QVBoxLayout()
        instructionsWidget.setLayout(instructionsLayout)
        instructionsTextEdit = QtGui.QTextEdit()
        instructionsTextEdit.setAlignment(Qt.Qt.AlignJustify)
        instructionsTextEdit.setReadOnly(True)
        instructionsLayout.addWidget(instructionsTextEdit)
        instructionsWidget.resize(750,500)
        instructionsTextEdit.setPlainText(
        #QtGui.QMessageBox.information(0, "Help", \
                    """
Welcome to Plugin Manager!

Introduction:
- Plugin manager is the client front-end for a enumeration of local plugins and retrieval of remote plugins
- Remote plugins on a server, in a location specified by a URL
- All plugin URLs of an organization are listed in a repository index file on a server, in a location indicated by a repo URL
- Repository URLs of different organization are listed in a repository list on a knossos server, in a location indicated by the repo list URL
- In addition to remote plugins, local plugins can be developed independently of any repository

Operation:
- The plugin GUI includes a plugin table, metadata table, log table and options
- Upon startup of knossos, a default plugin directory is used. This can be reconfigured by "Plugin Dir"
- The default knossos repo list URL is used. Chnage it by editing "Repo List URL"
- Press "Refresh" to enumerate local plugins and retrieve remote repository indices
- Check "Offline" to avoid remote listing: local plugins would be enumerated, no remote plugins would be listed, hence no updates would be available
- Press "Update All" to to retrieve all updated remote plugins, or "Update Selected" for those plugins selected in the table
- Python files (.py extension) are defined as Knossos plugins by metadata lines of format
#KNOSSOS_PLUGIN\\t<Key>\\t<Value>
- Mandatory metadata fields are Version and Description. Absence of any disqualifies the file as plugin
- All metadata fields are shown at the metadata table when a plugin is clicked
- Double-click a plugin in the table to open the local version, if available
- Press "Defaults" to restore default plugin dir, repo list URL and other options
- Repo index and repo list file format is a header line of <First Field Name>\\t<Second Field Name>\\t<Third Field Name> etc.,
while the rest of the files is lines of <First Field>\\t<Second Field>\\t<Third Field> etc.
Mandatory fields for repo index - URL,Version,Description ; for repo list - Name,URL

Notes:
- The plugin directory requires refreshing every time a local or remote plugin is added or removed,
in order for knossos to correctly reflect available local and remote plugins in the plugin menu
- Version numbers are compared from major to minor, using . as hierarchy separator: 3 > 2.2 > 2.1.9 > 1.10 > 1.9
Each version component has to be the pythonic string representation of a number. For example, 1.02 is not supported, use 1.2 instead.
- Check "Overwrite Same Version" to overwrite local with remote even when version number is identical
- Check "Quiet" to skip errors and make default choices for questions
- Press "Save Log" to save the current log into a tab-separated text file
- Press "Clear Log" to clear the log
                    """)
        instructionsWidget.show()
        return
    
    def setTableHeaders(self, table, columnNames):
        columnNum = len(columnNames)
        table.setColumnCount(columnNum)
        for i in xrange(columnNum):
            twi = QtGui.QTableWidgetItem(columnNames[i])
            table.setHorizontalHeaderItem(i, twi)
            self.twiHeadersList.append(twi)
        return

    def clearTable(self, table):
        table.clearContents()
        del self.twiHash.setdefault(table,[])[:]
        table.setRowCount(0)
        return

    def resizeTable(self, table):
        table.resizeColumnsToContents()
        table.resizeRowsToContents()
        return

    def addTableRow(self, table, columnTexts, atEnd = False):
        rowIndex = 0
        if atEnd:
            rowIndex = table.rowCount
        table.insertRow(rowIndex)
        for i in xrange(len(columnTexts)):
            twi = QtGui.QTableWidgetItem(columnTexts[i])
            twi.setFlags(twi.flags() & (~Qt.Qt.ItemIsEditable))
            self.twiHash.setdefault(table,[]).append(twi)
            table.setItem(rowIndex, i, twi)
        self.resizeTable(table)
        return

    def clearLogButtonClicked(self):
        self.clearTable(self.logTable)
        return

    def saveLogButtonClicked(self):
        fn = QtGui.QFileDialog.getSaveFileName(self, "Save File", "", ".txt(*.txt)")
        if (fn == ""):
            return
        table = self.logTable
        rl = []
        for row in range(table.rowCount):
            cl = []
            for col in xrange(table.columnCount):
                cl.append(str(table.item(row,col).text()))
            rl.append("\t".join(cl))
        open(fn,"wt").write("\n".join(rl))
        return

    def log(self, title, text):
        self.addTableRow(self.logTable, [time.strftime("%d/%m/%Y %H:%M:%S", time.localtime()), title, text])
        return

    def showMessage(self, title, text):
        self.log(title, text)
        return

    def showError(self, title, text):
        self.showMessage(title, text)
        if not self.quietModeCheckBox.checked:
            box = QtGui.QMessageBox()
            box.setWindowTitle(title)
            box.setText(text)
            box.exec_()
        return

    def getUrl(self, url):
        try:
            return urllib2.urlopen(url).read()
        except:
            self.showError("network", "error fetching URL: " + url)
            raise

    def splitStrip(self, s):
        elems = s.split("\n")
        return map(lambda x: x.rstrip("\r\n"), elems)
    
    def parseMetadata(self, content):
        metadata = {}
        lines = self.splitStrip(content)
        for line in lines:
            elems = line.split("\t")
            if len(elems) < 3:
                continue
            if elems[0] <> self.METADATA_KEYWORD:
                continue
            metadata[elems[1]] = elems[2]
        return metadata

    def parseHeaderedList(self, s, key):
        lines = self.splitStrip(s)
        sep = "\t"
        fields = lines[0].split(sep)
        h = {}
        for line in lines[1:]:
            cur_h = {}
            elems = line.split(sep)
            for i in xrange(len(elems)):
                cur_h[fields[i]] = elems[i]
            keyVal = cur_h[key]
            h[keyVal] = cur_h
        return h

    def parseVersion(self, s):
        try:
            l = []
            for comp in s.split("."):
                compInt = int(comp)
                compStr = str(compInt)
                if compStr <> comp:
                    raise
                l.append(compInt)
            return l
        except:
            self.showError("version","wrong version format: " + s)
            raise

    def compareVersions(self, h):
        local, remote = h["Local"], h["Remote"]
        if remote == "":
            return False
        if local == "":
            return True
        localList,remoteList = map(self.parseVersion, [local,remote])
        if localList < remoteList:
            return True
        return (localList == remoteList) and self.overwriteSameCheckBox.isChecked()

    def newPlugin(self,name):
        h = {}
        for field in self.PLUGIN_COLUMN_NAMES + ["Filename"]:
            h[field] = ""
        h["Name"] = name
        h["Metadata"] = {}
        return h

    def getLocalRepo(self):
        return "\n".join(["URL"] + [("file:" + urllib.pathname2url(fn)) for fn in glob.glob(os.path.join(self.pluginDir, "*.py"))])

    def validateFields(self,fields):
        necessary = ["Version","Description"]
        for field in necessary:
            if not field in fields:
                self.showError("field","missing obligatory field: " + field)
                raise
        self.parseVersion(fields["Version"])
        return

    def listRepo(self, repoName, repoContent, isLocalRepo):
        self.showMessage("list","listing repo " + repoName)
        try:
            h = self.parseHeaderedList(repoContent, "URL")
        except:
            self.showError("list", "error parsing field list of repo: " + repoName)
            raise
        for url in h:
            try:
                fn = os.path.basename(urlparse.urlsplit(url).path)
                name,ext = os.path.splitext(fn)
                curRepo = self.plugins.get(name,{"Repo":self.LOCAL_REPO})["Repo"]
                if curRepo <> self.LOCAL_REPO:
                    self.showError("list","plugin " + name + " already present in repo " + curRepo)
                    raise
                cur_h = self.plugins.get(name, self.newPlugin(name))
                cur_h["Filename"] = fn
                cur_h["Extension"] = ext
                cur_h["Repo"] = repoName
                fields = self.parseMetadata(self.getUrl(url)) if isLocalRepo else h[url]
                self.validateFields(fields)
                cur_h["Local" if isLocalRepo else "Remote"] = fields["Version"]
                if isLocalRepo:
                    cur_h["Metadata"] = fields
                else:
                    cur_h["URL"] = url
                cur_h["Description"] = fields["Description"]
                self.plugins[name] = cur_h
            except:
                self.showError("list", "error listing repo " + repoName + " , URL: " + url)
                continue
        return

    def listRepos(self, repoListURL):
        try:
            repoListContent = self.getUrl(repoListURL)
        except:
            self.showError("repo", "error getting repo list from URL " + repoListURL)
            return
        try:
            h = self.parseHeaderedList(repoListContent, "Name")
        except:
            self.showError("repo", "error parsing repo list at URL " + repoListURL)
            return
        for name in h:
            url = h[name].get("URL","")
            if url == "":
                self.showError("repo", "No URL for repo " + name)
                continue
            try:
                repoContent = self.getUrl(url)
            except:
                self.showError("repo", "error retrieving repo " + name + " , from URL " + url)
                continue
            try:
                self.listRepo(name, repoContent, False)
            except:
                self.showError("repo", "error parsing repo " + name + " , at URL " + url)
                continue
        return

    def loadSetting(self,key,default):
        settings = Qt.QSettings()
        settings.beginGroup(self.SETTING_GROUP_NAME)
        val = settings.value(key)
        settings.endGroup()
        if (val == None) or (str(val) == ""):
            val = default
            self.saveSetting(key,val)
        return str(val)

    def saveSetting(self,key,val):
        settings = Qt.QSettings()
        settings.beginGroup(self.SETTING_GROUP_NAME)
        settings.setValue(key,str(val))
        settings.endGroup()
        return

    def saveOverwriteSame(self):
        self.saveSetting(self.SETTING_NAMES["SETTING_OVERWRITE_SAME_NAME"], int(self.overwriteSameCheckBox.isChecked()))
        return
    
    def loadOverwriteSame(self):
        v = self.loadSetting(self.SETTING_NAMES["SETTING_OVERWRITE_SAME_NAME"], int(False))
        self.overwriteSameCheckBox.setChecked(bool(int(v)))
        return
    
    def saveQuietMode(self):
        self.saveSetting(self.SETTING_NAMES["SETTING_QUIET_MODE_NAME"], int(self.quietModeCheckBox.isChecked()))
        return
    
    def loadQuietMode(self):
        v = self.loadSetting(self.SETTING_NAMES["SETTING_QUIET_MODE_NAME"], int(True))
        self.quietModeCheckBox.setChecked(bool(int(v)))
        return

    def saveOfflineMode(self):
        self.saveSetting(self.SETTING_NAMES["SETTING_OFFLINE_MODE_NAME"], int(self.offlineModeCheckBox.isChecked()))
        return
    
    def loadOfflineMode(self):
        v = self.loadSetting(self.SETTING_NAMES["SETTING_OFFLINE_MODE_NAME"], int(False))
        self.offlineModeCheckBox.setChecked(bool(int(v)))
        return

    def saveGeometrySetting(self):
        self.saveSetting(self.SETTING_NAMES["SETTING_GEOMETRY"], self.saveGeometry().toHex())
        return

    def loadGeometrySetting(self):
        v = self.loadSetting(self.SETTING_NAMES["SETTING_GEOMETRY"], self.saveGeometry().toHex())
        self.restoreGeometry(QtCore.QByteArray.fromHex(v))
        return

    def splitter2sizestr(self,splitter):
        return ";".join(map(str,splitter.sizes()))
    
    def sizestr2splitter(self,sizestr):
        return tuple(map(long,sizestr.split(";")))
    
    def savePluginTableSplitSize(self):
        self.saveSetting(self.SETTING_NAMES["SETTING_PLUGIN_TABLE_SPLIT_SIZES_NAME"], self.splitter2sizestr(self.tableSplit))
        return
    
    def loadPluginTableSplitSize(self):
        v = self.loadSetting(self.SETTING_NAMES["SETTING_PLUGIN_TABLE_SPLIT_SIZES_NAME"], "300L;100L")
        self.tableSplit.setSizes(self.sizestr2splitter(v))
        return
    
    def savePanelSplitSize(self):
        self.saveSetting(self.SETTING_NAMES["SETTING_PANEL_SPLIT_SIZES_NAME"], self.splitter2sizestr(self.panelSplit))
        return
    
    def loadPanelSplitSize(self):
        v = self.loadSetting(self.SETTING_NAMES["SETTING_PANEL_SPLIT_SIZES_NAME"], "400L;200L;100L")
        self.panelSplit.setSizes(self.sizestr2splitter(v))
        return

    def saveRepoListURL(self):
        self.saveSetting(self.SETTING_NAMES["SETTING_REPO_LIST_URL_NAME"], self.repoLineEdit.text)
        return

    def loadRepoListURL(self):
        v = self.loadSetting(self.SETTING_NAMES["SETTING_REPO_LIST_URL_NAME"], self.DEFAULT_REPO_LIST_URL)
        self.repoLineEdit.setText(v)
        return

    def savePluginDir(self):
        KnossosModule.scripting.setPluginDir(self.pluginDir)
        return
    
    def loadPluginDir(self):
        self.pluginDir = str(KnossosModule.scripting.getPluginDir())
        self.pluginDirLabel.setText(self.PLUGIN_DIR_LABEL_PREFIX + self.pluginDir)
        return

    def saveSettings(self):
        self.saveGeometrySetting()
        self.savePanelSplitSize()
        self.savePluginTableSplitSize()
        self.saveOverwriteSame()
        self.saveOfflineMode()
        self.saveQuietMode()
        self.saveRepoListURL()
        return
    
    def loadSettings(self):
        self.loadOverwriteSame()
        self.loadQuietMode()
        self.loadOfflineMode()
        self.loadPluginTableSplitSize()
        self.loadPanelSplitSize()
        self.loadRepoListURL()
        self.loadPluginDir()
        self.loadGeometrySetting()
        return

    def updatePlugin(self, name):
        h = self.plugins[name]
        if not self.compareVersions(h):
            self.showError("update","no need to update plugin " + name)
            return
        content = self.getUrl(h["URL"])
        filepath = os.path.join(self.pluginDir,h["Filename"])
        open(filepath,"wb").write(content)
        self.showMessage("update","updated plugin " + name)
        return

    def updatePlugins(self, names):
        if names == []:
            self.showMessage("update","no plugins to update")
            return
        self.showMessage("update","updating plugins...")
        for name in names:
            try:
                self.updatePlugin(name)
            except:
                self.showError("update","error while updating plugin " + name)
                pass
        self.showMessage("update","update finished")
        self.refreshButtonClicked()
        return

    def getSelectedPlugins(self):
        l = []
        t = self.pluginTable
        for row in xrange(t.rowCount):
            ti = t.item(row, self.PLUGIN_NAME_COLUMN_INDEX)
            if ti.isSelected():
                l.append(ti.text())
        return l

    def updateSelectedButtonClicked(self):
        self.updatePlugins(self.getSelectedPlugins())
        return

    def updateAllButtonClicked(self):
        self.updatePlugins(self.plugins.keys())
        return

    def refreshPluginNames(self):
        local_names = filter(self.isLocal, self.plugins.keys())
        local_names.sort()
        KnossosModule.scripting.setPluginNames(";".join(local_names))
        return

    def isLocal(self,name):
        return self.plugins[name]["Local"] <> ""

    def refreshButtonClicked(self):
        self.showMessage("refresh","refreshing plugin list...")
        self.resetPlugins()
        self.listRepo(self.LOCAL_REPO,self.getLocalRepo(),True)
        if not self.offlineModeCheckBox.isChecked():
            self.listRepos(self.repoLineEdit.text)
        self.refreshPluginNames()
        self.refreshPluginsTable()
        self.showMessage("refresh","refresh finished")
        return

    def resetPlugins(self):
        self.showMessage("reset", "resetting plugin list")
        self.clearTable(self.pluginTable)
        self.clearTable(self.metaDataTable)
        self.plugins = {}
        self.refreshPluginNames()
        return

    def changePluginDir(self, d):
        self.pluginDir = d
        self.savePluginDir()
        self.loadPluginDir()
        self.refreshButtonClicked()
        return

    def pluginDirButtonClicked(self):
        browse_dir = QtGui.QFileDialog.getExistingDirectory(self, "Select Plugin Directory", self.pluginDir, QtGui.QFileDialog.ShowDirsOnly)
        if "" <> browse_dir:
            self.showMessage("dir", "selected plugin dir " + browse_dir)
            self.changePluginDir(browse_dir)
        return

    def defaultsButtonClicked(self):
        self.showMessage("default", "restoring defaults")
        for setting in self.SETTING_NAMES.values():
            self.saveSetting(setting,"")
        self.loadSettings()
        self.changePluginDir(str(KnossosModule.scripting.getDefaultPluginDir()))
        return

    def offlineModeCheckBoxChanged(self):
        if (not self.guiDone):
            return
        self.refreshButtonClicked()
        return

    def refreshPluginsTable(self):
        table = self.pluginTable
        table.setVisible(False)
        try:
            names = self.plugins.keys()
            names.sort()
            for name in names:
                cur_h = self.plugins[name]
                self.addTableRow(table,map(lambda col_name: cur_h[col_name], self.PLUGIN_COLUMN_NAMES), True)
        except:
            pass
        table.setVisible(True)
        self.resizeTable(table)
        return

    def pluginTableCellClicked(self, row, col):
        name = self.pluginTable.item(row, self.PLUGIN_NAME_COLUMN_INDEX).text()
        table = self.metaDataTable
        self.clearTable(table)
        metadata = self.plugins[name]["Metadata"]
        metadatas = metadata.items()
        metadatas.sort()
        for [key, val] in metadatas:
            self.addTableRow(table, [key, val], True)
        self.resizeTable(table)
        return
    
    def pluginTableCellDoubleClicked(self, row, col):
        name = self.pluginTable.item(row, self.PLUGIN_NAME_COLUMN_INDEX).text()
        self.showMessage("open", "opening plugin " + name)
        if not self.isLocal(name):
            self.showError("open", "no local version of plugin " + name)
            return
        KnossosModule.scripting.openPlugin(name)
        return

    def closeEvent(self, event):
        if (not self.guiDone):
            return
        self.saveSettings()
        return

    pass
