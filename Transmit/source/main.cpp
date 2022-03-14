#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "MicroBit.h"
#include "time.h"   // srand(times)
#include "aes.h"

MicroBit    uBit;

// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
#define PORT 8
#define MODE_SENDING 1
#define MODE_RECEIVING  2
#define SC 199
#define MAX_CIPHER_LEN 300

extern void sha256(const void *data, size_t len, char *sha2digest);
AES aes(AESKeyLength::AES_128);  ////128 - key length, can be 128, 192 or 256

int appMode = 0; // application mode it can be either MODE_SENDING / MODE_RECEIVING
int indexSalt = -1; // initialized dummy negative value. Actual value will be 0..9
char runCommand = '\0';

char dpk[128];

//=========================================================
//========= Function to print a string in hexadecimal
//=========================================================
void showHex (const unsigned char *s, int len)
{
  for(int i=0; i< len; i++)
    uBit.serial.printf("%02x", (unsigned int) s[i]);
    // printf("%02x", (unsigned int) s[i]);
  uBit.serial.printf("\n");
}

/*
* Returning "Integer" random generated number using Rand and RAND_MAX

* ------- Parameter info
* min: Minimum constant value to generate Digit Frequency table e.g. 0, 100, 1000
* max: Maximum constatn value to generate Digit frequency table e.g. 99, 999, 9999

*/

char strSalt2Send[50];

int randNumber(const int min, const int max) {
    int result = 0;

    result = (rand() % (max - min)) + min;
    return result;
}


//
void sendBuffer(char* msg, int len) {

}


int modeSelection() {
  uBit.display.scroll("A->S | B <- R");
  int result = -1;

  //await input.
  while(result == -1){
    if(uBit.buttonA.isPressed()){
      uBit.display.print("S");
      result = MODE_SENDING;
    } else if(uBit.buttonB.isPressed()){
      uBit.display.print("R");
      result = MODE_RECEIVING;
    }
  }

  return result; // Return Result
}

int main() {
    // Initialise the micro:bit runtime.
    uBit.init();

    srand(time(NULL));

    uBit.sleep(5000); // holding 5 Sec to allow receiver to get ready for listing shake-hand

    uBit.serial.printf("\n Transmitte ");

    // Initialize Radio
    uBit.radio.enable();
    uBit.radio.setGroup(PORT); // Setting up communication channel / Port
    uBit.display.print("T");

    int salt = randNumber(0, 9); // This is index of saltkey which will be used for encryption

    salt = salt + SC; // SecretKey + Salt Index
    itoa(salt, strSalt2Send);

    sha256(strSalt2Send, strlen(strSalt2Send), dpk); // generate key from strSalt2Send for further communication

    uBit.radio.datagram.send(strSalt2Send); // sendint "salt" index

    uBit.serial.printf("\n dpk %s %s %d", dpk, strSalt2Send, salt);

    unsigned int cipherLen,  plainLen = 1;
    unsigned char command1[2] = {0x31, 0x00}, command2[2] = {0x32, 0x00};

    while(1)
    {
        if (uBit.buttonA.isPressed()) { //if found the button A pressed at the time of call
          uBit.display.print("1");
          showHex(command1, 2);
          // Generate cipher using AES of command
          unsigned char *cipher = aes.EncryptECB( command1, plainLen, (unsigned char*) dpk, cipherLen);

          showHex(cipher, cipherLen);
          uBit.radio.datagram.send((char*)cipher);

        } else if (uBit.buttonB.isPressed()) {//if found the button B pressed at the time of call
            uBit.serial.printf("\n Sending command B ");
            uBit.display.print("2");
            showHex(command2, 2);
            unsigned char *cipher = aes.EncryptECB( command2, plainLen, (unsigned char*) dpk, cipherLen);

            showHex(cipher, cipherLen);
            uBit.radio.datagram.send((char*)cipher);
        }

        uBit.sleep(500);//min time in ms between two transmission, 500ms is  0.5sec!
    }
}
