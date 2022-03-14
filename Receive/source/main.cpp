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


////////////////////////////RECEIVING CODE/////////////////////////////////////////////////////////
char readMsg[250], strSalt2Receive[50];
int maxRepeat = 0;

void recieveSaltIndex(MicroBitEvent) {
  uBit.serial.printf("\n Start receiving ");
  // Receive the message from the radio
  PacketBuffer b = uBit.radio.datagram.recv();

  // Obtain the length of the recived data
  int packet_len = b.length();

  // Get a pointer to byte array
  uint8_t *buf  = b.getBytes();

  //Add Null terminate the string.
  buf[packet_len] = '\0';

  indexSalt = atoi((char*)buf); // assign IndexSalt value sent by sender which will be used to decrypt command
  // indexSalt = indexSalt - SC; // Secret key was added in salt
  itoa(indexSalt, strSalt2Receive);
  sha256(strSalt2Receive, strlen(strSalt2Receive), dpk); // generate key from strSalt2Send for further communication
  // uBit.display.print(buf[0]);
  uBit.serial.printf("\n dpk %s strSalt2Receive %s %d", dpk, strSalt2Receive, indexSalt);
}

void recieveBuffer(MicroBitEvent) {
  /* code */
  // Storage for our string
  // Receive the message from the radio
  PacketBuffer b = uBit.radio.datagram.recv();
  unsigned char* command;

  // Obtain the length of the recived data
  int packet_len = b.length();

  // Get a pointer to byte array
  uint8_t *buf  = b.getBytes();

  uBit.serial.printf("receiving buffer %s\r\n",(char*)buf);
  //Add Null terminate the string.
  buf[packet_len] = '\0';
  strcat(readMsg, (char*)buf);

    command = aes.DecryptECB((unsigned char*)readMsg, 16, (unsigned char*)dpk);
    uBit.serial.printf("%s Command is %s \r\n",readMsg, (char*)command);

    if (command[0] == '1' || strcmp((char*)command, "2") == 0) {
      runCommand = '1';
      uBit.display.print("1");
    } else if(command[0] == '2' || strcmp((char*)command, "2") == 0) {
      runCommand = '2';
      uBit.display.print("2");
    }

    uBit.serial.printf(" END Recevie Command -> %s ", (char*)command);

    // Clear buffer
    readMsg[0] = 0;
    maxRepeat = 0;
}


void initCommReceive() {
  // Initialise MicroBit with
  uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, recieveSaltIndex); // temporary for read Salt
  uBit.radio.enable();
  uBit.radio.setGroup(PORT);
  uBit.display.print("R");

  // Wait while Salt received from sender
  while(indexSalt == -1) {
    uBit.sleep(500); // keep waiting to receive sault
  }
  //End -- Wait while Salt received from sender
  uBit.radio.disable();
  uBit.sleep(200);

  // ReInitialize microbit with receiving data in ReceiveBuffer now
  uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, recieveBuffer); // to receive Command with AES buffer
  uBit.radio.enable();
  uBit.radio.setGroup(PORT);
  uBit.serial.printf("Now receving buffer");
  uBit.display.print("?");
}

void receiveCommand() {
  char displayMsg[10];
      while(1) {
          if(runCommand != '\0') {
            strcpy(displayMsg, runCommand == '1' ? "1" : "2");
            uBit.serial.printf("Command  -> %s", runCommand);
            uBit.display.print(displayMsg);
          }
          uBit.sleep(500);
      }
}

int main() {
    // Initialise the micro:bit runtime.
    uBit.init();

    uBit.serial.printf("\n Receiver ");

    initCommReceive();
    receiveCommand();
}
