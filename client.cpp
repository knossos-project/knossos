//AGAR and SDL TODOS here
//commented out thread loop

#include <SDL/SDL.h>
#include "skeletonizer.h"
#include "remote.h"
#include "knossos.h"
#include "client.h"
#include "sleeper.h"

extern stateInfo *state;
extern stateInfo *tempConfig;

Client::Client(QObject *parent) :
    QObject(parent)
{
}

static int32_t connectToServer() {
    /*clientState *clientState = state->clientState;
    int32_t i = 0;
    int32_t timeoutIn100ms = 0;
    SDL_Thread *ThreadWrapper_SDLNet_TCP_Open; SDL TODO

    timeoutIn100ms = roundFloat((float)state->clientState->connectionTimeout / 100.);
    if(timeoutIn100ms < 1) {
        timeoutIn100ms = 1;
    }
    timeoutIn100ms = 30;

    SDLNet_ResolveHost(&(clientState->remoteServer), clientState->serverAddress, clientState->remotePort);
    if(clientState->remoteServer.host == INADDR_NONE) {
        LOG("Couldn't resolve remote server: %s", SDLNet_GetError());
        return FALSE;
    }

    clientState->connectionTried = FALSE;
    ThreadWrapper_SDLNet_TCP_Open = SDL_CreateThread(Wrapper_SDLNet_TCP_Open, clientState);
    for(i = 0; i < timeoutIn100ms; i++) {
            if(clientState->connectionTried == TRUE) {
                break;
            }
            SDL_Delay(100);
    }
    if(clientState->remoteSocket == NULL) {
            LOG("Error or timeout while connecting to server: %s.", SDLNet_GetError());
            SDL_KillThread(ThreadWrapper_SDLNet_TCP_Open);
            return FALSE;
    }

    clientState->connected = TRUE;
    LOG("Connection established to %s:%d", clientState->serverAddress, clientState->remotePort);

    clientState->socketSet = SDLNet_AllocSocketSet(1);
    if(clientState->socketSet == NULL) {
        LOG("Error allocating socket set: %s", SDLNet_GetError());
        return FALSE;
    }

    SDLNet_TCP_AddSocket(clientState->socketSet, clientState->remoteSocket);

    AG_LabelText(state->viewerState->ag->syncOptLabel, "Connected to server.");
    */
    return TRUE;
}

static int32_t closeConnection() {
/*    if(state->clientState->remoteSocket != NULL) {
        SDLNet_TCP_Close(state->clientState->remoteSocket);
        state->clientState->remoteSocket = NULL;
    }

    if(state->clientState->socketSet != NULL) {
        SDLNet_FreeSocketSet(state->clientState->socketSet);
        state->clientState->socketSet = NULL;
    }

    state->clientState->connected = FALSE;
    AG_LabelText(state->viewerState->ag->syncOptLabel, "No connection to server.");
*/
    return TRUE;
}

static float bytesToFloat(Byte *source) {
    /*
     * There are issues with sending floats over networks. There might be
     * strange bugs when sending data between different architectures...
     */
    float dest = 0.;
    memcpy(&dest, source, sizeof(float));
    return dest;
}

