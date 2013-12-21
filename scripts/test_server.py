from twisted.internet import reactor
from autobahn.websocket import WebSocketServerFactory, \
                               WebSocketServerProtocol, \
                               listenWS
import bbmt_pb2

class BrewBitServerProtocol(WebSocketServerProtocol):
    def onMessage(self, msg, binary):
        request = bbmt_pb2.ApiMessage()
        request.ParseFromString(msg)
        print "Request:\n",request
       
        response = None
        if request.type == bbmt_pb2.ApiMessage.AUTH_REQUEST:
            response = bbmt_pb2.ApiMessage()
            response.type = bbmt_pb2.ApiMessage.AUTH_RESPONSE
            response.authResponse.authenticated = True
            self.send_response(response)
        elif request.type == bbmt_pb2.ApiMessage.UPDATE_REQUEST:
            with open('build/app_mt/app_mt_update.bin', 'rb') as f:
                while True:
                    response = bbmt_pb2.ApiMessage()
                    response.type = bbmt_pb2.ApiMessage.UPDATE_CHUNK
                    response.updateChunk.offset = f.tell()
                    response.updateChunk.data = f.read(1024)
                    self.send_response(response)
                    
                    if len(response.updateChunk.data) < 1024:
                        break
                   
    def send_response(self, response):
        if response:
            print "Response:\n",response
            self.sendMessage(response.SerializeToString(), True)
 
if __name__ == '__main__':
    factory = WebSocketServerFactory("ws://localhost:9000")
    factory.protocol = BrewBitServerProtocol
    listenWS(factory)
    reactor.run()
   