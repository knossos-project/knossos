/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#include "skeletonizer.h"
#include "remote.h"
#include "knossos.h"
#include "viewer.h"
#include "client.h"
#include "sleeper.h"

extern stateInfo *state;
extern stateInfo *tempConfig;

Client::Client(QObject *parent) :
    QObject(parent)
{
    state->clientState->remoteSocket = new QTcpSocket();
    connect(state->clientState->remoteSocket, SIGNAL(connected()), this, SLOT(socketConnectionSucceeded()));
    connect(state->clientState->remoteSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketConnectionFailed(QAbstractSocket::SocketError)));
}

/**
 * This method is a replacement for the SDL_NET functionality.
 * @todo is KIKI a local server in tools/kiki.py ?
 * @todo checking if the socket is executed in a separete thread
 * a replacement for the SocketSet functionality
 */
static bool connectToServer() {
    //int timeoutIn100ms = roundFloat((float) state->clientSignal.connectionTimeout / 100.);
    int timeoutIn100ms = 30; // ??

    // lookup the host
    QHostInfo hostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    if(hostInfo.error() != QHostInfo::NoError) {
        qDebug() << hostInfo.errorString();
        return false;
    }

    // connect to host
    state->clientState->remoteSocket->connectToHost(hostInfo.hostName(), state->clientState->remotePort);
    state->clientState->socketSet = new QSet<QTcpSocket *>();
    state->clientState->socketSet->insert(state->clientState->remoteSocket);

    // AG_LabelText(state->viewerState->ag->syncOptLabel, "Connected to server.");


    return true;
}

