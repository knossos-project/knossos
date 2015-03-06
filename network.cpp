#include "network.h"

#include "loader.h"
#include "stateInfo.h"

#include <QDir>
#include <QHttpMultiPart>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSemaphore>

#include <curl/curl.h>
#include <curl/multi.h>
#include <stdio.h>
#ifdef Q_OS_WIN
#include "windows.h"
#endif
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

Network::Network(const QObject *) {}

void Network::submitSegmentationJob(const QString & path) {
   QHttpPart part;
   part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/zip"));
   // Django requires enctype="multipart/form-data" for receiving file uploads
   part.setHeader(QNetworkRequest::ContentDispositionHeader,
                  QVariant("enctype=\"multipart/form-data\"; name=\"submit\"; filename=\"" + QFileInfo(path).fileName() + "\""));
   auto uploadfile = new QFile(path);
   uploadfile->open(QIODevice::ReadOnly);
   part.setBodyDevice(uploadfile);

   auto multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
   multiPart->append(part);
   uploadfile->setParent(multiPart);
   auto & job = Segmentation::singleton().job;
   QNetworkRequest request(QUrl(job.submitPath));
   auto reply = manager.post(request, multiPart);
   multiPart->setParent(reply);
   connect(reply, &QNetworkReply::finished, [reply]() {
       QString content = (reply->error() == QNetworkReply::NoError) ? reply->readAll() : reply->errorString();
       QMessageBox verificationBox(QMessageBox::Information, "Your verification", content);
       verificationBox.exec();
       reply->deleteLater();
   });
}

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
  FtpElement *elem =(FtpElement *)stream;
  char *local_filename;
  FILE **ftp_fh_ptr;
  local_filename = elem->isOverlay ? elem->cube->local_overlay_filename : elem->cube->local_data_filename;
  ftp_fh_ptr = elem->isOverlay ? &elem->cube->ftp_overlay_fh : &elem->cube->ftp_data_fh;
  if ((NULL == local_filename) || (NULL == elem)) {
    return size*nmemb;
  }
  if(NULL == *ftp_fh_ptr) {
    /* open file for writing */
    *ftp_fh_ptr = fopen(local_filename, "wb");
    if(NULL == *ftp_fh_ptr) {
      return -1; /* failure, can't open file to write */
    }
  }
  return fwrite(buffer, size, nmemb, *ftp_fh_ptr);
}

static size_t conf_fwrite(void *buffer, size_t size, size_t nmemb, void *stream) {
    FtpElement *elem = (FtpElement *)stream;
    FILE **ftp_fh_ptr = &elem->cube->ftp_data_fh;
    *ftp_fh_ptr = fopen(elem->cube->local_data_filename, "wb");

    if(NULL == *ftp_fh_ptr) {
      return -1; /* failure, can't open file to write */
    }

    return fwrite(buffer, size, nmemb, *ftp_fh_ptr);
}

std::string downloadRemoteConfFile(QString url) {
    CURL *eh = NULL;
    FtpElement *elem = new FtpElement;
    elem->cube = new C_Element;
    elem->isOverlay = false;

    char remoteURL[CSTRING_SIZE];
    snprintf(remoteURL, CSTRING_SIZE, url.toStdString().c_str());

    url.remove("http://");
    url.remove("https://");
    elem->cube->fullpath_data_filename = new char[url.length() + 1];
    strcpy(elem->cube->fullpath_data_filename, url.toStdString().c_str());

    char *lpath = (char*)malloc(CSTRING_SIZE);
    snprintf(lpath, CSTRING_SIZE, "%s/knossos.conf", state->loadFtpCachePath);

    //libcurl can't create the folder if it does not exist
    QString qpath(state->loadFtpCachePath);

    QDir tmppath(qpath);
    if(!tmppath.exists()) tmppath.mkdir(qpath);

    elem->cube->local_data_filename = lpath;

    eh = curl_easy_init();

    curl_easy_setopt(eh, CURLOPT_URL, remoteURL);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, conf_fwrite);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, elem);
    curl_easy_setopt(eh, CURLOPT_PRIVATE, elem);
    curl_easy_setopt(eh, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(eh, CURLOPT_CONNECTTIMEOUT, 10);

    curl_easy_perform(eh);
    curl_easy_cleanup(eh);

    long httpCode;
    curl_easy_getinfo(eh, CURLINFO_HTTP_CODE , &httpCode);

    if(elem->cube->ftp_data_fh != NULL) {
        fclose(elem->cube->ftp_data_fh);
        elem->cube->ftp_data_fh = NULL;
    }
    else {
        free(elem->cube);
        free(elem);
        free(lpath);
        return "";
    }

    if(httpCode != 200) {
        free(elem->cube);
        free(elem);
        free(lpath);
        return "";
    }

    free(elem->cube);
    free(elem);
    free(lpath);

    std::string ret{state->loadFtpCachePath};
    ret.append("/knossos.conf");

    return ret;
}

