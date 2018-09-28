//
//  main.cpp
//  Server
//
//  Created by Bernardo Ferreira on 27/03/17.
//  Copyright © 2017 Bernardo Ferreira. All rights reserved.
//

#include <iostream>
#include "SseServer.hpp"

int main(int argc, const char * argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    SseServer server;
    
    return 0;
}
