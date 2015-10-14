#!/bin/bash
candle knossos.wxs && light knossos.wixobj
candle bundle.wxs -ext WixBalExtension && light bundle.wixobj -ext WixBalExtension