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
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

extern  stateInfo *state;

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
  C_Element *elem =(C_Element *)stream;
  if ((NULL == elem->local_filename) || (NULL == elem)) {
    return size*nmemb;
  }
  if(NULL == elem->ftp_fh) {
    /* open file for writing */
    elem->ftp_fh = fopen(elem->local_filename, "wb");
    if(NULL == elem->ftp_fh) {
      return -1; /* failure, can't open file to write */
    }
  }

  return fwrite(buffer, size, nmemb, elem->ftp_fh);
}

CURLM *curlm = NULL;
int downloadFiles(CURL **eh_array, int totalCubeCount, C_Element *cubeArray[], int currentCubeCount, int max_connections, int pipelines_per_connection, int max_downloads, QSemaphore *ftpThreadSem, QSemaphore *loaderThreadSem, int *hadErrors/*, DWORD beginTickCount*/)
{
    C_Element *currentCube;
    CURLMsg *msg;
    CURL *eh;
    CURLcode res;
    long L;
    unsigned int C=0;
    int M, Q, U = -1;
    fd_set R, W, E;
    struct timeval T;
    long httpCode = 0;
    char remoteURL[MAX_PATH];
    int isBreak = false;
    int retVal = EXIT_SUCCESS;
    int result;

    *hadErrors = false;

    if (NULL == curlm) {
        curlm = curl_multi_init();
        if (NULL == curlm) {
            LOG("Failed to initialize CURL multi-mode");
            return EXIT_FAILURE;
        }
    }
    curl_multi_setopt(curlm, CURLMOPT_PIPELINING, (long)0);
    curl_multi_setopt(curlm, CURLMOPT_MAXCONNECTS, (long)currentCubeCount);

    for (C = 0; C < currentCubeCount; ++C) {
        eh = eh_array[C];
        currentCube = cubeArray[C];
        snprintf(remoteURL, MAX_PATH, "http://%s:%s@%s%s", state->ftpUsername, state->ftpPassword, state->ftpHostName, currentCube->fullpath_filename);
        curl_easy_setopt(eh, CURLOPT_URL, remoteURL);
        curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, my_fwrite);
        curl_easy_setopt(eh, CURLOPT_WRITEDATA, currentCube);
        curl_easy_setopt(eh, CURLOPT_PRIVATE, currentCube);
        currentCube->curlHandle = eh;
        curl_multi_add_handle(curlm, eh);
    }

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
                LOG("FTP thread should exit");
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
            LOG("E: curl_multi_fdset\n");
            *hadErrors = true;
            retVal = EXIT_FAILURE;
            break;
          }
          if (curl_multi_timeout(curlm, &L)) {
            LOG("E: curl_multi_timeout\n");
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
             /* LOG("E: select(%i,,,,%li): %i: %s\n",
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
                eh = msg->easy_handle;
                curl_easy_getinfo(eh, CURLINFO_PRIVATE, &currentCube);
                curl_easy_getinfo(eh, CURLINFO_HTTP_CODE , &httpCode);
                curl_multi_remove_handle(curlm, eh);
                currentCube->isFinished = true;
                currentCube->curlHandle = NULL;
                if ((CURLE_OK != result) || (200 != httpCode)) {
                    LOG("result = %d, httpCode = %d", result, httpCode);
                    currentCube->hasError = true;
                    *hadErrors = true;
                }
                if (NULL != currentCube->ftp_fh) {
                    fclose(currentCube->ftp_fh); /* close the local file */
                    currentCube->ftp_fh = NULL;
                    if (currentCube->hasError) {
                        remove(currentCube->local_filename);
                    }
                }
                if (NULL != loaderThreadSem) {
                    loaderThreadSem->release();
                }
          }
        }
    }
    for (C = 0; C < currentCubeCount; C++) {
        currentCube = cubeArray[C];
        if (currentCube->isFinished) {
            continue;
        }
        currentCube->isFinished = true;
        *hadErrors = true;
        currentCube->isAborted = true;
        if (NULL != currentCube->ftp_fh) {
            fclose(currentCube->ftp_fh); /* close the local file */
            currentCube->ftp_fh = NULL;
            remove(currentCube->local_filename);
        }
        eh = currentCube->curlHandle;
        currentCube->curlHandle = NULL;
        if (NULL != loaderThreadSem) {
            loaderThreadSem->release();
        }
        if (NULL == eh) {
            continue;
        }
        curl_multi_remove_handle(curlm, eh);
    }

    return retVal;
}

int downloadFile(char *remote_path, char *local_filename) {
    int retVal;
    int hadErrors;
    C_Element elem;
    CURL *eh = NULL;

    C_Element *elem_array = &elem;
    memset(&elem, 0, sizeof(elem));
    elem.fullpath_filename = remote_path;
    elem.local_filename = local_filename;
    eh = curl_easy_init();
    if (NULL == eh) {
        return EXIT_FAILURE;
    }
    retVal = downloadFiles(&eh, 1, &elem_array, 1, 1, 1, 1, NULL, NULL, &hadErrors/*, GetTickCount()*/);
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
    CURL **eh_array = NULL;
    FILE *fh = NULL;
    int i;
    int C;
    int isBreak = false;
    int retVal = true;
    int MAX_DOWNLOADS;
    int max_connections, pipelines_per_connection;
    int hadErrors = false;
    int totalDownloads = 0;

    eh_array = (CURL**)malloc(sizeof(CURL *) * fts->cubeCount);
    if (NULL == eh_array) {
        LOG("Error allocating eh_array");
        return EXIT_FAILURE;
    }
    memset(eh_array, 0, sizeof(CURL *) * fts->cubeCount);
    for (i = 0; i < fts->cubeCount; i++) {
        eh_array[i] = curl_easy_init();
    }
    currentCube = fts->thisPtr->Dcoi->previous;
    firstCube = fts->thisPtr->Dcoi;
    multiCubes = randomize_connection_number(&max_connections, &pipelines_per_connection, &MAX_DOWNLOADS);
    i = 0;
    if (NULL == multiCubes) {
        LOG("Error allocating multiCubes!");
        return false;
    }
    while (currentCube != firstCube) {
        prevCube = currentCube->previous;

        fh = fopen(currentCube->local_filename, "rb");
        if (NULL != fh) {
            fclose(fh);
            currentCube->isFinished = true;
            fts->loaderThreadSem->release();
        }
        else {
            multiCubes[i++] = currentCube;
        }
        if ((MAX_DOWNLOADS == i) || ((prevCube == firstCube) && (i > 0))) {
            if (EXIT_SUCCESS != downloadFiles(eh_array, fts->cubeCount, multiCubes, i, max_connections, pipelines_per_connection, MAX_DOWNLOADS, fts->ftpThreadSem, fts->loaderThreadSem, &hadErrors/*, fts->beginTickCount*/)) {
                retVal = false;
                break;
            }
            totalDownloads += i;
            i = 0;
        }
        currentCube = prevCube;
    }

    if (NULL != multiCubes) {
        free(multiCubes); multiCubes = NULL;
    }

    for (i = 0; i < fts->cubeCount; i++) {
        curl_easy_cleanup(eh_array[i]);
        eh_array[i] = NULL;
    }
    free(eh_array); eh_array = NULL;

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
