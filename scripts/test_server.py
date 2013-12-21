from twisted.internet import reactor
from autobahn.websocket import WebSocketServerFactory, \
                               WebSocketServerProtocol, \
                               listenWS
import bbmt_pb2

class EchoServerProtocol(WebSocketServerProtocol):
   def onMessage(self, msg, binary):
       request = bbmt_pb2.ApiMessage()
       request.ParseFromString(msg)
       print "Request:\n",request
       
       response = None
       if request.type == bbmt_pb2.ApiMessage.AUTH_REQUEST:
           response = bbmt_pb2.ApiMessage()
           response.type = bbmt_pb2.ApiMessage.AUTH_RESPONSE
           response.authResponse.authenticated = True
       elif request.type == bbmt_pb2.ApiMessage.UPDATE_REQUEST:
           response = bbmt_pb2.ApiMessage()
           response.type = bbmt_pb2.ApiMessage.UPDATE_CHUNK
           response.updateChunk.offset = 0
           response.updateChunk.data = b"123456"
           
       if response:
           print "Response:\n",response
           self.sendMessage(response.SerializeToString(), True)
 
if __name__ == '__main__':
    factory = WebSocketServerFactory("ws://localhost:9000")
    factory.protocol = EchoServerProtocol
    listenWS(factory)
    reactor.run()
   