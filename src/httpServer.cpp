#include <WebServer.h>
#include "httpServer.h"
#include "web.h"
#include <ArduinoJson.h>

httpServer::httpServer():_server(80){

}

WebServer* httpServer::getServer(){
  return &_server;
}

void httpServer::handleClient(){
  _server.handleClient();
}

void httpServer::begin(){
  _server.begin();
}

void httpServer::handleRootPath() {            //Handler for the rooth path
  String html = MAIN_page;
  _server.send(200, "text/html", html); 
}


void httpServer::handleConfigurePath() {
  String html = CONFIGURE_page;
  _server.send(200, "text/html", html); 
}

void httpServer::setup(){
  _server.on("/", std::bind(&httpServer::handleRootPath, this));    //Associate the handler function to the path
  _server.on("/configure",std::bind(&httpServer::handleConfigurePath, this));
}