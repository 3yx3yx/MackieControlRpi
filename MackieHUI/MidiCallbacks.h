#ifndef MIDICALLBACKS_H
#define MIDICALLBACKS_H

#include "midiprocess.h"

QString convertToRawMidi (std::vector<unsigned char> *message, int port);


void inputCallbackRedirect1 (double deltatime, std::vector< unsigned char > *message, void *);
void inputCallbackRedirect2 (double deltatime, std::vector< unsigned char > *message, void * );
void inputCallbackRedirect3 (double deltatime, std::vector< unsigned char > *message, void * );
void inputCallbackRedirect4 (double deltatime, std::vector< unsigned char > *message, void * );
void inputCallbackRedirect5 (double deltatime, std::vector< unsigned char > *message, void * );
void inputCallbackRedirect6 (double deltatime, std::vector< unsigned char > *message, void * );
void inputCallbackRedirect7 (double deltatime, std::vector< unsigned char > *message, void * );


#endif // MIDICALLBACKS_H
