#include "rfal_utils.h"
#include "rfal_analogConfig.h"

#include "rfal_platform.h"
#include "rfal_rf.h"
#include "rfal_nfc.h"

#define EXAMPLE_NFCA_DEVICES     10


#include <Arduino.h>

#define EMULATE_STATE_NOTINIT 0
#define EMULATE_STATE_START_DISCOVERY 1
#define EMULATE_STATE_DISCOVERY 2

#include"felica.hpp"

static uint8_t ceNFCA_NFCID[]     = {0x00,0x12,0x34,0x67};    /* =_STM, 5F 53 54 4D NFCID1 / UID (4 bytes) */
static uint8_t ceNFCA_SENS_RES[]  = {0x04, 0x00};             /* SENS_RES / ATQA for 4-byte UID            */


static uint8_t ceNFCA_SEL_RES     = 0x20;                     /* SEL_RES / SAK                             *///iso dep
//static uint8_t ceNFCA_SEL_RES     = 0x08;                     /* SEL_RES / SAK                             *///mifare classic
static uint8_t NFCID3[] = {0x01, 0xFE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
static uint8_t GB[] = {0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02, 0x07, 0x80, 0x03, 0x02, 0x00, 0x03, 0x04, 0x01, 0x32, 0x07, 0x01, 0x03};
static uint8_t felica_idm[FELICA_IDM_SIZE];
static uint8_t felica_pmm[FELICA_PMM_SIZE];
static uint8_t ceNFCF_SC[]         = {0x00, 0x03};
static uint8_t ceNFCF_SENSF_RES[]  = {0x01,                                                   /* SENSF_RES                                */
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             /* NFCID2                                   */
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             /* PAD0, PAD01, MRTIcheck, MRTIupdate, PAD2 */
                                    0x00, 0x00 };        
uint8_t globalCommProtectCnt;
rfalNfcDiscoverParam discoverparam;

uint8_t state=0;

static Felica *felica;

bool nfcinit(){
    ReturnCode err=rfalNfcInitialize();
    if(err!=RFAL_ERR_NONE){
        return false;
    }
    rfalNfcDefaultDiscParams(&discoverparam);
    discoverparam.devLimit=1U;
    memcpy(discoverparam.nfcid3,NFCID3,sizeof(NFCID3));
    memcpy(discoverparam.GB,GB,sizeof(GB));
    discoverparam.GBLen=sizeof(GB);
    discoverparam.p2pNfcaPrio=true;
    discoverparam.notifyCb=notify;
    discoverparam.totalDuration=1000U;
    discoverparam.techs2Find=RFAL_NFC_TECH_NONE;

    //NFC Aエミュレート用パラメータ
    /*
    memcpy(discoverparam.lmConfigPA.SENS_RES,ceNFCA_SENS_RES,RFAL_LM_SENS_RES_LEN);
    memcpy(discoverparam.lmConfigPA.nfcid,ceNFCA_NFCID,RFAL_LM_NFCID_LEN_04);
    discoverparam.lmConfigPA.nfcidLen=RFAL_LM_NFCID_LEN_04;
    discoverparam.lmConfigPA.SEL_RES=ceNFCA_SEL_RES;
    discoverparam.techs2Find|=RFAL_NFC_LISTEN_TECH_A;
    //*/

    //NFC Fエミュレート用パラメータ
    /*
    memcpy(discoverparam.lmConfigPF.SC,ceNFCF_SC,RFAL_LM_SENSF_SC_LEN);
    memcpy(&ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN],felica_idm,RFAL_NFCID2_LEN);
    memcpy(&ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN],felica_pmm,RFAL_NFCID2_LEN);
    memcpy(discoverparam.lmConfigPF.SENSF_RES,ceNFCF_SENSF_RES,RFAL_LM_SENSF_RES_LEN);
    discoverparam.techs2Find|=RFAL_NFC_LISTEN_TECH_F;
    //*/

    //Poll felica
    discoverparam.techs2Find|=RFAL_NFC_POLL_TECH_F;

    err=rfalNfcDiscover(&discoverparam);
    rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
    if(err!=RFAL_ERR_NONE){
        return false;
    }
    state=EMULATE_STATE_START_DISCOVERY;

    return true;
}

uint8_t block_data[20][16]={
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    //稚内(0x0F,0x40)で65535円チャージ
    {18,0x02,0x02,0x03,0x04,0x05,0x0F,0x40,0x08,0x09,0xFF,0xFF,0x0C,0x0D,0x0E,0x0F},
    //稚内(0x0F,0x40)から城北線味美(0x42,0x82)まで移動 残金は1万円(0x10 0x27)
    {26,0x01,0x02,0x03,0x04,0x05,0x0F,0x40,0x42,0x82,0x10,0x27,0x0C,0x0D,0x0E,0x0F},
    //城北線味美(0x42,0x82)から鹿児島中央(0x06,0xA2)まで0円で移動
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0x06,0xA2,0x10,0x27,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {0xA0,0x00,0x02,0x04,0x10,0x03,0x0A,0x75,0x23,0x09,0x82,0x00,0xA0,0x00,0x25,0x0F},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x1a,0x00,0x00,0x0F}
    };
