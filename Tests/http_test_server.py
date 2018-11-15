#!/usr/bin/python

#  http_test_server.py
#  KSSCore
#
#  Created by Steven W. Klassen on 2015-10-29.
#  Copyright (c) 2015 Klassen Software Solutions. All rights reserved.


from BaseHTTPServer import BaseHTTPRequestHandler,HTTPServer

PORT_NUMBER = 8080

#This class will handles any incoming request from
#the browser
class myHandler(BaseHTTPRequestHandler):

    def _handler(self):
        if self.path == "/hello":
            self.send_response(200)
            self.send_header('Content-type','text/plain')
            self.end_headers()
            if self.command == "GET":
                self.wfile.write("Hello World")
        else:
            self.send_response(404)

        return

    def do_GET(self):
        self._handler()
        return

    def do_HEAD(self):
        self._handler()
        return

    def do_POST(self):
        self._handler()
        return

    def do_PUT(self):
        self._handler()
        return



try:
    #Create a web server and define the handler to manage the
    #incoming request
    server = HTTPServer(('', PORT_NUMBER), myHandler)
    print 'Started httpserver on port ' , PORT_NUMBER

    #Wait forever for incoming htto requests
    server.serve_forever()

except KeyboardInterrupt:
    print '^C received, shutting down the web server'
    server.socket.close()

