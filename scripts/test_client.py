import os
import sys
import time
from twisted.internet import reactor
from autobahn.websocket.protocol import WebSocketClientFactory, \
                               WebSocketClientProtocol
from autobahn.twisted.websocket import connectWS
import bbmt_pb2


class BrewBitClientProtocol(WebSocketClientProtocol):
 
    def send_msg(self, msg):
        if msg:
            print "Message:\n",msg
            self.sendMessage(msg.SerializeToString(), True)
 
    def onOpen(self):
        request = bbmt_pb2.ApiMessage()
        request.type = bbmt_pb2.ApiMessage.AUTH_REQUEST
        request.authRequest.device_id = 'ajsdf09jaosivd8a'
        request.authRequest.auth_token = 'asdfasdf'
        self.send_msg(request)
 
    def onMessage(self, msg, binary):
       print "Got msg: " + msg
 
 
if __name__ == '__main__':
    endpoint = "ws://localhost:9000"
    #endpoint = sys.argv[1]
    factory = WebSocketClientFactory(endpoint, headers = {'Device-ID': 'ajsdf09jaosivd8a'}, debug = False)
    factory.protocol = BrewBitClientProtocol
    connectWS(factory)
    reactor.run()
   