void setup(){

    Serial.begin(9600);
    delay(1000);
    Serial.printf("Setup\n");
    spiInit();
    delay(1000);

    if(nfcinit()){
        Serial.printf("nfc init success\n");
    }
    else{
        Serial.printf("nfc init faild\n");
    }
}
rfalNfcState lnfcstate=RFAL_NFC_STATE_NOTINIT;
void loop(){
    static rfalNfcDevice *nfcDevice;
    rfalNfcWorker();

    switch(state){
        case EMULATE_STATE_START_DISCOVERY:
            rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
            rfalNfcDiscover(&discoverparam);
            state=EMULATE_STATE_DISCOVERY;
        break;

        case EMULATE_STATE_DISCOVERY:
            if(!rfalNfcIsDevActivated(rfalNfcGetState())){
                break;
            }
            rfalNfcGetActiveDevice(&nfcDevice);
            switch(nfcDevice->type){
                case RFAL_NFC_LISTEN_TYPE_NFCF:
                    platformLog("Felica card found. UID:");
                    for(uint8_t i=0;i<nfcDevice->nfcidLen;i++){
                        platformLog("%02X ",nfcDevice->nfcid[i]);
                        
                    }
                    platformLog("\n");
                    NFCF(&nfcDevice->dev.nfcf);
                break;

                case RFAL_NFC_POLL_TYPE_NFCA:
                case RFAL_NFC_POLL_TYPE_NFCF:
                    //platformLog("Activated in CE %s mode.\n", (nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? "NFC-A" : "NFC-F");
                    //platformLog("Activated interface %s\n",(nfcDevice->rfInterface==RFAL_NFC_INTERFACE_ISODEP)?"ISODEP":((nfcDevice->rfInterface==RFAL_NFC_INTERFACE_NFCDEP)?"NFCDEP":"RF"));
                    emulate(nfcDevice);
                break;

                default:
                    platformLog("Activated type %d\n",nfcDevice->type);
                break;
            }
            rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);

            switch(nfcDevice->type){
                case RFAL_NFC_POLL_TYPE_NFCA:
                case RFAL_NFC_POLL_TYPE_NFCF:
                break;
                default:
                    platformDelay(500);
            }

            state=EMULATE_STATE_START_DISCOVERY;
        break;

        case EMULATE_STATE_NOTINIT:
        default:
        break;
    }

}
void NFCF(rfalNfcfListenDevice *nfcDev){
    ReturnCode err=RFAL_ERR_NONE;
    uint8_t *rxData;
    uint16_t *rcvLen;
    uint8_t txBuf[1024];
    uint16_t txLen;

    txLen=RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN+2;
    txBuf[0]=0x0A;
    memcpy(&txBuf[RFAL_NFCF_CMD_LEN],nfcDev->sensfRes.NFCID2,RFAL_NFCID2_LEN);
    for(uint16_t i=0;i<=0xFF;i++){
        txBuf[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN]=(i&0xFF);
        txBuf[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN+1]=(i>>8)&0xFF;
        err=TransceiveBlocking(txBuf,txLen*8,&rxData,&rcvLen,RFAL_FWT_NONE);

        Serial.printf("%4d err:%d rx %4d:",i,err,(*rcvLen/8));
        for(uint16_t i=0;i<(*rcvLen/8);i++){
            Serial.printf("%02X ",rxData[i]);
        }
        Serial.printf("\n");

    }
    /*
    txBuf[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN]=0x01;
    txBuf[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN+1]=0x8B;
    txBuf[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN+2]=0x00;
    txBuf[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN+3]=0x01;
    txBuf[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN+4]=0x80;
    txBuf[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN+5]=0x00;
    txLen=RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN+6;
    //*/



}

