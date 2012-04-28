#
#   This file is a part of KNOSSOS.
# 
#   (C) Copyright 2007-2011
#   Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
# 
#   KNOSSOS is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2 of
#   the License as published by the Free Software Foundation.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
#
#
#   For further information, visit http://www.knossostool.org or contact
#       Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
#       Fabian.Svara@mpimf-heidelberg.mpg.de
#

#!/usr/bin/python

import socket
import select
import struct
import sys


class KikiServer:
    KIKI_REPEAT = chr(0)
    KIKI_CONNECT = chr(1)
    KIKI_DISCONNECT = chr(2)
    KIKI_HI = chr(3)
    KIKI_HIBACK = chr(4)
    KIKI_BYE = chr(5)
    KIKI_ADVERTISE = chr(6)
    KIKI_WITHDRAW = chr(7)
    KIKI_SAVEMASTER = chr(26)
    
    # These are not used by the kiki server but are listed for documentation
    # purposes.
    KIKI_POSITION = chr(8)
    KIKI_ADDCOMMENT = chr(9)
    KIKI_EDITCOMMENT = chr(10)
    KIKI_DELCOMMENT = chr(11)
    KIKI_ADDNODE = chr(12)
    KIKI_EDITNODE = chr(13)
    KIKI_DELNODE = chr(14)
    KIKI_ADDSEGMENT = chr(15)
    KIKI_DELSEGMENT = chr(16)
    KIKI_ADDTREE = chr(17)
    KIKI_DELTREE = chr(18)
    KIKI_MERGETREE = chr(19)
    KIKI_SPLIT_CC = chr(20)
    KIKI_PUSHBRANCH = chr(21)
    KIKI_POPBRANCH = chr(22)
    KIKI_CLEARSKELETON = chr(23)

    INFTY = 1e10

    def __init__(self):
        # I/O buffers
        # socket -> buffer string
        self.outMessages = {}
        self.inMessages = {}

        # client socket -> (client id, client name, scale x, scale y, scale z,
        #               offset x, offset y, offset z)
        self.clients = {}

        # client socket -> client id
        self.idToSock = {}

        # sending socket -> [listening socket 1, listening socket 2, ...]
        self.connections = {}

        # listening socket -> [sending socket 1, sending socket 2, ...]
        # This is used to quickly remove disconnected sockets from
        # self.connections.
        self.reverseConnections = {}

        # List of IDs that became freed, highest currently allocated id
        self.freeIDs = []
        self.highID = 0

        # Tuple of the magnification / peer id of the current save master. This
        # is used to determine if a new instances should become save master. It
        # will become save master if the magnification is lower or if the
        # magnification is the same and the peer id is lower.
        self.curSavemaster = self.INFTY
        self.curHighestSavePriority = [self.INFTY, self.INFTY]

    def go(self, portNumber = 7890):
        self.serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serverSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.serverSocket.bind(("localhost", portNumber))
        self.serverSocket.setblocking(0)
        self.serverSocket.listen(5)

        print("KIKI r374 waiting for KNOSSOS connections")
        while True:
            self.serverLoop()
        
    def serverLoop(self):
        writeTo = []
        try:
            for messageTo in self.outMessages.iterkeys():
                if len(self.outMessages[messageTo]) > 0:
                    writeTo.append(messageTo)
            readFrom = self.clients.keys()
            readFrom.append(self.serverSocket)
        except:
            for messageTo in self.outMessages.keys():
                if len(self.outMessages[messageTo]) > 0:
                    writeTo.append(messageTo)
            readFrom = self.clients.copy().keys()
            readFrom.append(self.serverSocket)

        try:
            readReady, writeReady, errors = select.select(readFrom, writeTo, [])
        except:
            print("Error " + str(sys.exc_info()[0]))

        # Need to do select error checking and remove offending sockets
        writeTo = []

        # Handle activity on the server socket. This is where new connections
        # are being established.
        if self.serverSocket in readReady:
            # Serversocket is ready, looks like we got a new connection
            (clientSocket, clientAddress) = self.serverSocket.accept()
            print("Accepted connection from " + str(clientAddress))
            self.addClient(clientSocket)
            # Subsequent operations are for client sockets only.
            i = 0
            for currentSock in readReady:
                if currentSock == self.serverSocket:
                    del readReady[i]
                i = i + 1
        
        # Check if the save master needs to be updated
        if self.curHighestSavePriority[1] < self.INFTY:
            if self.curHighestSavePriority[1] != self.curSavemaster:
                self.curSavemaster = self.curHighestSavePriority[1]

                saveMasterString = struct.pack("<ci",
                                               self.KIKI_SAVEMASTER,
                                               self.curSavemaster)

                print("Sending KIKI_SAVEMASTER for id " + str(self.curSavemaster))
                try:
                    for currentTargetClient in self.clients.iterkeys():
                        if self.clients[currentTargetClient][0] < 0:
                            # See comment below.
                            continue

                        self.outMessages[currentTargetClient] = \
                            "".join([self.outMessages[currentTargetClient], saveMasterString])                

                except AttributeError:
                    for currentTargetClient in self.clients.keys():
                        if self.clients[currentTargetClient][0] < 0:
                            # See comment below.
                            continue

                        self.outMessages[currentTargetClient] = \
                            "".join([self.outMessages[currentTargetClient], saveMasterString])
                        
        # Handle clients sending us data.
        for currentClient in readReady:
            try:
                readData = currentClient.recv(4096)
            except:
                print("Error reading from client. " + str(sys.exc_info()))
                readData = ""

            if(len(readData) == 0):
                print(str(self.clients[currentClient][1]) + \
                    " (id " + str(self.clients[currentClient][0]) + ") closed the connection.")
                self.rmClient(currentClient)
                
                i = 0
                for iterClient in readReady:
                    if iterClient == currentClient:
                        del readReady[i]
                    i = i + 1
                
                i = 0
                for iterClient in writeReady:
                    if iterClient == currentClient:
                        del writeReady[i]
                    i = i + 1

                continue

            self.inMessages[currentClient] = \
                "".join([self.inMessages[currentClient], readData])

            # Interpret the data: It's either a message to the server or a
            # message to listening clients, in which case we add it to
            # self.outMessages later.
            processed = self.parseMessage(self.inMessages[currentClient],
                                          currentClient)
            self.inMessages[currentClient] = \
                self.inMessages[currentClient][processed:]

        # Handle client we need to send data to.
        for currentClient in writeReady:
            if(len(self.outMessages[currentClient]) > 0):
                try:
                    sentData = currentClient.send(self.outMessages[currentClient])
                except:
                    print("Error writing to client. " + str(sys.exc_info()))
                    sentData = 0

            if(sentData == 0):
                self.rmClient(currentClient)

                i = 0
                for iterClient in readReady:
                    if iterClient == currentClient:
                        del readReady[i]
                    i = i + 1

                i = 0
                for iterClient in writeReady:
                    if iterClient == currentClient:
                        del writeReady[i]
                    i = i + 1

                continue
 
            self.outMessages[currentClient] = \
                 self.outMessages[currentClient][sentData:]
     
    def parseMessage(self, dataString, currentClient):
        # splice out messages that are for the server, copy the rest into the
        # outMessages dictionary
        # This function must interpret the data string from the beginning. (fifo
        # functionality)
        # Make sure to always return the correct "processed" value

        totalEaten = 0
        ateBytes = 0
        messageLength = 0
        bufferLength = len(dataString)

        while bufferLength >= 5 \
              or dataString[0] == self.KIKI_BYE:

            # 5 bytes is the length of the message type byte plus 4 bytes for the
            # following integer giving the message length or, in the case of
            # KIKI_HIBACK, the client id.
            messageLength = struct.unpack("<xi", dataString[0:5])[0]

            if dataString[0] == self.KIKI_REPEAT:
                if bufferLength >= messageLength:
                    # print "KIKI_REPEAT from " + str(currentClient.getpeername())
                    # print "messageLength = " + str(messageLength)
                    # print "bufferLength = " + str(bufferLength)
                    for listeningClient in self.connections[currentClient]:
                        self.outMessages[listeningClient] = \
                            "".join([self.outMessages[listeningClient],
                            dataString[5:messageLength]])

                    ateBytes = messageLength
                    totalEaten += ateBytes

            elif ( (dataString[0] == self.KIKI_CONNECT) \
                or (dataString[0] == self.KIKI_DISCONNECT) ):
                if bufferLength >= messageLength:
                    clientCount = (messageLength - 5) / 4
                    for clientId in struct.unpack("<" + str(clientCount * "i"),
                                                  dataString[5:messageLength]):

                        targetSocket = self.idToSock[clientId]
                        if(dataString[0] == self.KIKI_CONNECT):
                            print("KIKI_CONNECT from " + \
                                str(currentClient.getpeername()))
                            self.addConnection(currentClient, targetSocket)

                        if(dataString[0] == self.KIKI_DISCONNECT):
                            print("KIKI_DISCONNECT from " + \
                                str(currentClient.getpeername()))
                            self.rmConnection(currentClient, targetSocket)

                        ateBytes = messageLength
                        totalEaten += ateBytes

            elif dataString[0] == self.KIKI_HI:
    
                if bufferLength >= messageLength:
                    print("KIKI_HI from " + str(currentClient.getpeername()))
                    
                    # 5 bytes for the message type / length header, 6 times 4
                    # bytes for the trailing floats and ints. The rest is the
                    # length of the name.
                    nameLength = messageLength - 5 - 6 * 4
                    unpacked = struct.unpack("<" + str(nameLength) + "s3f3i", dataString[5:messageLength])

                    # The client sent us information about its dataset, completing
                    # the connection.
                    self.setupClient(currentClient, unpacked[0], unpacked[1],
                                     unpacked[2], unpacked[3], unpacked[4],
                                     unpacked[5], unpacked[6])

                    # The client ID was assigned by addClient() when the client
                    # first connected.
                    clientID = self.clients[currentClient][0]

                    # Send KIKI_HIBACK message, telling the client about the ID we
                    # assigned it.
                    hibackString = struct.pack("<ci", self.KIKI_HIBACK, clientID)
                    self.outMessages[currentClient] = \
                        "".join([self.outMessages[currentClient], hibackString])

                    # Tell all other clients about this new client
                    newNameLen = len(unpacked[0])
                    messageLen = 5 + 4 + 6 * 4 + newNameLen
                    advertiseString = struct.pack("<cii3f3i" + str(newNameLen) + "s",
                                                  self.KIKI_ADVERTISE,
                                                  messageLen,
                                                  clientID,
                                                  unpacked[1], unpacked[2],
                                                  unpacked[3], unpacked[4],
                                                  unpacked[5], unpacked[6],
                                                  unpacked[0])

                    try:
                        for currentTargetClient in self.clients.iterkeys():
                            if currentTargetClient == currentClient:
                                continue
                            if self.clients[currentTargetClient][0] < 0:
                                # See comment below.
                                continue
                            
                            # print "Kuku advertising id " + str(clientID) + " to " + str(self.clients[currentTargetClient][0])
                            self.outMessages[currentTargetClient] = \
                                "".join([self.outMessages[currentTargetClient], advertiseString]) 
                    except AttributeError:
                        for currentTargetClient in self.clients.keys():
                            if currentTargetClient == currentClient:
                                continue
                            if self.clients[currentTargetClient][0] < 0:
                                continue
                            
                            # print "Kuku advertising id " + str(clientID) + " to " + str(self.clients[currentTargetClient][0])
                            self.outMessages[currentTargetClient] = \
                                "".join([self.outMessages[currentTargetClient], advertiseString]) 

                    # Tell this new client about all currently connected clients
                    # And connect it to all clients ;)
                    try:
                        for currentAdvertisedClient in self.clients.iterkeys():
                            if currentAdvertisedClient == currentClient:
                                continue
                            
                            if self.clients[currentAdvertisedClient][0] < 0:
                                # Positive client id indicates a successfully
                                # negotiated connection. Negative id means a
                                # connection is established but KIKI_HI has not yet
                                # been received.
                                continue

                            currentNameLength = len(self.clients[currentAdvertisedClient][1]) 
                            currentLength = 5 + 4 + currentNameLength + 6 * 4
                            clientID = self.clients[currentAdvertisedClient][0]
                            
                            advertiseString = struct.pack("<cii3f3i" + \
                                                            str(currentNameLength) + "s",
                                                            self.KIKI_ADVERTISE,
                                                            currentLength,
                                                            clientID,
                                                            self.clients[currentAdvertisedClient][2],
                                                            self.clients[currentAdvertisedClient][3],
                                                            self.clients[currentAdvertisedClient][4],
                                                            self.clients[currentAdvertisedClient][5],
                                                            self.clients[currentAdvertisedClient][6],
                                                            self.clients[currentAdvertisedClient][7],
                                                            self.clients[currentAdvertisedClient][1])

                            # print "Kuku advertising id " + str(clientID) + " to " + str(self.clients[currentClient][0])
                            # print "advertising info: " + str(self.clients[currentAdvertisedClient])
        
                            self.outMessages[currentClient] = \
                                "".join([self.outMessages[currentClient], advertiseString])

                            # This will connect every client to every
                            # other available client.

                            print("Adding connection between " + \
                                str(currentClient.getpeername()) + " and " + \
                                str(currentAdvertisedClient.getpeername()))
                            self.addConnection(currentClient, currentAdvertisedClient)
                            self.addConnection(currentAdvertisedClient, currentClient)
                    except AttributeError:
                        for currentAdvertisedClient in self.clients.keys():
                            if currentAdvertisedClient == currentClient:
                                continue

                            if self.clients[currentAdvertisedClient][0] < 0:
                                # Positive client id indicates a successfully
                                # negotiated connection. Negative id means a
                                # connection is established but KIKI_HI has not yet
                                # been received.
                                continue

                            currentNameLength = len(self.clients[currentAdvertisedClient][1]) 
                            currentLength = 5 + 4 + currentNameLength + 6 * 4
                            clientID = self.clients[currentAdvertisedClient][0]
                            
                            advertiseString = struct.pack("<cii3f3i" + \
                                                            str(currentNameLength) + "s",
                                                            self.KIKI_ADVERTISE,
                                                            currentLength,
                                                            clientID,
                                                            self.clients[currentAdvertisedClient][2],
                                                            self.clients[currentAdvertisedClient][3],
                                                            self.clients[currentAdvertisedClient][4],
                                                            self.clients[currentAdvertisedClient][5],
                                                            self.clients[currentAdvertisedClient][6],
                                                            self.clients[currentAdvertisedClient][7],
                                                            self.clients[currentAdvertisedClient][1])

                            # print "Kuku advertising id " + str(clientID) + " to " + str(self.clients[currentClient][0])
                            # print "advertising info: " + str(self.clients[currentAdvertisedClient])
        
                            self.outMessages[currentClient] = \
                                "".join([self.outMessages[currentClient], advertiseString])

                            # This will connect every client to every
                            # other available client.

                            print("Adding connection between " + \
                                str(currentClient.getpeername()) + " and " + \
                                str(currentAdvertisedClient.getpeername()))
                            self.addConnection(currentClient, currentAdvertisedClient)
                            self.addConnection(currentAdvertisedClient, currentClient)

                    ateBytes = messageLength
                    totalEaten += ateBytes

            elif dataString[0] == self.KIKI_BYE:
                print("KIKI_BYE from " + str(currentClient.getpeername()))
                self.rmClient(currentClient)
                ateBytes = 1
                totalEaten += ateBytes

            else:
                # This means any subsequent message from the current client cannot
                # be interpreted correctly anymore.

                print("Unknown message from " + str(currentClient.getpeername()))
                for i in dataString:
                    print(hex(ord(i))),
                ateBytes = len(dataString)
                totalEaten += ateBytes
                break

            dataString = dataString[ateBytes:]
            # print "remaining bytes in dataString: "
            # for i in range(0, len(dataString)):
            #    print "d'" + str(dataString[i]) + "' ",
            # print ""

            bufferLength = len(dataString)
            if bufferLength <= 0:
                break

        return totalEaten

    def addConnection(self, sourceSocket, targetSocket):
        self.connections[sourceSocket].append(targetSocket)
        self.reverseConnections[targetSocket].append(sourceSocket)

    def rmConnection(self, sourceSocket, targetSocket):
        i = 0
        # Handle reverseConnections

        for currentSocket in self.connections[sourceSocket]:
            if currentSocket == targetSocket:
                print("Deleting connection between" \
                      + str(sourceSocket.getpeername()) \
                      +" and " \
                      + str(targetSocket.getpeername()))

                del self.connections[sourceSocket][i]
            i = i + 1

    def addClient(self, socket):
        if len(self.freeIDs) != 0:
            # We can reuse an ID.
            newClientId = self.freeIDs[0]
            del self.freeIDs[0]
        else:
            newClientId = self.highID + 1
            self.highID = self.highID + 1

        print("Assigning id " + str(newClientId) + " to new client " + \
              str(socket.getpeername()))
           
        self.idToSock[newClientId] = socket

        # negative client id means the client has not yet sent KIKI_HI 
        self.clients[socket] = [-newClientId, "", -1, -1, -1, -1, -1, -1]
        self.connections[socket] = []
        self.inMessages[socket] = ""
        self.outMessages[socket] = ""
        self.reverseConnections[socket] = []

    def setupClient(self, socket, name, scaleX, scaleY, scaleZ, offsetX, offsetY, offsetZ):
        print("Client " + str(socket.getpeername()) \
              + " completed the connection with parameters: \n" \
              + "\tname: \"" + name + "\"\n" \
              + "\tscale (x, y, z): (" + str(scaleX) + ", " + str(scaleY) \
              + ", " + str(scaleZ) + ")\n" \
              + "\toffset (x, y, z): (" + str(offsetX) + ", " + str(offsetY) \
              + ", " + str(offsetZ) + ")")
        
        # Switch sign to indicate finalized connection.
        self.clients[socket][0] = -self.clients[socket][0]
        self.clients[socket][1] = name
        self.clients[socket][2] = scaleX
        self.clients[socket][3] = scaleY
        self.clients[socket][4] = scaleZ
        self.clients[socket][5] = offsetX
        self.clients[socket][6] = offsetY
        self.clients[socket][7] = offsetZ

        self.updateSavemaster()

    def rmClient(self, socket):
        if(self.clients[socket]):
            clientId = self.clients[socket][0]

            print("Removing client " + str(self.clients[socket][1]) + " (id " + str(clientId) + ")")

            del self.idToSock[clientId]
            self.freeIDs.append(clientId)
            del self.clients[socket]

            keysInConnections = self.reverseConnections[socket]
            keysInReverseConnections = self.connections[socket]

            for currentKey in keysInConnections:
                for currentValueElement in self.connections[currentKey]:
                    if currentValueElement == socket:
                        i = self.connections[currentKey].index(currentValueElement)
                        del self.connections[currentKey][i]
        
            for currentKey in keysInReverseConnections:
                for currentValueElement in self.reverseConnections[currentKey]:
                    if currentValueElement == socket:
                        i = self.reverseConnections[currentKey].index(currentValueElement)
                        del self.reverseConnections[currentKey][i]

            del self.connections[socket]
            del self.reverseConnections[socket]

            del self.inMessages[socket]
            del self.outMessages[socket]

            if clientId == self.curHighestSavePriority[1]:
                self.curSavemaster = self.INFTY
                self.updateSavemaster()

    def updateSavemaster(self):
            self.curHighestSavePriority = [self.INFTY, self.INFTY]
            try:
                for curClient in self.clients.iterkeys():
                    if self.clients[curClient][0] > 0:
                        if (self.clients[curClient][2] < self.curHighestSavePriority[0]) or \
                           (self.clients[curClient][2] == self.curHighestSavePriority[0] and \
                            abs(self.clients[curClient][0]) < self.curHighestSavePriority[1]):
                            self.curHighestSavePriority[0] = self.clients[curClient][2]
                            self.curHighestSavePriority[1] = abs(self.clients[curClient][0])
            except AttributeError:
                for curClient in self.clients.keys():
                    if self.clients[curClient][0] > 0:
                        if (self.clients[curClient][2] < self.curHighestSavePriority[0]) or \
                           (self.clients[curClient][2] == self.curHighestSavePriority[0] and \
                            abs(self.clients[curClient][0]) < self.curHighestSavePriority[1]):
                            self.curHighestSavePriority[0] = self.clients[curClient][2]
                            self.curHighestSavePriority[1] = abs(self.clients[curClient][0])