static bool closeConnection() {
    if(state->clientState->remoteSocket != NULL) {
           state->clientState->remoteSocket->close();
       }

       delete(state->clientState->socketSet);

       state->clientState->connected = false;


      state->clientState->connected = false;
      //AG_LabelText(state->viewerState->ag->syncOptLabel, "No connection to server.");
      return true;

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
uint32_t Client::parseInBuffer() {
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
                emit remoteJumpSignal(pPosition->x, pPosition->y, pPosition->z);

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
                    state->clientState->saveMaster = true;
                    LOG("Instance id %d (%s) now autosaving.",
                        state->clientState->myId,
                        state->name);
                }
                else {
                    state->clientState->saveMaster = false;
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

                emit updateSkeletonFileNameSignal(d[0], d[1], (char *)s);
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

                emit addTreeCommentSignal(d[0], d[1], (char *)s);
                //Skeletonizer::addTreeComment(d[0], d[1], (char *)s);

                break;

            case KIKI_ADDCOMMENT:
                /* Format: revision, node id, comment string */

                messageLen = Client::parseInBufferByFmt(-1, "xldds", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                emit addCommentSignal(d[0], (char *)s, NULL, d[1]);

                break;

            case KIKI_EDITCOMMENT:
                /* Format: revision, node id, new node id, new content */

                messageLen = Client::parseInBufferByFmt(-1, "xlddds", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                emit editCommentSignal(d[0], NULL, d[1], (char *)s, NULL, d[2]);

                break;

            case KIKI_DELCOMMENT:
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                emit delCommentSignal(d[0], NULL, d[1]);

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

                Skeletonizer::addNode(d[0], d[2], f[0], d[3], pPosition, (Byte)d[4], d[5], d[6], false);

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

                emit editNodeSignal(d[0], d[2], NULL, f[0], d[4], d[5], d[6], d[3]);

                break;

            case KIKI_DELNODE:
                /* Format: revision, id */
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                LOG("DELNODE: Received revision %d", d[0]);

                emit delNodeSignal(d[0], d[1], NULL);

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

                emit delSegmentSignal(d[0], d[1], d[2], NULL);

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
                Skeletonizer::addTreeListElement(true, d[0], d[1], treeCol);

                break;

            case KIKI_DELTREE:
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                emit delTreeSignal(d[0], d[1]);

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

                Skeletonizer::pushBranchNode(d[0], true, true, NULL, d[1]);

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

                emit clearSkeletonSignal(d[0], false);

                break;

            case KIKI_SETACTIVENODE:
                /* Format: Revision, node id */
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                emit setActiveNodeSignal(d[0], NULL, d[1]);

                break;

            case KIKI_SETSKELETONMODE:
                /* Format: Revision, work mode */
                messageLen = Client::parseInBufferByFmt(9, "xdd", f, s, d, clientState->inBuffer);
                if(messageLen < 0)
                    goto critical;
                else if(messageLen == 0)
                    goto loopExit;

                printf("SETSKELETONMODE: Received revision %d / mode %d\n", d[0], d[1]);
                emit skeletonWorkModeSignal(d[0], d[1]);

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

    return true;

critical:
    LOG("Error parsing remote input.");
    Client::skeletonSyncBroken();
    return false;
}

/**
 * @test Byte * is converted to char *
 */
static bool flushOutBuffer() {

    state->protectOutBuffer->lock();

    if(state->clientState->outBuffer->length > 0) {
        char *content = (char *)state->clientState->outBuffer->data;
        state->clientState->remoteSocket->write(content, state->clientState->outBuffer->length);
        state->clientState->remoteSocket->flush();
        memset(state->clientState->outBuffer->data, '\0', state->clientState->outBuffer->length);
        state->clientState->outBuffer->length = 0;
    }

    state->protectOutBuffer->unlock();
    return true;
}

static bool cleanUpClient() {
    free(state->clientState);
    state->clientState = NULL;

    return true;
}

/**
 * @todo autoSaveOffEvent
 * @todo pushEvent
 * @test byte buffer is temporarily converted to char *(because of the socket read method) and backwards
 */
bool Client::clientRun() {
    clientState *clientState = state->clientState;
    Byte *message = NULL;
    uint32_t messageLen = 0, nameLen = 0, readLen = 0;
    //SDL_Event autoSaveOffEvent;

    message = (Byte*)malloc(8192 * sizeof(Byte));
    if(message == NULL) {
        LOG("Out of memory");
        _Exit(false);
    }
    memset(message, '\0', 8192 * sizeof(Byte));

    //autoSaveOffEvent.type = SDL_USEREVENT;
    //autoSaveOffEvent.user.code = USEREVENT_NOAUTOSAVE;

    if(connectToServer() == true) {
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

        while(true) {
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


            if(state->clientState->remoteSocket->isValid()) {
                char *msg = "";
                readLen = state->clientState->remoteSocket->read(msg, 8192);
                message = (Byte *) msg;
                if(readLen == 0) {
                    LOG("Remote server closed the connection.");
                    break;
                }


                Client::IOBufferAppend(clientState->inBuffer, message, readLen, NULL);
                memset(message, '\0', 8192);
            }

            /*
             * Interpret what we have in the read buffer
             */
            parseInBuffer();

            state->protectClientSignal->lock();
            if(state->clientSignal == true) {
                state->clientSignal = false;
                state->protectClientSignal->lock();
                break;
            }
            state->protectClientSignal->lock();
        }
    }

    state->clientState->saveMaster = false;
    if(!state->skeletonState->autoSaveBool) {
       // AG_TextMsg(AG_MSG_INFO, AGAR TODO
       //            "Synchronization has ended and this instance "
       //            "will currently not autosave. Please turn autosave "
       //            "on manually if it is required.");
    }

    closeConnection();

    return true;
}

/**
 * This method is a replacement for the SDL_NET functionality.
 */
void Client::start() {
    clientState *clientState = state->clientState;

    while(!state->viewerState->viewerReady or state->viewerState->splash) {
        Sleeper::msleep(50);
    }

    if(clientState->connectAsap) {
        Knossos::sendClientSignal();
    }

    for(int i = 1; i < 5; i++) {
        //viewerEventObj->sendLoadSignal(i * 100, i * 100, i * 100, NO_MAG_CHANGE);
        Sleeper::sleep(500);
        state->protectClientSignal->lock();
        while(!state->clientSignal) {
            state->conditionClientSignal->wait(state->protectClientSignal);
        }

        state->clientSignal = false;
        state->protectClientSignal->unlock();

        if(state->quitSignal == true) {
            break;
        }

        clientState->synchronizeSkeleton = tempConfig->clientState->synchronizeSkeleton;
        clientState->synchronizePosition = tempConfig->clientState->synchronizePosition;
        clientState->remotePort = tempConfig->clientState->remotePort;
        strncpy(clientState->serverAddress, tempConfig->clientState->serverAddress, 1024);

        clientRun();

        if(state->quitSignal == false) {
            break;
        }
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
        _Exit(false);
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

    return true;
}

bool Client::skeletonSyncBroken() {
    LOG("Skeletons have gone out of sync, stopping synchronization.");
    state->clientState->synchronizeSkeleton = false;
    return true;
}

int32_t Client::bytesToInt(Byte *source) {
    int32_t dest = 0;
    memcpy(&dest, source, sizeof(int32_t));
    return dest;
}

bool Client::integerToBytes(Byte *dest, int32_t source) {
    memcpy(dest, &source, sizeof(int32_t));
    return true;
}

bool Client::floatToBytes(Byte *dest, float source) {
    memcpy(dest, &source, sizeof(float));
    return true;
}

int Client::Wrapper_SDLNet_TCP_Open(void *params) {
    clientState *cState = (clientState*)params;
  //  clientState->remoteSocket = SDLNet_TCP_Open(&(clientState->remoteServer));
    cState->connectionTried = true;
    return true;
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
                return false;
            }

            LOG("Enlarging IO buffer %p to %d", iobuffer, newSize);

            newDataPtr = (Byte*)realloc(iobuffer->data, newSize);
            if(newDataPtr == NULL) {
                LOG("Unable to modify buffer size (realloc failed).");
                if(mutex != NULL) {
                    mutex->unlock();
                }
                return false;
            }

            iobuffer->size = newSize;
            iobuffer->data = newDataPtr;
        }
        else {
            /* Nothing we can do here, this probably indicates a network error
             * so the false return value from this function should be used to
             * close the connection */
            LOG("Maximum buffer size for %p reached. This probably indicates a network error.", iobuffer);
            if(mutex != NULL) {
                mutex->unlock();
            }
            return false;
        }
    }

    memcpy(&iobuffer->data[iobuffer->length], data, length);
    iobuffer->length += length;

    if(mutex != NULL) {
        mutex->unlock();
    }
    return true;
}

bool Client::addPeer(uint32_t id, char *name,
                     float xScale, float yScale, float zScale,
                     int32_t xOffset, int32_t yOffset, int32_t zOffset) {
    struct clientState *clientState = state->clientState;
    struct peerListElement *oldPeer = NULL, *newPeer = NULL;

    newPeer = (peerListElement*)malloc(sizeof(struct peerListElement));
    if(newPeer == NULL) {
        printf("3Out of memory\n");
        _Exit(false);
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

    return true;
}


bool Client::delPeer(uint32_t id) {
    qDebug() << "delPeer() is not implemented.\n";
    return true;
}

bool Client::syncMessage(const char *fmt, ...) {
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

    /* Returning false from this function means that synchronized skeletons have
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
        return true;
    }
    packedBytes = (Byte*)malloc(PACKLEN * sizeof(Byte));
    if(packedBytes == NULL) {
        printf("5Out of memory\n");
        _Exit(false);
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
        return false;
    }
    return true;

lenoverflow:

    va_end(ap);
    LOG("Length overflow in client.c, syncMessage(). Tell the programmers to find the bug or increase PACKLEN.");
    return false;
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

        return false;
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
        _Exit(false);
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


void Client::socketConnectionSucceeded() {
    LOG("Socket connection established");
}

void Client::socketConnectionFailed(QAbstractSocket::SocketError error) {
    LOG(state->clientState->remoteSocket->errorString().toStdString().c_str());

}
