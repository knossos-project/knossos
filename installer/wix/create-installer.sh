#!/bin/bash
candle knossos.wxs && light knossos.wixobj
candle bundle.wxs -ext WixBalExtension -ext WixUtilExtension && light bundle.wixobj -ext WixBalExtension

#signing process:
#insignia -ib bundle.exe -o engine.exe
# ... sign engine.exe with e.g. signtool engine.exe
#insignia -ab engine.exe bundle.exe -o bundle.exe
# ... sign bundle.exe with e.g. signtool bundle.exe
