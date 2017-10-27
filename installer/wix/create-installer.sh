#!/bin/bash
export PATH="/c/Program Files (x86)/WiX Toolset v3.11/bin":$PATH$
candle knossos.wxs && light knossos.wixobj
candle bundle.wxs -ext WixBalExtension -ext WixUtilExtension && light bundle.wixobj -ext WixBalExtension
insignia.exe -ib bundle.exe -o engine.exe
read -n1 -r -p "sign engine.exe  then press a key"
insignia -ab engine.exe bundle.exe -o bundle.exe
read -n1 -r -p "sign bundle.exe  then press a key"
mv bundle.exe win.setup.KNOSSOS-5.1.exe
