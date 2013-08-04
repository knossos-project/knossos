#include "knossos-global.h"
#include "stdio.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <curl/curl.h>
#include <curl/multi.h>

extern struct stateInfo *state;

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

LARGE_INTEGER Counter() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li;
}

CURLM *curlm = NULL;
int32_t downloadFiles(CURL **eh_array, int32_t totalCubeCount, C_Element *cubeArray[], int32_t currentCubeCount, int32_t max_connections, int32_t pipelines_per_connection, int32_t max_downloads, SDL_sem *ftpThreadSem, SDL_sem *loaderThreadSem, int32_t *hadErrors/*, DWORD beginTickCount*/)
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
    int32_t isBreak = FALSE;
    int32_t retVal = EXIT_SUCCESS;
    int32_t result;
    LARGE_INTEGER zeroCount, currentCount, freq;

    *hadErrors = FALSE;

    QueryPerformanceFrequency(&freq);
    zeroCount = Counter();

    if (NULL == curlm) {
        curlm = curl_multi_init();
        if (NULL == curlm) {
            LOG("Failed to initialize CURL multi-mode");
            return EXIT_FAILURE;
        }
    }
    curl_multi_setopt(curlm, CURLMOPT_PIPELINING, (long)0);
    curl_multi_setopt(curlm, CURLMOPT_MAXCONNECTS, (long)totalCubeCount);
    curl_multi_setopt(curlm, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long)totalCubeCount);
    //curl_multi_setopt(curlm, CURLMOPT_MAX_PIPELINE_LENGTH, (long)pipelines_per_connection);

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
            SDL_LockMutex(state->protectLoadSignal);
            if (state->loadSignal == TRUE) {
                isBreak = TRUE;
                LOG("DEBUG FTP should exit (loadSignal)")
            }
            SDL_UnlockMutex(state->protectLoadSignal);
            if (state->datasetChangeSignal != NO_MAG_CHANGE) {
                isBreak = TRUE;
                //LOG("DEBUG FTP should exit (datasetChangeSignal)")
            }
            if (0 == SDL_SemWaitTimeout(ftpThreadSem, 0)) {
                isBreak = TRUE;
                LOG("DEBUG FTP should exit (ftpThreadSem)")
            }
            if (isBreak) {
                LOG("FTP thread should exit");
                *hadErrors = TRUE;
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
            *hadErrors = TRUE;
            retVal = EXIT_FAILURE;
            break;
          }
          if (curl_multi_timeout(curlm, &L)) {
            LOG("E: curl_multi_timeout\n");
            *hadErrors = TRUE;
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
              LOG("E: select(%i,,,,%li): %i: %s\n",
                  M+1, L, errno, strerror(errno));
                *hadErrors = TRUE;
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
                currentCube->isFinished = TRUE;
                currentCube->curlHandle = NULL;
                //LOG("DEBUG result = %d, httpCode = %d for %d, %d, %d (DEBUG %d)", result, httpCode, currentCube->coordinate.x, currentCube->coordinate.y, currentCube->coordinate.z, currentCube->debugVal);
                if ((CURLE_OK != result) || (200 != httpCode)) {
                    LOG("result = %d, httpCode = %d", result, httpCode);
                    currentCube->hasError = TRUE;
                    *hadErrors = TRUE;
                }
                else {
                    //currentCount = Counter();
                    //LOG("BENCHMARK\t%d\t%d\t%08X%08X\t%08X%08X\t%08X%08X", currentCubeCount, currentCube->debugVal, currentCount.HighPart, currentCount.LowPart, zeroCount.HighPart, zeroCount.LowPart, freq.HighPart, freq.LowPart);
                    //LOG("FTP [%08X] Downloaded %d", GetTickCount() - beginTickCount, currentCube->debugVal);
                    //currentCube->tickDownloaded = GetTickCount();
                }
                if (NULL != currentCube->ftp_fh) {
                    fclose(currentCube->ftp_fh); /* close the local file */
                    currentCube->ftp_fh = NULL;
                    if (currentCube->hasError) {
                        remove(currentCube->local_filename);
                    }
                }
                if (NULL != loaderThreadSem) {
                    SDL_SemPost(loaderThreadSem);
                }
                //curl_easy_cleanup(eh); eh = NULL;
          }
        }
    }
    /*
    for (C = 0; C < cubeCount; C++) {
        LOG("DEBUG %d finished: %d", C, cubeArray[C]->isFinished);
    }
    */
    for (C = 0; C < currentCubeCount; C++) {
        currentCube = cubeArray[C];
        if (currentCube->isFinished) {
            //LOG("DEBUG Clean %d, %d, %d (DEBUG %d)", currentCube->coordinate.x, currentCube->coordinate.y, currentCube->coordinate.z, currentCube->debugVal);
            continue;
        }
        currentCube->isFinished = TRUE;
        *hadErrors = TRUE;
        LOG("DEBUG Abort %d, %d, %d (DEBUG %d)", currentCube->coordinate.x, currentCube->coordinate.y, currentCube->coordinate.z, currentCube->debugVal);
        currentCube->isAborted = TRUE;
        if (NULL != currentCube->ftp_fh) {
            fclose(currentCube->ftp_fh); /* close the local file */
            currentCube->ftp_fh = NULL;
            remove(currentCube->local_filename);
        }
        eh = currentCube->curlHandle;
        currentCube->curlHandle = NULL;
        if (NULL != loaderThreadSem) {
            //LOG("Posting for %i", currentCube->debugVal);
            SDL_SemPost(loaderThreadSem);
        }
        if (NULL == eh) {
            continue;
        }
        curl_multi_remove_handle(curlm, eh);
        //curl_easy_cleanup(eh); eh = NULL;
    }

    //curl_multi_cleanup(curlm);
    return retVal;
}

