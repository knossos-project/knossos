#ifndef FTP_H
#define FTP_H

#include <QThread>
#include "knossos-global.h"
#include "loader.h"

int downloadFile(const char *remote_path, char *local_filename);
int downloadMag1ConfFile();

struct FtpElement {
    bool isOverlay;
    C_Element *cube;
};

class FtpThread : public QThread
{
public:
    FtpThread(void *ctx);
    void run();
protected:
    void *ctx;
};

#endif // FTP_H
