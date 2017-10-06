#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "VectorBuffer.h"

using std::cout;
using std::endl;


int main(int argc , char *argv[]) {

    VectorBuffer buf;

    buf.appendFromString("0b01011100101");

    cout << buf.toBitString() << endl;

	return EXIT_SUCCESS;
}



