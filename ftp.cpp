#include <QSemaphore>
#include "ftp.h"
#include "knossos-global.h"
#include "loader.h"
#include <stdio.h>
#ifdef Q_OS_WIN
#include "windows.h"
#endif

#include <curl/curl.h>
#include <curl/multi.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

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

static size_t mag1conf_fwrite(void *buffer, size_t size, size_t nmemb, void *stream) {
    FtpElement *elem = (FtpElement *)stream;
    FILE **ftp_fh_ptr = &elem->cube->ftp_data_fh;

    *ftp_fh_ptr = fopen(elem->cube->local_data_filename, "wb");

    if(NULL == *ftp_fh_ptr) {
      return -1; /* failure, can't open file to write */
    }

    return fwrite(buffer, size, nmemb, *ftp_fh_ptr);
}

int downloadMag1ConfFile() {
    CURL *eh = NULL;
    FtpElement *elem = new FtpElement;
    elem->cube = new C_Element;
    elem->isOverlay = false;

    QString currentPath = QString("%1mag1/knossos.conf").arg(state->ftpBasePath);

    elem->cube->fullpath_data_filename = new char[currentPath.length() + 1];
    strcpy(elem->cube->fullpath_data_filename, currentPath.toStdString().c_str());

    char remoteURL[MAX_PATH];

    snprintf(remoteURL, MAX_PATH, "http://%s:%s@%s%s", state->ftpUsername, state->ftpPassword, state->ftpHostName, elem->cube->fullpath_data_filename);

    char *lpath = (char*)malloc(MAX_PATH);
    snprintf(lpath, MAX_PATH, "%smag1/knossos.conf", state->loadFtpCachePath);
    elem->cube->local_data_filename = lpath;

    eh = curl_easy_init();

    curl_easy_setopt(eh, CURLOPT_URL, remoteURL);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, mag1conf_fwrite);
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

        return EXIT_FAILURE;
    }

    if(httpCode != 200) {
        free(elem->cube);
        free(elem);
        free(lpath);

        return EXIT_FAILURE;
    }

    free(elem->cube);
    free(elem);
    free(lpath);

    return EXIT_SUCCESS;
}

CURLM *curlm = NULL;

void fill_curl_eh(CURL *eh, C_Element *cube, bool isOverlay, FtpElement *elem) {
    char remoteURL[MAX_PATH];

    elem->isOverlay = isOverlay;
    elem->cube = cube;
    CURL **eh_ptr = isOverlay ? &cube->curlOverlayHandle : &cube->curlDataHandle;
    *eh_ptr = eh;
    snprintf(remoteURL, MAX_PATH, "http://%s:%s@%s%s", state->ftpUsername, state->ftpPassword, state->ftpHostName,
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
            state->protectLoadSignal->lock();
            if (state->loadSignal == true) {
                isBreak = true;
            }
            state->protectLoadSignal->unlock();

            if (state->datasetChangeSignal != NO_MAG_CHANGE) {
                isBreak = true;
            }
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
                        qDebug() << QString().sprintf("cube coordinate = %d,%d,%d\t%s\tresult = %d\thttpCode = %d",
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
    int hadErrors, retriesPend;
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

C_Element **randomize_connection_number(int *max_connections, int *pipelines_per_connection, int *max_downloads) {
    *max_connections = 5;//((li.LowPart & 0xFFFF) % 5) + 3;
    *pipelines_per_connection = 1; //((li.LowPart >> 16) % 5) + 1;
    *max_downloads = (*max_connections) * (*pipelines_per_connection);
    return (C_Element **)malloc((*max_downloads) * sizeof(C_Element*));
}

//#define MAX_DOWNLOADS 2 /* number of simultaneous transfers */
int ftpthreadfunc(ftp_thread_struct *fts) {
    C_Element *currentCube, *prevCube, *firstCube;
    C_Element **multiCubes;
    CURL **data_eh_array = NULL, **overlay_eh_array = NULL;
    int eh_array_size = sizeof(CURL *) * fts->cubeCount;
    FILE *fh = NULL;
    int i;
    // Retries are disabled until supported for data/overlay download
    //extern int RETRIES_NUM;
    int curRetry;
    int retVal = true;
    int downloadRetVal;
    int MAX_DOWNLOADS;
    int max_connections, pipelines_per_connection;
    int hadErrors, retriesPend;
    int totalDownloads = 0;

    data_eh_array = (CURL**)malloc(eh_array_size);
    if (NULL == data_eh_array) {
        qDebug() << "Error allocating data_eh_array";
        return EXIT_FAILURE;
    }
    memset(data_eh_array, 0, eh_array_size);
    overlay_eh_array = (CURL**)malloc(eh_array_size);
    if (NULL == overlay_eh_array) {
        qDebug() << "Error allocating overlay_eh_array";
        return EXIT_FAILURE;
    }
    memset(overlay_eh_array, 0, eh_array_size);

    for (i = 0; i < fts->cubeCount; i++) {
        data_eh_array[i] = curl_easy_init();
        overlay_eh_array[i] = curl_easy_init();
    }
    currentCube = fts->thisPtr->Dcoi->previous;
    firstCube = fts->thisPtr->Dcoi;
    multiCubes = randomize_connection_number(&max_connections, &pipelines_per_connection, &MAX_DOWNLOADS);
    i = 0;
    if (NULL == multiCubes) {
        qDebug() << "Error allocating multiCubes!";
        return false;
    }
    while (currentCube != firstCube) {
        prevCube = currentCube->previous;

        // Caching as alternative to download is currently disabled, until properly handled for data/overlay
        /*
        fh = fopen(currentCube->local_filename, "rb");
        if (NULL != fh) {
            fclose(fh);
            currentCube->isFinished = true;
            fts->loaderThreadSem->release();
        }
        else {
        */
            multiCubes[i++] = currentCube;
        //}
        if ((MAX_DOWNLOADS == i) || ((prevCube == firstCube) && (i > 0))) {
            hadErrors = false;
            // Retries are disabled until supported for data/overlay download
            //do {
                downloadRetVal = downloadFiles(data_eh_array, overlay_eh_array, fts->cubeCount, multiCubes, i,
                                               max_connections, pipelines_per_connection,
                                               MAX_DOWNLOADS,  fts->ftpThreadSem,
                                               fts->loaderThreadSem, &hadErrors
                                               //, &retriesPend
                                               /*, fts->beginTickCount*/);
                if (EXIT_SUCCESS != downloadRetVal) {
                    retVal = false;
                    goto ftpthreadfunc_finish;
                }
            //} while (retriesPend);
            totalDownloads += i;
            i = 0;
        }
        currentCube = prevCube;
    }

ftpthreadfunc_finish:
    if (NULL != multiCubes) {
        free(multiCubes); multiCubes = NULL;
    }

    for (i = 0; i < fts->cubeCount; i++) {
        curl_easy_cleanup(data_eh_array[i]);
        data_eh_array[i] = NULL;
        curl_easy_cleanup(overlay_eh_array[i]);
        overlay_eh_array[i] = NULL;
    }
    free(data_eh_array); data_eh_array = NULL;
    free(overlay_eh_array); overlay_eh_array = NULL;

    if (false == retVal) {
        fts->loaderThreadSem->release();
    }

    return retVal;
}

void FtpThread::run() {
    ftpthreadfunc((ftp_thread_struct *)this->ctx);
}
FtpThread::FtpThread(void *ctx) {
    this->ctx = ctx;
}
