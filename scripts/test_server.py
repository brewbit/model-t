import os
import time
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
        elif request.type == bbmt_pb2.ApiMessage.ACTIVATION_TOKEN_REQUEST:
            response = bbmt_pb2.ApiMessage()
            response.type = bbmt_pb2.ApiMessage.ACTIVATION_TOKEN_RESPONSE
            response.activationTokenResponse.activation_token = "12kcin"
            self.send_response(response)
            
            time.sleep(1)
            
            response = bbmt_pb2.ApiMessage()
            response.type = bbmt_pb2.ApiMessage.ACTIVATION_NOTIFICATION
            response.activationNotification.auth_token = "12kcinoaw98hvnow8hfsaj"
            self.send_response(response)
        elif request.type == bbmt_pb2.ApiMessage.FIRMWARE_UPDATE_CHECK_REQUEST:
            response = bbmt_pb2.ApiMessage()
            response.type = bbmt_pb2.ApiMessage.FIRMWARE_UPDATE_CHECK_RESPONSE
            response.firmwareUpdateCheckResponse.update_available = True
            response.firmwareUpdateCheckResponse.version = "1.0.1"
            response.firmwareUpdateCheckResponse.binary_size = os.stat('build/app_mt/app_mt_update.bin').st_size
            self.send_response(response)
        elif request.type == bbmt_pb2.ApiMessage.FIRMWARE_DOWNLOAD_REQUEST:
            with open('build/app_mt/app_mt_update.bin', 'rb') as f:
                while True:
                    response = bbmt_pb2.ApiMessage()
                    response.type = bbmt_pb2.ApiMessage.FIRMWARE_DOWNLOAD_RESPONSE
                    response.firmwareDownloadResponse.offset = f.tell()
                    response.firmwareDownloadResponse.data = f.read(1024)
                    self.send_response(response)
                    
                    if len(response.firmwareDownloadResponse.data) < 1024:
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
   