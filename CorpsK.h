#ifndef CORPSK_H
#define CORPSK_H

extern const unsigned char TableMultiply2_8[256][256];
#define Multiply(a,b) (TableMultiply2_8[a][b])

//unsigned char Multiply(unsigned char a, unsigned char b);
unsigned char Inverse(unsigned char a);


#endif
