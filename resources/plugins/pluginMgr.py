from PythonQt import QtGui, Qt
import KnossosModule
import urllib, urllib2, os, urlparse, glob, time, inspect, tempfile

#KNOSSOS_PLUGIN Name PluginMgr
#KNOSSOS_PLUGIN Version 1
#KNOSSOS_PLUGIN Description Plugin manager is a plugin manager

class pluginMgr(QtGui.QWidget):
    DEFAULT_URL_STR = "http://knossos-project.github.io/knossos-plugins/knossos_plugins.txt"
    ERROR_PROCESSING_REPOSITORY_STR = "Error processing repository"
    REMOTE_REPO_INDEX_ERROR_STR = "Error reading remote repository index"
    REPOSITORY_LISTED_STR = "Repository Listed"
    LOCAL_REPO_STR = "Local Plugin Directory"
    LOCAL_DIR_DEFAULT_STR = "knos_plg"
    LOCAL_DIR_NOT_EXISTS_STR = "Local directory not exists! Create?"
    CANNOT_CREATE_DIR_STR = "Cannot create directory:\n"
    METADATA_NAME_STR = "Name"
    METADATA_VERSION_STR = "Version"
    METADATA_DESCRIPTION_STR = "Description"
    COLUMN_FILENAME_STR = "Filename"
    PLUGIN_DICT_FILENAME_STR = "Filename"
    PLUGIN_DICT_CONTENT_STR = "Content"
    PLUGIN_DICT_METADATA_STR = "Metadata"
    PLUGIN_DICT_URL_STR = "URL"
    REMOTE_TO_LOCAL_STR = "Remote->Local"
    LIST_LOCAL_BEFORE_REMOTE_TO_LOCAL_STR = "List selected local repository before Remote->Local"
    LIST_REMOTE_BEFORE_REMOTE_TO_LOCAL_STR = "List selected remote repository before Remote->Local"
    LOCAL_NON_LOWER_SKIPPED_STR = "Skipping plugin %s: Remote version %s not higher than local version %s"
    LOCAL_RENAME_ERROR = "Skipping plugin %s: Cannot rename local filename %s to remote filename %s"
    LOCAL_EXISTS_ERROR = "Skipping plugin %s: filename %s already locally used by another plugin %s"
    MANDATORY_METADATA_FIELDS = [METADATA_NAME_STR, METADATA_VERSION_STR, METADATA_DESCRIPTION_STR]
    PLUGIN_LIST_COLUMNS = [COLUMN_FILENAME_STR] + MANDATORY_METADATA_FIELDS
    PLUGIN_NAME_COLUMN_INDEX = PLUGIN_LIST_COLUMNS.index(METADATA_NAME_STR)
    PLUGIN_FILENAME_COLUMN_INDEX = PLUGIN_LIST_COLUMNS.index(COLUMN_FILENAME_STR)
    METADATA_TABLE_FIELD_STR = "Field"
    METADATA_TABLE_CONTENT_STR = "Content"
    PLUGIN_METADATA_COLUMNS = [METADATA_TABLE_FIELD_STR, METADATA_TABLE_CONTENT_STR]
    LOCAL_REPOSITORY_STR = "Local Repository"
    REMOTE_REPOSITORY_STR = "Remote Repository"
    PROCESSOR_STR = "Processor"

    def initGUI(self):
        self.twiHeadersList = []
        self.twiHash = {}
        # General
        self.setWindowTitle("Plugin Manager")
        layout = QtGui.QVBoxLayout()
        self.setLayout(layout)
        # Local Dir 
        localDirGroupBox = QtGui.QGroupBox("Local Repository")
        layout.addWidget(localDirGroupBox)
        localDirLayout = QtGui.QVBoxLayout()
        localDirGroupBox.setLayout(localDirLayout)
        localDirSelectLayout = QtGui.QHBoxLayout()
        localDirLayout.addLayout(localDirSelectLayout)
        localDirLabel = QtGui.QLabel("Path")
        localDirSelectLayout.addWidget(localDirLabel)
        self.localDirEdit = QtGui.QLineEdit()
        localDirSelectLayout.addWidget(self.localDirEdit)
        localDirBrowseButton = QtGui.QPushButton("Browse...")
        localDirBrowseButton.clicked.connect(self.localDirBrowseButtonClicked)
        localDirSelectLayout.addWidget(localDirBrowseButton)
        localDirDefaultButton = QtGui.QPushButton("Default")
        localDirDefaultButton.clicked.connect(self.localDirDefaultButtonClicked)
        localDirSelectLayout.addWidget(localDirDefaultButton)
        localDirListButton = QtGui.QPushButton("List")
        localDirListButton.clicked.connect(self.localDirListButtonClicked)
        localDirSelectLayout.addWidget(localDirListButton)
        localDirSplitter = QtGui.QSplitter()
        localDirSplitter.setOrientation(Qt.Qt.Horizontal)
        localDirLayout.addWidget(localDirSplitter)
        localDirListFrame = QtGui.QFrame()
        localDirSplitter.addWidget(localDirListFrame)
        localDirListLayout = QtGui.QVBoxLayout()
        localDirListFrame.setLayout(localDirListLayout)
        localDirListLayout.addWidget(QtGui.QLabel("Plugins"))
        self.localDirPluginTable = QtGui.QTableWidget()
        localDirListLayout.addWidget(self.localDirPluginTable)
        self.setTableHeaders(self.localDirPluginTable, self.PLUGIN_LIST_COLUMNS)
        self.localDirPluginTable.cellClicked.connect(self.localDirTableCellClicked)
        self.localDirPluginTable.cellDoubleClicked.connect(self.localDirTableCellDoubleClicked)
        self.finalizeTable(self.localDirPluginTable)
        localDirMetadataFrame = QtGui.QFrame()
        localDirSplitter.addWidget(localDirMetadataFrame)
        localDirMetadataLayout = QtGui.QVBoxLayout()
        localDirMetadataFrame.setLayout(localDirMetadataLayout)
        localDirMetadataLayout.addWidget(QtGui.QLabel("Plugin Metadata"))
        self.localDirMetaDataTable = QtGui.QTableWidget()
        localDirMetadataLayout.addWidget(self.localDirMetaDataTable)
        self.setTableHeaders(self.localDirMetaDataTable, self.PLUGIN_METADATA_COLUMNS)
        self.finalizeTable(self.localDirMetaDataTable)
        # Sync
        remoteToLocalButton = QtGui.QPushButton("Remote->Local")
        remoteToLocalButton.clicked.connect(self.remoteToLocalButtonClicked)
        layout.addWidget(remoteToLocalButton)
        # Repository
        repoGroupBox = QtGui.QGroupBox("Remote Repository")
        repoLayout = QtGui.QVBoxLayout()
        repoGroupBox.setLayout(repoLayout)
        layout.addWidget(repoGroupBox)
        repoSelectLayout = QtGui.QHBoxLayout()
        repoLayout.addLayout(repoSelectLayout)
        repoSelectLayout.addWidget(QtGui.QLabel("URL"))
        self.repoUrlEdit = QtGui.QLineEdit()
        repoSelectLayout.addWidget(self.repoUrlEdit)
        repoDefaultButton = QtGui.QPushButton("Default")
        repoDefaultButton.clicked.connect(self.repoDefaultButtonClicked)
        repoSelectLayout.addWidget(repoDefaultButton)
        repoListButton = QtGui.QPushButton("List")
        repoListButton.clicked.connect(self.repoListButtonClicked)
        repoSelectLayout.addWidget(repoListButton)
        repoSplitter = QtGui.QSplitter()
        repoSplitter.setOrientation(Qt.Qt.Horizontal)
        repoLayout.addWidget(repoSplitter)
        repoListFrame = QtGui.QFrame()
        repoSplitter.addWidget(repoListFrame)
        repoListLayout = QtGui.QVBoxLayout()
        repoListFrame.setLayout(repoListLayout)
        repoListLayout.addWidget(QtGui.QLabel("Plugins"))
        self.repoPluginTable = QtGui.QTableWidget()
        repoListLayout.addWidget(self.repoPluginTable)
        self.setTableHeaders(self.repoPluginTable, self.PLUGIN_LIST_COLUMNS)
        self.repoPluginTable.cellClicked.connect(self.repoTableCellClicked)
        self.repoPluginTable.cellDoubleClicked.connect(self.repoTableCellDoubleClicked)
        self.finalizeTable(self.repoPluginTable)
        repoMetadataFrame = QtGui.QFrame()
        repoSplitter.addWidget(repoMetadataFrame)
        repoMetadataLayout = QtGui.QVBoxLayout()
        repoMetadataFrame.setLayout(repoMetadataLayout)
        repoMetadataLayout.addWidget(QtGui.QLabel("Plugin Metadata"))
        self.repoMetaDataTable = QtGui.QTableWidget()
        repoMetadataLayout.addWidget(self.repoMetaDataTable)
        self.setTableHeaders(self.repoMetaDataTable, self.PLUGIN_METADATA_COLUMNS)
        self.finalizeTable(self.repoMetaDataTable)
        # Options
        optionsGroupBox = QtGui.QGroupBox("Options")
        layout.addWidget(optionsGroupBox)
        optionsLayout = QtGui.QHBoxLayout()
        optionsGroupBox.setLayout(optionsLayout)
        self.quietModeCheckBox = QtGui.QCheckBox("Quiet Mode")
        optionsLayout.addWidget(self.quietModeCheckBox)
        self.quietModeCheckBox.setChecked(False)
        self.quietModeCheckBox.setToolTip("- Silence errors\n- Skip questions using default answers")
        self.autoOverwriteCheckBox = QtGui.QCheckBox("Force Overwrite Local")
        optionsLayout.addWidget(self.autoOverwriteCheckBox)
        self.autoOverwriteCheckBox.setChecked(False)
        self.autoOverwriteCheckBox.setToolTip("Overwrite local plugin even when remote version is not higher")
        self.clearLogButton = QtGui.QPushButton("Clear Log")
        optionsLayout.addWidget(self.clearLogButton)
        self.clearLogButton.clicked.connect(self.clearLogButtonClicked)
        # Log
        logGroupBox = QtGui.QGroupBox("Log")
        layout.addWidget(logGroupBox)
        logLayout = QtGui.QVBoxLayout()
        logGroupBox.setLayout(logLayout)
        self.logTable = QtGui.QTableWidget()
        logLayout.addWidget(self.logTable)
        self.setTableHeaders(self.logTable, ["Date/Time", "Title", "Text"])
        self.finalizeTable(self.logTable)
        # Show
        self.show()
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

    def initLogic(self):
        self.repoPlugins = {}
        self.localPlugins = {}
        self.localPluginsFilenameToName = {}
        self.localDir = None
        self.repoUrl = None
        self.getStarted()
        self.log("Info", "Plugin manager started")
        self.useDefaults()
        return

    def getStarted(self):
        QtGui.QMessageBox.information(0, "Getting Started", \
                    """
Welcome to Plugin Manager!

Introduction:
- Plugin manager is the client front-end for a distributed Knossos plugin version control
- Plugins are placed remotely on different servers and indicated by URLs
- The list of plugin URLs is maintained in a single repository file, also indicated by a URL
- By accessing the plugin index file you can retrieve remote plugins to a local directory

Operation:
- The plugin shows two group boxes - a local repository above and a remote repository below
- An options group box and log are shown at the bottom
- Upon startup, a default local directory would be created
- Alternatively you can Browse to select a local folder, or go back to Default
- Once a local directory is selected, List it to enumerate local plugins
- Enter a URL for a remote repository, or use the default one
- Then List to enumerate remote plugins in repository
- Python files (.py extension) are defined as Knossos plugin files by metadata lines of format
#KNOSSOS_PLUGIN <Field Name (without whitespaces)> <Content>
- Mandatory metadata fields are Name, Version, Description. Absence of any disqualifies the file as plugin
- All metadata fields are shown to the right when a plugin is clicked
- Double-click a plugin to show its content, execute using python evaluation
- Press Remote->Local to update local plugins from remote repository

Notes:
- Specified local directory is only in effect once listed. Don't forget to list if you change it
- Plugins are uniquely identified by internal name, not by filename. An local file is renamed during update when remote filename changes
- Version numbers are compared lexicographically: 3 > 2.1 > 2
- Update only overwrites older local versions with newer remote versions by default
- To force overwriting same/newer local versions, check "Force Overwrite Local"
- Check "Quiet Mode" to skip errors and make default choices for questions
- Log includes all run-time information and errors. Clear Log clears it
                    """)
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

    def localDirDefaultButtonClicked(self):
        defaultDir = os.path.expanduser(os.path.join("~",self.LOCAL_DIR_DEFAULT_STR))
        self.localDirValidate(defaultDir)
        self.localDirEdit.setText(defaultDir)
        return
    
    def localDirBrowseButtonClicked(self):
        browse_dir = QtGui.QFileDialog.getExistingDirectory()
        if "" <> browse_dir:
            if self.localDirValidate(browse_dir):
                self.localDirEdit.setText(browse_dir)
        return
    
    def localDirValidate(self, dirText):
        exists = os.path.exists(dirText)
        if not exists:
            choice = QtGui.QMessageBox.Ok
            if not self.quietModeCheckBox.checked:
                choice = QtGui.QMessageBox.question(0, self.LOCAL_REPO_STR, self.LOCAL_DIR_NOT_EXISTS_STR, QtGui.QMessageBox.Ok | QtGui.QMessageBox.Cancel)
            if QtGui.QMessageBox.Ok == choice:
                try:
                    os.makedirs(dirText)
                    exists = True
                except:
                    self.showMessage(self.LOCAL_REPO_STR, self.CANNOT_CREATE_DIR_STR + dirText)
                    pass
        return exists

    def useDefaults(self):
        self.localDirDefaultButtonClicked()
        self.localDirListButtonClicked()
        self.repoDefaultButtonClicked()
        self.repoListButtonClicked()
        return

    def log(self, title, text):
        self.addTableRow(self.logTable, [time.strftime("%d/%m/%Y %H:%M:%S", time.localtime()), title, text])
        return

    def showMessage(self, title, text):
        self.log(title, text)
        if not self.quietModeCheckBox.checked:
            box = QtGui.QMessageBox()
            box.setWindowTitle(title)
            box.setText(text)
            box.exec_()
        return

    def getUrl(self, url):
        try:
            s = urllib2.urlopen(url).read()
        except:
            self.showMessage("Get URL Error", "Error reading URL:\n" + url)
            raise
        return s

    def parseMetadata(self, content):
        metadata = {}
        lines = content.split("\n")
        for line in lines:
            try:
                elems = line.split(None, 2)
            except:
                continue
            if len(elems) < 3:
                continue
            if elems[0] <> "#KNOSSOS_PLUGIN":
                continue
            metadata[elems[1]] = elems[2].rstrip("\r\n")
        return metadata
    
    def processUrls(self, urls):
        plugins = {}
        for url in urls:
            try:
                fn = os.path.basename(urlparse.urlsplit(url).path)
                content = self.getUrl(url)
                metadata = self.parseMetadata(content)
                missingMetadata = set(self.MANDATORY_METADATA_FIELDS) - set(metadata)
                if len(missingMetadata) > 0:
                    self.showMessage(self.PROCESSOR_STR, "Plugin\n" + url + "\nMissing mandatory metadata fields:\n\n" + "\n".join(missingMetadata))
                    raise
                pluginName = metadata[self.METADATA_NAME_STR]
                if pluginName in plugins:
                    existingUrl = plugins[pluginName][self.PLUGIN_DICT_URL_STR]
                    self.showMessage(self.PROCESSOR_STR, "Plugin\n" + url + "\nwith name\n" + pluginName + "\ncollides with same name of plugin\n" + existingUrl)
                    raise
                plugins[pluginName] = {self.PLUGIN_DICT_FILENAME_STR:fn, self.PLUGIN_DICT_CONTENT_STR:content, self.PLUGIN_DICT_METADATA_STR:metadata, self.PLUGIN_DICT_URL_STR:url}
            except:
                self.showMessage(self.PROCESSOR_STR, "Error processing plugin\n" + url)
                continue
        return plugins

    def listPlugins(self, plugins, pluginTable):
        pluginNames = plugins.keys()
        pluginNames.sort()
        for pluginName in pluginNames:
            metadata = plugins[pluginName][self.PLUGIN_DICT_METADATA_STR]
            filename = plugins[pluginName][self.PLUGIN_DICT_FILENAME_STR]
            self.addTableRow(pluginTable, [filename] + [metadata[field] for field in self.MANDATORY_METADATA_FIELDS])
        self.resizeTable(pluginTable)
        return

    def listMetadata(self, plugins, pluginTable, metadataTable, row):
        self.clearTable(metadataTable)
        pluginName = pluginTable.item(row, self.PLUGIN_NAME_COLUMN_INDEX).text()
        metadata = plugins[pluginName][self.PLUGIN_DICT_METADATA_STR]
        metadatas = metadata.items()
        metadatas.sort()
        for [key, val] in metadatas:
            self.addTableRow(metadataTable, [key, val], True)
        self.resizeTable(metadataTable)
        return

    def showPlugin(self, plugins, pluginTable, row):
        pluginName = pluginTable.item(row, self.PLUGIN_NAME_COLUMN_INDEX).text()
        content = plugins[pluginName][self.PLUGIN_DICT_CONTENT_STR]
        class showPluginWidget(QtGui.QWidget):
            def __init__(self, pluginName, content, parent = None):
                super(showPluginWidget, self).__init__(parent)
                self.pluginName = pluginName
                self.content = content
                self.setWindowTitle(pluginName)
                layout = QtGui.QVBoxLayout()
                self.setLayout(layout)
                textEdit = QtGui.QTextEdit()
                layout.addWidget(textEdit)
                textEdit.insertPlainText(content)
                textEdit.setReadOnly(True)
                textEdit.setTextInteractionFlags(Qt.Qt.TextSelectableByMouse | Qt.Qt.TextSelectableByKeyboard)
                runButton = QtGui.QPushButton("Run")
                layout.addWidget(runButton)
                runButton.clicked.connect(self.showPluginRunButtonClicked)
                return
            
            def showPluginRunButtonClicked(self):
                tempFile = tempfile.NamedTemporaryFile(delete=False)
                tempFile.write(self.content)
                tempFile.close()
                d = dict(locals(), **globals())
                execfile(tempFile.name, d, d)
                os.remove(tempFile.name)
                return
            
            pass
        
        spd = showPluginWidget(pluginName, content)
        spd.show()
        return
    
    def localDirListButtonClicked(self):
        self.clearTable(self.localDirPluginTable)
        self.clearTable(self.localDirMetaDataTable)

        localPluginUrls = []
        if not self.localDirValidate(self.localDirEdit.text):
            self.showMessage(self.LOCAL_REPOSITORY_STR, self.ERROR_PROCESSING_REPOSITORY_STR)
            return
        localPluginUrls = [("file:" + urllib.pathname2url(fn)) for fn in glob.glob(os.path.join(self.localDirEdit.text, "*.py"))]
        try:
            self.localPlugins = self.processUrls(localPluginUrls)
        except:
            self.showMessage(self.LOCAL_REPOSITORY_STR, self.ERROR_PROCESSING_REPOSITORY_STR)
            return
        for pluginName in self.localPlugins:
            filename = self.localPlugins[pluginName][self.PLUGIN_DICT_FILENAME_STR]
            self.localPluginsFilenameToName[filename] = pluginName
        self.listPlugins(self.localPlugins, self.localDirPluginTable)
        self.localDir = self.localDirEdit.text
        self.log(self.LOCAL_REPOSITORY_STR, self.REPOSITORY_LISTED_STR)
        return
    
    def localDirTableCellClicked(self, row, col):
        self.listMetadata(self.localPlugins, self.localDirPluginTable, self.localDirMetaDataTable, row)
        return
    
    def localDirTableCellDoubleClicked(self, row, col):
        self.showPlugin(self.localPlugins, self.localDirPluginTable, row)
        return
    
    def repoDefaultButtonClicked(self):
        self.repoUrlEdit.setText(self.DEFAULT_URL_STR)
        return

    def repoListButtonClicked(self):
        self.clearTable(self.repoPluginTable)
        self.clearTable(self.repoMetaDataTable)

        try:
            repoList = self.getUrl(self.repoUrlEdit.text)
        except:
            self.showMessage(self.REMOTE_REPOSITORY_STR, self.REMOTE_REPO_INDEX_ERROR_STR)
            return
        repoPluginUrls = repoList.split("\n")
        try:
            self.repoPlugins = self.processUrls(repoPluginUrls)
        except:
            self.showMessage(self.REMOTE_REPOSITORY_STR, self.ERROR_PROCESSING_REPOSITORY_STR)
            return
        self.listPlugins(self.repoPlugins, self.repoPluginTable)
        self.repoUrl = self.repoUrlEdit.text
        self.log(self.REMOTE_REPOSITORY_STR, self.REPOSITORY_LISTED_STR)
        return

    def repoTableCellClicked(self, row, col):
        self.listMetadata(self.repoPlugins, self.repoPluginTable, self.repoMetaDataTable, row)
        return
    
    def repoTableCellDoubleClicked(self, row, col):
        self.showPlugin(self.repoPlugins, self.repoPluginTable, row)
        return
    
    def remoteToLocalButtonClicked(self):
        if self.localDirEdit.text <> self.localDir:
            self.showMessage(self.REMOTE_TO_LOCAL_STR, self.LIST_LOCAL_BEFORE_REMOTE_TO_LOCAL_STR)
            return
        if self.repoUrlEdit.text <> self.repoUrl:
            self.showMessage(self.REMOTE_TO_LOCAL_STR, self.LIST_REMOTE_BEFORE_REMOTE_TO_LOCAL_STR)
            return
        for pluginName in self.repoPlugins:
            localFilename = None
            remoteFilename = self.repoPlugins[pluginName][self.PLUGIN_DICT_FILENAME_STR]
            remoteFullPath = os.path.join(self.localDir, remoteFilename)
            if pluginName in self.localPlugins:
                localVersion = self.localPlugins[pluginName][self.PLUGIN_DICT_METADATA_STR][self.METADATA_VERSION_STR]
                remoteVersion = self.repoPlugins[pluginName][self.PLUGIN_DICT_METADATA_STR][self.METADATA_VERSION_STR]
                if not remoteVersion > localVersion:
                    if not self.autoOverwriteCheckBox.checked:
                        self.log(self.REMOTE_TO_LOCAL_STR, self.LOCAL_NON_LOWER_SKIPPED_STR % (pluginName, remoteVersion, localVersion))
                        continue
                localFilename = self.localPlugins[pluginName][self.PLUGIN_DICT_FILENAME_STR]
                try:
                    os.rename(os.path.join(self.localDir, localFilename), remoteFullPath)
                except:
                    self.showMessage(self.REMOTE_TO_LOCAL_STR, self.LOCAL_RENAME_ERROR % (pluginName, localFilename, remoteFilename))
                    continue
            if os.path.exists(remoteFullPath):
                if None == localFilename:
                    self.showMessage(self.REMOTE_TO_LOCAL_STR, self.LOCAL_EXISTS_ERROR % (pluginName, remoteFilename, self.localPluginsFilenameToName[remoteFilename]))
                    continue
            content = self.repoPlugins[pluginName][self.PLUGIN_DICT_CONTENT_STR]
            open(remoteFullPath, "wb").write(content)
        self.localDirListButtonClicked()
        return
    
    pass

main_class = pluginMgr
main_class()