#define FLOATS 16
#define INTS 16
static uint32_t parseInBuffer() {
    int32_t messageLen = 0;
    clientState *clientState = state->clientState;

    Coordinate *pPosition = NULL;

    float f[FLOATS];
    int32_t d[INTS];
    Byte s[2048];

    memset(s, '\0', 2048 * sizeof(char));

    /* TODO
     *  - Check for negative return value from parseInBufferByFmt.
     */

    /* There are no messages with less than 5 bytes.
     *
     * In every case block, messageLen has to be set to the appropriate
     *  value so that the message can be correctly removed from the input buffer
     *  after parsing.
     */
    while(clientState->inBuffer->length >= 5) {
        switch(clientState->inBuffer->data[0]) {
            case KIKI_HIBACK:
                messageLen = Client::parseInBufferByFmt(5, "xd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                clientState->myId = d[0];

                break;

            case KIKI_POSITION:
                messageLen = Client::parseInBufferByFmt(17, "xdddd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                /* d contains, in that order: clientID, x, y, z */

                // printf("Recieved coordinate (%d, %d, %d) from peer id %d\n",
                //        d[1], d[2], d[3], d[0]);

                pPosition =
                    Client::transNetCoordinate(d[0], d[1], d[2], d[3]);

                if(pPosition == NULL) {
                    LOG("Unable to transform coordinate.");
                    break;
                }

                // printf("Coordinate after translation: (%d, %d, %d)\n",
                //        pPosition->x, pPosition->y, pPosition->z);

                Remote::remoteJump(pPosition->x,
                           pPosition->y,
                           pPosition->z);
                free(pPosition);
                pPosition = NULL;

                break;

            case KIKI_ADVERTISE:
                /* Format: d's: clientId, x offset, y offset, z offset.
                 *         f's: x scale, y scale, z scale.
                 *         s: name
                 */

                messageLen = Client::parseInBufferByFmt(-1, "xldfffddds", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                /* LOG("Client id %d received a KIKI_ADVERTISE message: Name: \"%s\" (id %d), scale (%f, %f, %f), offset (%d, %d, %d).",
                 *       state->clientState->myId,
                 *       s,
                 *       d[0],
                 *       f[0],
                 *       f[1],
                 *       f[2],
                 *       d[1],
                 *       d[2],
                 *       d[3]);
                 */

                Client::addPeer(d[0], (char *)s, f[0], f[1], f[2], d[1], d[2], d[3]);

                break;

            case KIKI_SAVEMASTER:
                /*
                 * Format: save master's peer id: d0.
                 */

                messageLen = Client::parseInBufferByFmt(5, "xd", f, s, d, clientState->inBuffer);
                if(messageLen < 0) {
                    goto critical;
                }
                else if(messageLen == 0) {
                    goto loopExit;
                }

                if(d[0] < 0) {//client Id should be positive
                    break; //ignore this message if invalid
                }
                if(state->clientState->myId == (uint32_t)d[0]) {
                    state->clientState->saveMaster = TRUE;
                    LOG("Instance id %d (%s) now autosaving.",
                        state->clientState->myId,
                        state->name);
                }
                else {
                    state->clientState->saveMaster = FALSE;
                }
                break;

            case KIKI_SKELETONFILENAME:
                /*
                 *  Format: revision: d0
                 *          auto increment save counter: d1
                 *          filename: s
                 *
                 */

                messageLen = Client::parseInBufferByFmt(-1, "xldds", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;
                Skeletonizer::updateSkeletonFileName(d[0], d[1], (char *)s);

                break;

            case KIKI_WITHDRAW:
                messageLen = Client::parseInBufferByFmt(5, "xd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                LOG("Received KIKI_WITHDRAW message for id %d", d[0]);

                Client::delPeer(d[0]);

                break;

            case KIKI_ADDTREECOMMENT:
                /* Format: revision, tree id, comment string */

                messageLen = Client::parseInBufferByFmt(-1, "xlds", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                        goto critical;
                else if(messageLen == 0)
                        goto loopExit;

                Skeletonizer::addTreeComment(d[0], d[1], (char *)s);

                break;

            case KIKI_ADDCOMMENT:
                /* Format: revision, node id, comment string */

                messageLen = Client::parseInBufferByFmt(-1, "xldds", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::addComment(d[0], (char *)s, NULL, d[1]);

                break;

            case KIKI_EDITCOMMENT:
                /* Format: revision, node id, new node id, new content */

                messageLen = Client::parseInBufferByFmt(-1, "xlddds", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::editComment(d[0], NULL, d[1], (char *)s, NULL, d[2]);

                break;

            case KIKI_DELCOMMENT:
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::delComment(d[0], NULL, d[1]);

                break;

            case KIKI_ADDNODE:
                /* Format: revision d0
                           peer id d1,
                           node id d2,
                           radius f0,
                           tree id d3,
                           vp type d4,
                           from magnification d5,
                           timestamp d6,
                           position x d7,
                           y d8,
                           z d9 */
                messageLen = Client::parseInBufferByFmt(45, "xdddfddddddd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                pPosition = (Coordinate*)malloc(sizeof(Coordinate));
                if(pPosition == NULL) {
                    LOG("Out of memory.");
                    goto critical;
                }

                pPosition->x = d[7];
                pPosition->y = d[8];
                pPosition->z = d[9];

                Skeletonizer::addNode(d[0], d[2], f[0], d[3], pPosition, (Byte)d[4], d[5], d[6], FALSE);

                free(pPosition);
                pPosition = NULL;

                break;

            case KIKI_EDITNODE:
                /* Format: revision, peer id, nodeid, radius, from magnification, position x, y, z */
                messageLen = Client::parseInBufferByFmt(33, "xdddfdddd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::editNode(d[0], d[2], NULL, f[0], d[4], d[5], d[6], d[3]);

                break;

            case KIKI_DELNODE:
                /* Format: revision, id */
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                LOG("DELNODE: Received revision %d", d[0]);

                Skeletonizer::delNode(d[0], d[1], NULL);

                break;

            case KIKI_ADDSEGMENT:
                /* Format: revision, source node, target node */
                messageLen = Client::parseInBufferByFmt(13, "xddd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::addSegment(d[0], d[1], d[2]);

                break;

            case KIKI_DELSEGMENT:
                /* Format: Revision, source node, target node */
                messageLen = Client::parseInBufferByFmt(13, "xddd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::delSegment(d[0], d[1], d[2], NULL);

                break;

            case KIKI_ADDTREE:
                messageLen = Client::parseInBufferByFmt(21, "xddfff", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                color4F treeCol;
                treeCol.r = f[0];
                treeCol.g = f[1];
                treeCol.b = f[2];
                treeCol.a = 1.;
                Skeletonizer::addTreeListElement(TRUE, d[0], d[1], treeCol);

                break;

            case KIKI_DELTREE:
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::delTree(d[0], d[1]);

                break;

            case KIKI_MERGETREE:
                messageLen = Client::parseInBufferByFmt(13, "xddd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::mergeTrees(d[0], d[1], d[2]);

                break;

            case KIKI_SPLIT_CC:
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::splitConnectedComponent(d[0], d[1]);

                LOG("Called splitcc with %d %d", d[0], d[1]);

                break;

            case KIKI_PUSHBRANCH:
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::pushBranchNode(d[0], TRUE, TRUE, NULL, d[1]);

                break;

            case KIKI_POPBRANCH:
                messageLen = Client::parseInBufferByFmt(5, "xd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::popBranchNode(d[0]);

                break;

            case KIKI_CLEARSKELETON:
                messageLen = Client::parseInBufferByFmt(5, "xd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::clearSkeleton(d[0], FALSE);

                break;

            case KIKI_SETACTIVENODE:
                /* Format: Revision, node id */
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                Skeletonizer::setActiveNode(d[0], NULL, d[1]);

                break;

            case KIKI_SETSKELETONMODE:
                /* Format: Revision, work mode */
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                printf("SETSKELETONMODE: Received revision %d / mode %d\n", d[0], d[1]);
                Skeletonizer::setSkeletonWorkMode(d[0], d[1]);

                break;

            default:
                /*
                 * BUG
                 *
                 * The current handling of this situation is very bad. The
                 * connection should be terminated in this case.
                 */

                LOG("Critical error.");
                messageLen = 5;
        }

        /* Remove the message we just interpreted from the input buffer */
        memmove(clientState->inBuffer->data, &clientState->inBuffer->data[messageLen], clientState->inBuffer->length - messageLen);
        clientState->inBuffer->length = clientState->inBuffer->length - messageLen;
        memset(&clientState->inBuffer->data[clientState->inBuffer->length], '\0', clientState->inBuffer->size - clientState->inBuffer->length);
        messageLen = 0;

    }

loopExit:

    return TRUE;

critical:
    LOG("Error parsing remote input.");
    Client::skeletonSyncBroken();
    return FALSE;
}

static bool flushOutBuffer() {
    //Can this block at inconvenient times?

    clientState *clientState = state->clientState;

    state->protectOutBuffer->lock();

    if(clientState->outBuffer->length > 0) {
        // check return value
        //SDLNet_TCP_Send(clientState->remoteSocket, SDL TODO
        //        state->clientState->outBuffer->data,
        //       state->clientState->outBuffer->length);
        memset(clientState->outBuffer->data, '\0', clientState->outBuffer->size);
        clientState->outBuffer->length = 0;
    }

    state->protectOutBuffer->lock();
    return TRUE;
}

static bool cleanUpClient() {
    free(state->clientState);
    state->clientState = NULL;

    return TRUE;
}

static int32_t clientRun() {
    clientState *clientState = state->clientState;
    Byte *message = NULL;
    uint32_t messageLen = 0, nameLen = 0, readLen = 0;
    SDL_Event autoSaveOffEvent;

    message = (Byte*)malloc(8192 * sizeof(Byte));
    if(message == NULL) {
        LOG("Out of memory");
        _Exit(FALSE);
    }
    memset(message, '\0', 8192 * sizeof(Byte));

    autoSaveOffEvent.type = SDL_USEREVENT;
    autoSaveOffEvent.user.code = USEREVENT_NOAUTOSAVE;

    if(connectToServer() == TRUE) {
        /*
         * Autosave is turned off. Whether a knossos instance autosaves or
         * does not autosave during synchronization is determined by the "save
         * master" state which is dynamically agreed between the synchronizing
         * instances. The user can turn autosave back on manually to force
         * autosave by a synchronizing instance.
         * Every synchronizing knossos instance starts out with save master flag
         * set but gives away the flag as soon as another knossos instance with
         * higher save priority (lower mag factor, lower peer id) is
         * encountered.
         *
         */

        //if(SDL_PushEvent(&autoSaveOffEvent) == FAIL)
        //    LOG("SDL_PushEvent returned -1"); SDL TODO

        /*
         * Compose HI message
         */

        /*
         * strlen(name) bytes for the name, 3 floats, 3 ints, 5 bytes "header"
         */
        if(strlen(state->name) > (8192 - 5 - 6 * 4)) {
        }
        else {
            nameLen = strlen(state->name);
        }
        messageLen = nameLen + 6 * 4 + 5;
        message[0] = KIKI_HI;
        Client::integerToBytes(&message[1], messageLen);
        strncpy((char *)&message[5], state->name, nameLen);
        Client::floatToBytes(&message[5 + nameLen], state->scale.x);
        Client::floatToBytes(&message[5 + nameLen + 4], state->scale.y);
        Client::floatToBytes(&message[5 + nameLen + 8], state->scale.z);
        Client::integerToBytes(&message[5 + nameLen + 12], state->offset.x);
        Client::integerToBytes(&message[5 + nameLen + 16], state->offset.y);
        Client::integerToBytes(&message[5 + nameLen + 20], state->offset.z);

        Client::IOBufferAppend(clientState->outBuffer, message, messageLen, state->protectOutBuffer);
        memset(message, '\0', 8192 * sizeof(Byte));

        while(TRUE) {
            /*
             * Slowing this loop down might be a good idea if it proves
             * to eat a lot of power. It probably wouldn't hurt, synchronizing
             * as quickly as we can isn't that important.
             */

            /*
             * Write all pending outgoing data to the socket
             */
            flushOutBuffer();

            /*
             * Read all incoming data into the read buffer
             * We're using CheckSockets and the socket set here because there
             * seems to be no way to read with a given timeout and it's not
             * possible to use non-blocking sockets with SDLNet.
             * This solution should effectively behave like doing a read with a
             * 100 ms timeout.
             */
            //SDLNet_CheckSockets(clientState->socketSet, 100); TODO SDLNet_Checksockets immediate crash

            //if(SDLNet_SocketReady(clientState->remoteSocket))
            if(true){
                //readLen = SDLNet_TCP_Recv(clientState->remoteSocket, message, 8192); //TODO SDLNet_TCP_Recv crash
                if(readLen == 0) {
                    LOG("Remote server closed the connection.");
                    break;
                }
                /*
                printf("Raw message from socket: ");
                for(i=0; i<readLen;i++)
                    printf("%dd ", message[i]);
                printf("\n");
                printf("SDLNet_TCP_Recv() returned %d\n", readLen);
                */

                Client::IOBufferAppend(clientState->inBuffer, message, readLen, NULL);
                memset(message, '\0', 8192);
            }

            /*
             * Interpret what we have in the read buffer
             */
            parseInBuffer();

            state->protectClientSignal->lock();
            if(state->clientSignal == TRUE) {
                state->clientSignal = FALSE;
                state->protectClientSignal->lock();
                break;
            }
            state->protectClientSignal->lock();
        }
    }

    state->clientState->saveMaster = FALSE;
    if(!state->skeletonState->autoSaveBool) {
       // AG_TextMsg(AG_MSG_INFO, AGAR TODO
       //            "Synchronization has ended and this instance "
       //            "will currently not autosave. Please turn autosave "
       //            "on manually if it is required.");
    }

    closeConnection();

    return TRUE;
}


void Client::start() {
    clientState *clientState = state->clientState;

    // Workaround to make --connect-asap work (on windows?)
    // We need to wait for the viewer thread to become ready.

    //while(!state->viewerState->viewerReady || state->viewerState->splash) {
    //    Sleeper::msleep(50);
    //}
    //if(state->clientState->connectAsap) {
    //    Knossos::sendClientSignal();
    //}
    int i = 1;
    while(i < 5) {
        Knossos::sendLoadSignal(i*100, i*100, i*100, NO_MAG_CHANGE);
        i++;
        Sleeper::msleep(500);
        /*state->protectClientSignal->lock();
        while(state->clientSignal == FALSE) {
        //    SDL_CondWait(state->conditionClientSignal, state->protectClientSignal);
        }
        state->clientSignal = FALSE;
        state->protectClientSignal->unlock();

        if(state->quitSignal == true) {
            break;
        }

        //  Update state.


        clientState->synchronizeSkeleton = tempConfig->clientState->synchronizeSkeleton;
        clientState->synchronizePosition = tempConfig->clientState->synchronizePosition;
        clientState->remotePort = tempConfig->clientState->remotePort;
        strncpy(clientState->serverAddress, tempConfig->clientState->serverAddress, 1024);


         //  Workaround because SDL_PushEvent in clientRun() will fail with no
         //  error if the connection is established as fast as possible.

        clientRun();

        if(state->quitSignal == true) {
            break;
        }*/
    }

    cleanUpClient();
}

bool Client::broadcastPosition(uint32_t x, uint32_t y, uint32_t z) {
    clientState *clientState = state->clientState;
    Byte *data = NULL;
    uint32_t messageLen;

    data = (Byte*)malloc(128 * sizeof(Byte));
    if(data == NULL) {
        LOG("Out of memory.");
        _Exit(FALSE);
    }
    memset(data, '\0', 128 * sizeof(Byte));

    messageLen = 22;

    data[0] = KIKI_REPEAT;
    Client::integerToBytes(&data[1], messageLen);
    data[5] = KIKI_POSITION;
    Client::integerToBytes(&data[6], clientState->myId);
    Client::integerToBytes(&data[10], x);
    Client::integerToBytes(&data[14], y);
    Client::integerToBytes(&data[18], z);

    Client::IOBufferAppend(clientState->outBuffer, data, messageLen, state->protectOutBuffer);

    free(data);

    return TRUE;
}

bool Client::skeletonSyncBroken() {
    LOG("Skeletons have gone out of sync, stopping synchronization.");
    state->clientState->synchronizeSkeleton = FALSE;
    return TRUE;
}

int32_t Client::bytesToInt(Byte *source) {
    int32_t dest = 0;
    memcpy(&dest, source, sizeof(int32_t));
    return dest;
}

bool Client::integerToBytes(Byte *dest, int32_t source) {
    memcpy(dest, &source, sizeof(int32_t));
    return TRUE;
}

bool Client::floatToBytes(Byte *dest, float source) {
    memcpy(dest, &source, sizeof(float));
    return TRUE;
}

int Client::Wrapper_SDLNet_TCP_Open(void *params) {
    clientState *cState = (clientState*)params;
  //  clientState->remoteSocket = SDLNet_TCP_Open(&(clientState->remoteServer));
    cState->connectionTried = TRUE;
    return TRUE;
}

bool Client::IOBufferAppend(struct IOBuffer *iobuffer, Byte *data, uint32_t length, QMutex *mutex) {
    uint32_t newSize = 0, minBufferSize = 0;
    Byte *newDataPtr = NULL;

    if(mutex != NULL) {
        mutex->lock();
    }
    /* Check if the buffer is large enough for us to append the data.
     * If it isn't we try to allocate larger chunks of memory for the buffer
     * until MAX_BUFFER_SIZE is reached. If this is defined as something
     * reasonably large, there's probably a network error if we reach it.
     */

    if(length > (iobuffer->size - iobuffer->length)) {
        /* The buffer is not large enough to accomodate the data we want to
         * append to it */
        if(iobuffer->size < MAX_BUFFER_SIZE) {
            minBufferSize = iobuffer->length + length;
            /* We can still make it bigger */
            newSize = iobuffer->size;
            do {
                /* Adding 1 is essential because nextpow2 will not return the
                 * next power of two if the argument already is a power of two.
                 * (It is expected to work that way for the hashtable)
                 *
                 */
                newSize = Hashtable::nextpow2(newSize + 1);
            } while(newSize <= minBufferSize && newSize <= MAX_BUFFER_SIZE);

            if(newSize >= MAX_BUFFER_SIZE) {
                LOG("Reached MAX_BUFFER_SIZE while trying to enlarge IO buffer.");
                if(mutex != NULL) {
                    mutex->unlock();
                }
                return FALSE;
            }

            LOG("Enlarging IO buffer %p to %d", iobuffer, newSize);

            newDataPtr = (Byte*)realloc(iobuffer->data, newSize);
            if(newDataPtr == NULL) {
                LOG("Unable to modify buffer size (realloc failed).");
                if(mutex != NULL) {
                    mutex->unlock();
                }
                return FALSE;
            }

            iobuffer->size = newSize;
            iobuffer->data = newDataPtr;
        }
        else {
            /* Nothing we can do here, this probably indicates a network error
             * so the FALSE return value from this function should be used to
             * close the connection */
            LOG("Maximum buffer size for %p reached. This probably indicates a network error.", iobuffer);
            if(mutex != NULL) {
                mutex->unlock();
            }
            return FALSE;
        }
    }

    memcpy(&iobuffer->data[iobuffer->length], data, length);
    iobuffer->length += length;

    /*
    printf("iobuffer->data[]: ");
    for(i=0;i<iobuffer->length;i++)
        printf("%dd ", iobuffer->data[i]);
    printf("\n");
    printf("iobuffer->length = %d\n", iobuffer->length);

    printf("added %d bytes to outbuffer\n", length);
    */

    if(mutex != NULL) {
        mutex->unlock();
    }
    return TRUE;
}

bool Client::addPeer(uint32_t id, char *name,
                     float xScale, float yScale, float zScale,
                     int32_t xOffset, int32_t yOffset, int32_t zOffset) {
    struct clientState *clientState = state->clientState;
    struct peerListElement *oldPeer = NULL, *newPeer = NULL;

    newPeer = (peerListElement*)malloc(sizeof(struct peerListElement));
    if(newPeer == NULL) {
        printf("3Out of memory\n");
        _Exit(FALSE);
    }
    memset(newPeer, '\0', sizeof(struct peerListElement));

    if(clientState->firstPeer == NULL) {
        clientState->firstPeer = newPeer;
        oldPeer = newPeer;
    }
    else {
        oldPeer = clientState->firstPeer;

        while(oldPeer->next != NULL) {
            oldPeer = oldPeer->next;
        }
    }

    oldPeer->next = newPeer;
    newPeer->next = NULL;

    newPeer->id = id;
    newPeer->name = name;
    SET_COORDINATE(newPeer->scale, xScale, yScale, zScale);
    SET_COORDINATE(newPeer->offset, xOffset, yOffset, zOffset);

    return TRUE;
}


bool Client::delPeer(uint32_t id) {
    printf("delPeer() is not implemented.\n");
    return TRUE;
}

bool Client::syncMessage(char *fmt, ...) {
    /* Many thanks to the va_start(3) manpage. */

    /*
     * This function allows writing different data types into an array on
     * the byte level using a format string. That is, data in the array will
     * not be aligned to a specific multiple of a byte as in single-type
     * arrays.
     */

    /*
     * The properly packed data will be appended to the outgoing network data
     * buffer. Proper locking is ensured through IOBufferAppend().
     */

    /* Returning FALSE from this function means that synchronized skeletons have
     * gone out of sync and this should be handled accordingly (usually by
     * closing the connection).
     */

#define PACKLEN 512

    va_list ap;
    Byte *packedBytes = NULL;
    int32_t len = 5;  // This is where the message part after KIKI_REPEAT and
                      // the message length field begins.
    int d;
    Byte b;
    char *s;
    float f;
    int32_t peerLenField = -1;

    if(!state->clientState->synchronizeSkeleton ||
       !state->clientState->connected) {
        return TRUE;
    }
    packedBytes = (Byte*)malloc(PACKLEN * sizeof(Byte));
    if(packedBytes == NULL) {
        printf("5Out of memory\n");
        _Exit(FALSE);
    }

    memset(packedBytes, '\0', PACKLEN * sizeof(Byte));

    packedBytes[0] = KIKI_REPEAT;

    va_start(ap, fmt);

    while(*fmt) {
        switch(*fmt++) {
            case 'd':
                len += 4;
                if(len >= PACKLEN) {
                    goto lenoverflow;
                }
                d = va_arg(ap, int32_t);

                Client::integerToBytes(&packedBytes[len - 4], d);
                break;

            case 'b':
                len += 1;
                if(len >= PACKLEN) {
                    goto lenoverflow;
                }
                b = (Byte)va_arg(ap, int32_t);

                packedBytes[len - 1] = b;
                break;

            case 'f':
                len += 4;
                if(len >= PACKLEN) {
                    goto lenoverflow;
                }
                f = (float)va_arg(ap, double);

                floatToBytes(&packedBytes[len - 4], f);
                break;

            case 's':
                s = va_arg(ap, char *);
                len += strlen(s);
                if(len >= PACKLEN) {
                    goto lenoverflow;
                }
                memcpy(&packedBytes[len - strlen(s)], s, strlen(s));
                break;
            case 'r':
                /* Insert the target skeleton revision number here. */
                len += 4;
                if(len > PACKLEN) {
                    goto lenoverflow;
                }
                Client::integerToBytes(&packedBytes[len - 4], state->skeletonState->skeletonRevision);
                //printf("sending delta to revision %d\n", *(int *)(&packedBytes[len - 4]));
                break;
            case 'l':
                /* Insert the length _without_ the KIKI_REPEAT and total length
                 * fields here.
                 *
                 * This could be confusing: The kiki server expects the total
                 * message length as an integer (4 bytes) after the first byte (after
                 * KIKI_REPEAT here) but relays only the bytes after those first
                 * 5 bytes to the peers. If messages are non-fixed length, that
                 * is, if they contain non-fixed length strings, those messages
                 * will have to contain another length field in order to be
                 * parsed by the peers. This is what 'l' stands for here.
                 */
                peerLenField = len;
                len += 4;
                if(len > PACKLEN) {
                    goto lenoverflow;
                }
                break;
        }
    }

    va_end(ap);

    Client::integerToBytes(&packedBytes[1], len);
    if(peerLenField >= 0) {
        Client::integerToBytes(&packedBytes[peerLenField], len - 5);
    }
    if(!Client::IOBufferAppend(state->clientState->outBuffer,
                       packedBytes,
                       len,
                       state->protectOutBuffer)) {
        return FALSE;
    }
    return TRUE;

lenoverflow:

    va_end(ap);
    LOG("Length overflow in client.c, syncMessage(). Tell the programmers to find the bug or increase PACKLEN.");
    return FALSE;
}

int32_t Client::parseInBufferByFmt(int32_t len, const char *fmt,
                                   float *f, Byte *s, int32_t *d,
                                   struct IOBuffer *buffer) {
    int32_t fIndex = -1;
    int32_t dIndex = -1;
    int32_t pos = 0;
    int32_t stringLen = 0;

    memset(s, '\0', 2048 * sizeof(Byte));

    /* If len is not specified as a parameter (len == -1) then bytes 1 to 5 will
     * be interpreted as len (int);
     */

    /* Return values:
     *     -1: Fatal error, connection should subsequently be closed.
     *      0: Buffer to small to parse complete message
     *  n > 0: Message successfully parsed, n bytes processed.
     */

    if(len < 0) {
        len = bytesToInt(&buffer->data[1]);
    }
    else if(len == 0) {
        /* This should never happen but would lead to an infinite loop. */
        LOG("Invalid message length from peer.");
        return FAIL;
    }

    else if(buffer->length < (uint32_t)len) {
        /*printf("In parseInBufferByFmt(): buffer->length = %d, len = %d. Shit.\n",
                buffer->length, len);

        printf("contents of input buffer: \n");
        for(i=0; i<buffer->length; i++) {
            printf("%dd ", buffer->data[i]);
        }
        printf("\n");
        */

        return FALSE;
    }

    while(*fmt) {
        switch(*fmt++) {
            case 's':
                if((len - pos) <= 2047) {
                    stringLen = len - pos;
                }
                else {
                    stringLen = 2047;
                }
                strncpy((char *)s, (char *)&buffer->data[pos], stringLen);
                pos += stringLen;
                break;
            case 'd':
                if(++dIndex >= INTS || (pos += 4) > len) {
                    goto overflow;
                }
                d[dIndex] = Client::bytesToInt(&buffer->data[pos - 4]);
                break;
            case 'f':
                if(++fIndex >= FLOATS || (pos += 4) > len) {
                    goto overflow;
                }
                f[fIndex] = bytesToFloat(&buffer->data[pos - 4]);
                break;
            case 'x':
                /* Single byte spacer */
                if(++pos >= len) {
                    goto overflow;
                }
                break;
            case 'l':
                /* Four byte spacer */
                if((pos += 4) >= len) {
                    goto overflow;
                }
                break;
        }
    }
    return len;

overflow:
    LOG("Overflow in client.c, parseInBufferByFmt(). Tell the developers to fix the bug.");
    return FAIL;
}

Coordinate* Client::transNetCoordinate(uint32_t id, int32_t x, uint32_t y, int32_t z) {
    clientState *clientState = state->clientState;
    peerListElement *peer = clientState->firstPeer;
    Coordinate *outCoordinate = NULL;

    while(peer != NULL) {
        if(peer->id == id) {
            break;
        }
        peer = peer->next;
    }

    outCoordinate = (Coordinate*)malloc(sizeof(Coordinate));
    if(outCoordinate == NULL) {
        printf("4Out of memory\n");
        _Exit(FALSE);
    }
    memset(outCoordinate, '\0', sizeof(Coordinate));

    if(peer == NULL) {
        LOG("Peer with id %d is not in peers list. That's a KIKI protocol violation and probably indicates a more serious bug somewhere else.", id);
        outCoordinate->x = x;
        outCoordinate->y = y;
        outCoordinate->z = z;
        return outCoordinate;
    }

    Coordinate::transCoordinate(outCoordinate, x, y, z, peer->scale, peer->offset);

    return outCoordinate;
}