void emulate(rfalNfcDevice *nfcDev){
    ReturnCode err=RFAL_ERR_NONE;
    uint8_t *rxData;
    uint16_t *rcvLen;
    uint8_t txBuf[1024];
    uint16_t txLen;

    do{
        rfalNfcWorker();

        switch(rfalNfcGetState()){
            case RFAL_NFC_STATE_ACTIVATED:
                //Serial.printf("activated\n");
                err=TransceiveBlocking(NULL,0,&rxData,&rcvLen,0);
                /*

                Serial.printf("err:%d rxdata:",err);
                for(uint16_t i=0;i<*rcvLen;i++){
                    Serial.printf("%02X ",rxData[i]);
                }
                Serial.printf("\n");
                //*/

            break;

            case RFAL_NFC_STATE_DATAEXCHANGE:
            case RFAL_NFC_STATE_DATAEXCHANGE_DONE:
                //*
                if(nfcDev->type==RFAL_NFC_POLL_TYPE_NFCA){
                    txLen=2;
                    txBuf[0]=0x90;
                    txBuf[1]=0x00;
                    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,RFAL_FWT_NONE);
                    Serial.printf("type a ");
                    Serial.printf("rxdata:");
                    for(uint16_t i=0;i<*rcvLen;i++){
                        Serial.printf("%02X ",rxData[i]);
                    }
                    Serial.printf("\n");
                }
                else{
                    const uint8_t rxLen=rxData[0]-1;
                    felica->listen(&rxData[1],rxLen,txBuf,&txLen);
                    err=TransceiveBlocking(txBuf,(txLen*8),&rxData,&rcvLen,RFAL_FWT_NONE);
                    Serial.printf("err:%d\n",err);
                    Serial.printf("tx:");
                    for(uint16_t i=0;i<txLen;i++){
                        Serial.printf("%02X ",txBuf[i]);
                    }
                    Serial.printf("\n");
                    Serial.printf("rx:");
                    for(uint8_t i=0;i<rxLen;i++){
                        Serial.printf("%02X ",rxData[1+i]);
                    }
                    Serial.printf("\n");
                    /*
                    Serial.printf("type f ");
                    Serial.printf("rxdata:");
                    for(uint16_t i=0;i<*rcvLen;i++){
                        Serial.printf("%02X ",rxData[i]);
                    }
                    Serial.printf("\n");
                    //*/

                }
                

                //*/
            
            break;

            case RFAL_NFC_STATE_START_DISCOVERY:
                return;

            case RFAL_NFC_STATE_LISTEN_SLEEP:
            default:
            break;
        }
    }while((err==RFAL_ERR_NONE) || (err==RFAL_ERR_SLEEP_REQ));
}

ReturnCode TransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxData, uint16_t **rcvLen, uint32_t fwt )
{
    ReturnCode err;

    err = rfalNfcDataExchangeStart( txBuf, txBufSize, rxData, rcvLen, fwt );
    //Serial.printf("tranciverr:%d\n",err);
    if( err == RFAL_ERR_NONE )
    {
        do{
            rfalNfcWorker();
            err = rfalNfcDataExchangeGetStatus();
        }
        while( err == RFAL_ERR_BUSY );
    }
    return err;
}

void notify(rfalNfcState st){
    //Serial.printf("Callback state:%d\n",st);
}