CURLM *curlm = NULL;

void fill_curl_eh(CURL *eh, C_Element *cube, bool isOverlay, FtpElement *elem) {
    char remoteURL[CSTRING_SIZE];

    elem->isOverlay = isOverlay;
    elem->cube = cube;
    CURL **eh_ptr = isOverlay ? &cube->curlOverlayHandle : &cube->curlDataHandle;
    *eh_ptr = eh;
    snprintf(remoteURL, CSTRING_SIZE, "http://%s:%s@%s%s", state->ftpUsername, state->ftpPassword, state->ftpHostName,
             isOverlay ? cube->fullpath_overlay_filename : cube->fullpath_data_filename);

    curl_easy_setopt(eh, CURLOPT_URL, remoteURL);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, my_fwrite);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, elem);
    curl_easy_setopt(eh, CURLOPT_PRIVATE, elem);
    //curl_easy_setopt(eh, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(eh, CURLOPT_LOW_SPEED_LIMIT, 1);
    curl_easy_setopt(eh, CURLOPT_LOW_SPEED_TIME, 10);
    curl_easy_setopt(eh, CURLOPT_CONNECTTIMEOUT, 10);
    curl_multi_add_handle(curlm, eh);
}

int downloadFiles(CURL **data_eh_array, CURL **overlay_eh_array, int /*totalCubeCount*/, C_Element *cubeArray[], int currentCubeCount, int /*max_connections*/, int /*pipelines_per_connection*/, int /*max_downloads*/, QSemaphore *ftpThreadSem, QSemaphore *loaderThreadSem, int *hadErrors/*, int *retriesPend, DWORD beginTickCount*/)
{
    C_Element *currentCube;
    FtpElement *ftpElementDataArray = NULL, *ftpElementOverlayArray = NULL;
    FtpElement *elem;
    int ftp_element_array_size = sizeof(FtpElement) * currentCubeCount;
    bool doOverlay = (NULL != overlay_eh_array);
    CURLMsg *msg;
    CURL *data_eh, *overlay_eh;
    CURL **eh_ptr;
    long L;
    int M, Q, U = -1;
    fd_set R, W, E;
    struct timeval T;
    long httpCode = 0;
    int isBreak = false;
    int retVal = EXIT_SUCCESS;
    int result;
    int actualCubeCount = 0;

    *hadErrors = false;
    //*retriesPend = false;

    ftpElementDataArray = (FtpElement*)malloc(ftp_element_array_size);
    if (NULL == ftpElementDataArray) {
        qDebug() << "Failed to allocate ftpElementDataArray!";
        return EXIT_FAILURE;
    }
    memset(ftpElementDataArray, 0, ftp_element_array_size);
    ftpElementOverlayArray = (FtpElement*)malloc(ftp_element_array_size);
    if (NULL == ftpElementOverlayArray) {
        qDebug() << "Failed to allocate ftpElementOverlayArray!";
        return EXIT_FAILURE;
    }
    memset(ftpElementOverlayArray, 0, ftp_element_array_size);

    if (NULL == curlm) {
        curlm = curl_multi_init();
        if (NULL == curlm) {
            qDebug() << "Failed to initialize CURL multi-mode";
            return EXIT_FAILURE;
        }
    }
    curl_multi_setopt(curlm, CURLMOPT_PIPELINING, (long)0);

    actualCubeCount = 0;
    for (int C = 0; C < currentCubeCount; ++C) {
        currentCube = cubeArray[C];
        if (currentCube->isFinished) {
            continue;
        }
        // Reimplement this retry-related code to support data/overlay combination
        /*
        if (currentCube->hasError) {
            currentCube->hasError = false;
        }
        */
        fill_curl_eh(data_eh_array[C], currentCube, false, &ftpElementDataArray[C]);
        actualCubeCount++;
        if (doOverlay) {
            fill_curl_eh(overlay_eh_array[C], currentCube, true, &ftpElementOverlayArray[C]);
            actualCubeCount++;
        }
    }
    curl_multi_setopt(curlm, CURLMOPT_MAXCONNECTS, (long)actualCubeCount);

    while (U) {
        if (NULL != ftpThreadSem) {
            if (true == ftpThreadSem->tryAcquire(1)) {
                isBreak = true;
            }
            if (isBreak) {
                qDebug() << "FTP thread should exit";
                *hadErrors = true;
                retVal = EXIT_FAILURE;
                break;
            }
        }
        curl_multi_perform(curlm, &U);
        if (U) {
          FD_ZERO(&R);
          FD_ZERO(&W);
          FD_ZERO(&E);
          if (curl_multi_fdset(curlm, &R, &W, &E, &M)) {
            qDebug() << "E: curl_multi_fdset\n";
            *hadErrors = true;
            retVal = EXIT_FAILURE;
            break;
          }
          if (curl_multi_timeout(curlm, &L)) {
            qDebug() << "E: curl_multi_timeout\n";
            *hadErrors = true;
            retVal = EXIT_FAILURE;
            break;
          }
          if (L == -1) {
            L = 100;
          }
          if (M == -1) {
        #ifdef WIN32
            Sleep(L);
        #else
            sleep(L / 1000);
        #endif
          }
          else {
            T.tv_sec = L/1000;
            T.tv_usec = (L%1000)*1000;
            if (0 > select(M+1, &R, &W, &E, &T)) {
             /* qDebug("E: select(%i,,,,%li): %i: %s\n",
                  M+1, L, errno, strerror(errno)); */
                *hadErrors = true;
                retVal = EXIT_FAILURE;
                break;
            }
          }
        }
        while ((msg = curl_multi_info_read(curlm, &Q))) {
              if (msg->msg == CURLMSG_DONE) {
                result = msg->data.result;
                CURL *eh = msg->easy_handle;
                curl_easy_getinfo(eh, CURLINFO_PRIVATE, &elem);
                currentCube = elem->cube;
                curl_easy_getinfo(eh, CURLINFO_HTTP_CODE , &httpCode);
                curl_multi_remove_handle(curlm, eh);
                bool isOverlay = elem->isOverlay;
                bool *is_finished_ptr = isOverlay ? &currentCube->isOverlayFinished : &currentCube->isDataFinished;
                *is_finished_ptr = true;
                currentCube->isFinished = currentCube->isDataFinished &
                        (doOverlay ? currentCube->isOverlayFinished : true);
                eh_ptr = isOverlay ? &currentCube->curlOverlayHandle : &currentCube->curlDataHandle;
                *eh_ptr = NULL;
                bool *has_error_ptr = isOverlay ? &currentCube->hasOverlayError : &currentCube->hasDataError;
                if ((CURLE_OK != result) || (200 != httpCode)) {
                    // Since most datasets are still not massively overlaid, no point in flooding the screen
                    // with errors of 404 failures for them
                    if ((!isOverlay) || (404 != httpCode)) {
                        qDebug() << QString().sprintf("cube coordinate = %d,%d,%d\t%s\tresult = %d\thttpCode = %ld",
                                                       currentCube->coordinate.x, currentCube->coordinate.y,
                                                       currentCube->coordinate.z, isOverlay ? "overlay" : "data",
                                                      result, httpCode);
                    }
                    *has_error_ptr = true;
                    // Retries are disabled until supported for data/overlay download
                    /*
                    currentCube->retries--;
                    if (currentCube->retries > 0) {
                        *retriesPend = true;
                        currentCube->isFinished = false;
                    }
                    */
                    *hadErrors = true;
                }
                FILE **ftp_fh_ptr = isOverlay ? &currentCube->ftp_overlay_fh : &currentCube->ftp_data_fh;
                char **local_filename = isOverlay ? &currentCube->local_overlay_filename : &currentCube->local_data_filename;
                if (NULL != *ftp_fh_ptr) {
                    fclose(*ftp_fh_ptr); /* close the local file */
                    *ftp_fh_ptr = NULL;
                    if (*has_error_ptr) {
                        remove(*local_filename);
                    }
                }
                currentCube->hasError |= *has_error_ptr;
                if (
                        (currentCube->isFinished) &&
                        (NULL != loaderThreadSem)
                        // Retries are disabled until supported for data/overlay download
                        /*
                        &&
                        (
                            (0 == currentCube->retries)
                            ||
                            (!currentCube->hasError)
                        )
                        */
                    )
                {
                    loaderThreadSem->release();
                }
          }
        }
    }
    for (int C = 0; C < currentCubeCount; C++) {
        currentCube = cubeArray[C];
        if (currentCube->isFinished) {
            continue;
        }
        bool isAborted = false;
        if ((!currentCube->isDataFinished) && (!currentCube->hasDataError)) {
            isAborted = true;
            currentCube->isDataFinished = true;
            if (NULL != currentCube->ftp_data_fh) {
                fclose(currentCube->ftp_data_fh); /* close the local file */
                currentCube->ftp_data_fh = NULL;
                remove(currentCube->local_data_filename);
            }
            data_eh = currentCube->curlDataHandle;
            currentCube->curlDataHandle = NULL;
            if (NULL != data_eh) {
                curl_multi_remove_handle(curlm, data_eh);
            }
        }
        if ((!currentCube->isOverlayFinished) && (!currentCube->hasOverlayError)) {
            isAborted = true;
            currentCube->isOverlayFinished = true;
            if (NULL != currentCube->ftp_overlay_fh) {
                fclose(currentCube->ftp_overlay_fh); /* close the local file */
                currentCube->ftp_overlay_fh = NULL;
                remove(currentCube->local_overlay_filename);
            }
            overlay_eh = currentCube->curlOverlayHandle;
            currentCube->curlOverlayHandle = NULL;
            if (NULL != overlay_eh) {
                curl_multi_remove_handle(curlm, overlay_eh);
            }
        }
        if (isAborted) {
            *hadErrors = true;
            currentCube->isFinished = true;
            currentCube->isAborted = true;
            if (NULL != loaderThreadSem) {
                loaderThreadSem->release();
            }
        }
    }

    free(ftpElementDataArray);
    free(ftpElementOverlayArray);

    return retVal;
}

int downloadFile(const char *remote_path, char *local_filename) {
    int retVal;
    int hadErrors;
    C_Element elem;
    CURL *eh = NULL;

    C_Element *elem_array = &elem;
    memset(&elem, 0, sizeof(elem));
    elem.fullpath_data_filename = (char*)malloc(strlen(remote_path) + 1);
    strcpy(elem.fullpath_data_filename, remote_path);
    //qDebug("elem.fullpath: %s", elem.fullpath_filename);
    elem.local_data_filename = local_filename;
    eh = curl_easy_init();
    if (NULL == eh) {
        return EXIT_FAILURE;
    }
    retVal = downloadFiles(&eh, NULL, 1, &elem_array, 1, 1, 1, 1, NULL, NULL, &hadErrors/*, &retriesPend, GetTickCount()*/);
    curl_easy_cleanup(eh); eh = NULL;
    if ((EXIT_FAILURE == retVal) || elem.hasError) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
