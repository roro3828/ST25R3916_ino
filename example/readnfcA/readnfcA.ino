#include "rfal_utils.h"

#include "rfal_platform.h"

#include "rfal_nfc.h"

#define EXAMPLE_NFCA_DEVICES     10


#include <Arduino.h>
uint8_t globalCommProtectCnt;
void setup(){

    Serial.begin(9600);
    delay(1000);
    Serial.printf("Setup\n");
    spiInit();
    delay(1000);

    ReturnCode           err;

    rfalNfcaSensRes      sensRes;

    rfalNfcaSelRes       selRes;

    rfalNfcaListenDevice nfcaDevList[EXAMPLE_NFCA_DEVICES];

    uint8_t              devCnt;

    uint8_t              devIt;


    rfalLmConfPA nfcAparam;
    for(uint8_t i=0;i<10;i++){
        nfcAparam.nfcid[i]=(((i*2)%10)*0x10)+(i*2+1)%10;
    }
    nfcAparam.nfcidLen=RFAL_LM_NFCID_LEN_10;
    nfcAparam.SEL_RES=0x20;
    nfcAparam.SENS_RES
    rfalNfcDiscoverParam param;
    param.lmConfigPA

    

    rfalInitialize();

    

    while(1)

    {

        rfalFieldOff();                                                                   /* Turn the Field Off */

        platformDelay(500);

        rfalNfcDiscover()

    }


}

void loop(){
}