int32_t downloadFile(char *remote_path, char *local_filename) {
    int32_t retVal;
    int32_t hadErrors;
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

C_Element **randomize_connection_number(int32_t *max_connections, int32_t *pipelines_per_connection, int32_t *max_downloads) {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    *max_connections = 5;//((li.LowPart & 0xFFFF) % 5) + 3;
    *pipelines_per_connection = 1; //((li.LowPart >> 16) % 5) + 1;
    *max_downloads = (*max_connections) * (*pipelines_per_connection);
    return malloc((*max_downloads) * sizeof(C_Element*));
}

//#define MAX_DOWNLOADS 2 /* number of simultaneous transfers */
int ftpthreadfunc(ftp_thread_struct *fts) {
    C_Element *currentCube, *prevCube, *firstCube;
    C_Element **multiCubes;
    CURL **eh_array = NULL;
    FILE *fh = NULL;
    int32_t i;
    int32_t C;
    int32_t isBreak = FALSE;
    int32_t retVal = TRUE;
    int32_t MAX_DOWNLOADS;
    int32_t max_connections, pipelines_per_connection;
    int32_t hadErrors = FALSE;
    int32_t totalDownloads = 0;

    //LOG("DEBUG ftpthreadfunc Start");

    eh_array = malloc(sizeof(CURL *) * fts->cubeCount);
    if (NULL == eh_array) {
        LOG("Error allocating eh_array");
        return EXIT_FAILURE;
    }
    memset(eh_array, 0, sizeof(CURL *) * fts->cubeCount);
    for (i = 0; i < fts->cubeCount; i++) {
        eh_array[i] = curl_easy_init();
    }
    /* LOG("FTP Thread about to start %d", GetCurrentThreadId()); */
    currentCube = state->loaderState->Dcoi->previous;
    firstCube = state->loaderState->Dcoi;
    multiCubes = randomize_connection_number(&max_connections, &pipelines_per_connection, &MAX_DOWNLOADS);
    i = 0;
    if (NULL == multiCubes) {
        LOG("Error allocating multiCubes!");
        return FALSE;
    }
    while (currentCube != firstCube) {
        prevCube = currentCube->previous;

        fh = fopen(currentCube->local_filename, "rb");
        if (NULL != fh) {
            // LOG("FTP Thread exists %d", HASH_COOR(currentCube->coordinate));
            fclose(fh);
            currentCube->isFinished = TRUE;
            SDL_SemPost(fts->loaderThreadSem);
        }
        else {
            /* LOG("FTP Thread not exists %d", HASH_COOR(currentCube->coordinate)); */
            multiCubes[i++] = currentCube;
        }
        if ((MAX_DOWNLOADS == i) || ((prevCube == firstCube) && (i > 0))) {
            /*
            for (C = 0; C < i; C++) {
                LOG("DEBUG ftpthreadfunc %d: %d, %d, %d", C, multiCubes[C]->coordinate.x, multiCubes[C]->coordinate.y, multiCubes[C]->coordinate.z);
            }
            */
            if (EXIT_SUCCESS != downloadFiles(eh_array, fts->cubeCount, multiCubes, i, max_connections, pipelines_per_connection, MAX_DOWNLOADS, fts->ftpThreadSem, fts->loaderThreadSem, &hadErrors/*, fts->beginTickCount*/)) {
                //LOG("downloadFiles failed!");
                retVal = FALSE;
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

    if ((TRUE == retVal) && (FALSE == hadErrors)) {
        //LOG("BINCHMARK (%08X)\t%d\t%d\t%d", fts->debugVal, MAX_DOWNLOADS, GetTickCount() - fts->beginTickCount, totalDownloads);
    }

    for (i = 0; i < fts->cubeCount; i++) {
        curl_easy_cleanup(eh_array[i]);
        eh_array[i] = NULL;
    }
    free(eh_array); eh_array = NULL;

    if (FALSE == retVal) {
        // An extra post to wake up loader if needed
        //LOG("FTP thread FALSE retVal loader semphore post.")
        SDL_SemPost(fts->loaderThreadSem);
    }

    //LOG("DEBUG ftpthreadfunc Finish");

    return retVal;
}
