#include "knossos-global.h"

extern struct stateInfo *state;

int ftpthreadfunc(SDL_sem *ftpThreadSem) {
    C_Element *currentCube = NULL, *prevCube = NULL;
    FILE *fh = NULL;

    /* LOG("FTP Thread about to start %d", GetCurrentThreadId()); */

    if (!FtpConnect(state->ftpHostName,&state->ftpConn)) {
        LOG("FTP Connection Error!");
        return FALSE;
    }
    if (!FtpLogin(state->ftpUsername,state->ftpPassword,state->ftpConn))
    {
        LOG("FTP Login Failure: %s", FtpLastResponse(state->ftpConn));
        return FALSE;
    }

    for (currentCube = state->loaderState->Dcoi->previous; currentCube != state->loaderState->Dcoi; currentCube = prevCube) {
        prevCube = currentCube->previous;
        /* LOG("Wait on FTP semaphore in %d", HASH_COOR(currentCube->coordinate)); */
        if (0 == SDL_SemWaitTimeout(ftpThreadSem, 1)) {
            LOG("Signalled on FTP semaphore in %d", HASH_COOR(currentCube->coordinate));
            break;
        }
        currentCube->hasError = FALSE;
        fh = fopen(currentCube->local_filename, "rb");
        if (NULL == fh) {
            /* LOG("FTP Thread not exists %d", HASH_COOR(currentCube->coordinate)); */
            /* LOG("Downloading %d,%d,%d", currentCube->coordinate.x, currentCube->coordinate.y, currentCube->coordinate.z); */
            if (1 != FtpGet(currentCube->local_filename, currentCube->fullpath_filename, FTPLIB_BINARY, state->ftpConn)) {
                /* LOG("FTP Thread downloading %d failed!", HASH_COOR(currentCube->coordinate)); */
                currentCube->hasError = TRUE;
            }
            else {
                /* LOG("FTP Thread downloaded %d", HASH_COOR(currentCube->coordinate)); */
            }
        }
        else {
            /* LOG("FTP Thread exists %d", HASH_COOR(currentCube->coordinate)); */
            fclose(fh);
        }
        SDL_SemPost(currentCube->ftpSem);
        /* LOG("FTP Thread posted semaphore on %d", HASH_COOR(currentCube->coordinate)); */
    }

    FtpQuit(state->ftpConn);
    state->ftpConn = NULL;
    /* LOG("FTP Thread about to quit %d", GetCurrentThreadId()); */

    return TRUE;
}
