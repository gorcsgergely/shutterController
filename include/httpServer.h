#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WebServer.h>
#include "config.h"

class httpServer{
    public:
        httpServer();
        WebServer* getServer();
        void handleClient();
        void begin();
        void handleRootPath();
        void handleConfigurePath();
        void setup();

    private:
        WebServer _server;
};

#endif