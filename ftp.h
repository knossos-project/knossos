#ifndef FTP_H
#define FTP_H

#include <QThread>
#include "knossos-global.h"

int32_t downloadFile(char *remote_path, char *local_filename);

class FtpThread : public QThread
{
public:
    FtpThread(void *ctx);
    void run();
protected:
    void *ctx;
};

#endif // FTP_H
