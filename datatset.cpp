#include "datatset.h"

#include "network.h"
#include "stateInfo.h"

#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

Dataset Dataset::fromLegacyConf(QString config) {
    Dataset info;

    QTextStream stream(&config);
    QString line;
    while (!(line = stream.readLine()).isNull()) {
        const QStringList tokenList = line.split(QRegularExpression("[ ;]"));
        QString token = tokenList.front();

        if (token == "experiment") {
            token = tokenList.at(2);
            info.experimentname = token.remove('\"');
        } else if (token == "scale") {
            token = tokenList.at(1);
            if(token == "x") {
                info.scale.x = tokenList.at(2).toFloat();
            } else if(token == "y") {
                info.scale.y = tokenList.at(2).toFloat();
            } else if(token == "z") {
                info.scale.z = tokenList.at(2).toFloat();
            }
        } else if (token == "boundary") {
            token = tokenList.at(1);
            if(token == "x") {
                info.boundary.x = tokenList.at(2).toFloat();
            } else if (token == "y") {
                info.boundary.y = tokenList.at(2).toFloat();
            } else if (token == "z") {
                info.boundary.z = tokenList.at(2).toFloat();
            }
        } else if (token == "magnification") {
            info.magnification = tokenList.at(1).toInt();
        } else if (token == "cube_edge_length") {
            info.cubeEdgeLength = tokenList.at(1).toInt();
        } else if (token == "ftp_mode") {
            info.remote = true;
            info.url = tokenList.at(1) + tokenList.at(2);
            info.url.setScheme("http");
            info.url.setUserName(tokenList.at(3));
            info.url.setPassword(tokenList.at(4));
            //discarding ftpFileTimeout parameter
        } else if (token == "compression_ratio") {
            info.compressionRatio = tokenList.at(1).toInt();
        } else {
            qDebug() << "Skipping unknown parameter" << token;
        }
    }

    return info;
}
