//参照 https://cardwerk.com/smart-card-standard-iso7816-4-section-6-basic-interindustry-commands/
//https://www.npa.go.jp/laws/notification/koutuu/menkyo/menkyo20240719_145.pdf

#include "rfal_nfc.h"
#include "apdu.hpp"
#include <Arduino.h>
#include "data.hpp"


#define STATE_NOTINIT 0
#define STATE_START_DISCOVERY 1
#define STATE_DISCOVERY 2

static const char DRIVERS_LICENSE_PASSWORD[]="3568";

//参照 https://www.nmda.or.jp/nmda/ic-card/pdf/kiyaku12.pdf
//NFCIDは4byteじゃないとなぜか動いてくれない
static uint8_t NFCID[RFAL_LM_NFCID_LEN_04]={0x12,0x34,0x56,0x78};
//ATQAの1バイト目は0x02
static uint8_t ATQA[RFAL_LM_SENS_RES_LEN]={0x02,0x00};
static uint8_t SAK=0x20;

static const uint8_t MYNUMBER_KENMENNYURYOKUHOJO[10]={0xD3,0x92,0x10,0x00,0x31,0x00,0x01,0x01,0x04,0x08};
static const uint8_t DRIVERS_LICENSE_AID[16]={0xA0,0x00,0x00,0x02,0x31,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t DRIVERS_LICENSE_PIN_SETTING_EFID[2]={0x00,0x1A};
static const uint8_t DRIVERS_LICENSE_PIN_EFID[2]={0x00,0x06};
static const uint8_t DRIVERS_LICENSE_DATA_EFID[2]={0x00,0x1B};
static const uint8_t DRIVERS_LICENSE_SIGNATURE_EFID[2]={0x00,0x1C};

uint8_t const *current_df=NULL;
uint8_t const *current_ef=NULL;

rfalNfcDiscoverParam discoverparam;
uint8_t state=STATE_NOTINIT;
static bool multiSel;

bool nfcinit(){
    ReturnCode err=rfalNfcInitialize();
    if(err!=RFAL_ERR_NONE){
        return false;
    }
    rfalNfcDefaultDiscParams(&discoverparam);
    discoverparam.devLimit=1U;
    discoverparam.notifyCb=notify;
    discoverparam.totalDuration=1000U;
    discoverparam.techs2Find=RFAL_NFC_TECH_NONE;
    //*
    //isoDep検出用パラメータ
    //discoverparam.techs2Find|=RFAL_NFC_POLL_TECH_A;
    discoverparam.techs2Find|=RFAL_NFC_POLL_TECH_B;
    //discoverparam.p2pNfcaPrio=true;
    //*/

    /*
    //エミュレート用パラメータ
    memcpy(discoverparam.lmConfigPA.nfcid,NFCID,RFAL_LM_NFCID_LEN_04);
    discoverparam.lmConfigPA.nfcidLen=RFAL_LM_NFCID_LEN_04;
    //ATQA
    memcpy(discoverparam.lmConfigPA.SENS_RES,ATQA,RFAL_LM_SENS_RES_LEN);
    //SAK
    discoverparam.lmConfigPA.SEL_RES=SAK;
    discoverparam.techs2Find|=RFAL_NFC_LISTEN_TECH_A;
    discoverparam.totalDuration=60000U;
    //*/


    err=rfalNfcDiscover(&discoverparam);
    rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
    if(err!=RFAL_ERR_NONE){
        return false;
    }

    state=STATE_START_DISCOVERY;

    return true;
}
void setup(){
    Serial.begin(115200);
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
        case STATE_START_DISCOVERY:
            rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
            rfalNfcDiscover(&discoverparam);
            state=STATE_DISCOVERY;
            multiSel=false;
        break;

        case STATE_DISCOVERY:
            if(!rfalNfcIsDevActivated(rfalNfcGetState())){
                break;
            }
            rfalNfcGetActiveDevice(&nfcDevice);
            switch(nfcDevice->type){
                case RFAL_NFC_LISTEN_TYPE_NFCA:
                    platformLog("TYPE A NFCID: %s\n",hex2Str(nfcDevice->nfcid,nfcDevice->nfcidLen));

                    if(nfcDevice->dev.nfca.type==RFAL_NFCA_T4T){
                        platformLog("NFCA Passive ISO-DEP device found.\n");
                        ISODEP(&nfcDevice->dev.nfca);
                    }
                break;
                case RFAL_NFC_LISTEN_TYPE_NFCB:
                    platformLog("TYPE B NFCID: %s\n",hex2Str(nfcDevice->nfcid,nfcDevice->nfcidLen));
                    if(rfalNfcbIsIsoDepSupported(&nfcDevice->dev.nfcb)){
                        platformLog("NFCB Passive ISO-DEP device found.\n");
                        readDriversLicense();
                    }
                break;
                case RFAL_NFC_POLL_TYPE_NFCA:
                    platformLog("TYPE A LISTEN\n");
                    emulate();

                break;

                default:
                    platformLog("Activated type %d\n",nfcDevice->type);
                break;
            }

            rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);

            switch(nfcDevice->type){
                case RFAL_NFC_POLL_TYPE_NFCA:
                break;
                default:
                platformDelay(500);
            }

            state=STATE_START_DISCOVERY;
        break;

        case STATE_NOTINIT:
        default:
            break;
    }

}
void ISODEP(rfalNfcaListenDevice *nfcDev){
    ReturnCode err=RFAL_ERR_NONE;
    uint8_t *rxData;
    uint16_t *rcvLen;
    platformLog("NFC A\nID:");
    for(uint8_t i=0;i<nfcDev->nfcId1Len;i++){
        platformLog("0x%02X ",nfcDev->nfcId1[i]);
    }
    platformLog("\n");
}
void ISODEP(rfalNfcbListenDevice *nfcDev){
    ReturnCode err=RFAL_ERR_NONE;
    uint8_t *rxData;
    uint16_t *rcvLen;
    platformLog("NFC B\nID:");
    for(uint8_t i=0;i<RFAL_NFCB_NFCID0_LEN;i++){
        platformLog("0x%02X ",nfcDev->sensbRes.nfcid0[4]);
    }
    platformLog("\nAFI: 0x%02X\n",nfcDev->sensbRes.appData.AFI);
    platformLog("numapps: %02d\n",nfcDev->sensbRes.appData.numApps);
    platformLog("cmd:0x%02X\n",nfcDev->sensbRes.cmd);
    platformLog("BRC:0x%02X\n",nfcDev->sensbRes.protInfo.BRC);
    platformLog("FsciProType:0x%02X\n",nfcDev->sensbRes.protInfo.FsciProType);
    platformLog("FwiAdcFo:0x%02X\n",nfcDev->sensbRes.protInfo.FwiAdcFo);
    platformLog("SFGI:0x%02X\n",nfcDev->sensbRes.protInfo.SFGI);
}

void readDriversLicense(){
    ReturnCode err=RFAL_ERR_NONE;
    uint8_t *rxData;
    uint16_t *rcvLen;
    uint8_t txBuf[1024];
    uint16_t txLen;

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_SELECT_FILE;
    txBuf[2]=0x04;
    txBuf[3]=0x0C;
    txBuf[4]=10;
    memcpy(&txBuf[5],MYNUMBER_KENMENNYURYOKUHOJO,10);
    txLen=15;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_SELECT_FILE;
    txBuf[2]=0x04;
    txBuf[3]=0x0C;
    txBuf[4]=16;
    memcpy(&txBuf[5],DRIVERS_LICENSE_AID,16);
    txLen=21;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_SELECT_FILE;
    txBuf[2]=0x02;
    txBuf[3]=0x0C;
    txBuf[4]=0x02;
    memcpy(&txBuf[5],DRIVERS_LICENSE_PIN_SETTING_EFID,2);
    txLen=7;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_READ_BINARY;
    txBuf[2]=0x00;
    txBuf[3]=0x00;
    txBuf[4]=3;
    txLen=5;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_SELECT_FILE;
    txBuf[2]=0x02;
    txBuf[3]=0x0C;
    txBuf[4]=0x02;
    memcpy(&txBuf[5],DRIVERS_LICENSE_PIN_EFID,2);
    txLen=7;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_VERIFY;
    txBuf[2]=0x00;
    txBuf[3]=0x80;
    txBuf[4]=0x04;
    memcpy(&txBuf[5],DRIVERS_LICENSE_PASSWORD,4);
    txLen=9;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_SELECT_FILE;
    txBuf[2]=0x02;
    txBuf[3]=0x0C;
    txBuf[4]=0x02;
    memcpy(&txBuf[5],DRIVERS_LICENSE_DATA_EFID,2);
    txLen=7;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_READ_BINARY;
    txBuf[2]=0x00;
    txBuf[3]=0x00;
    //なぜか1度に84byteまでしか読み出せない
    txBuf[4]=84;
    txLen=5;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    uint16_t offset=0;

    while(2<*rcvLen){
        offset+=84;
        txBuf[0]=0x00;
        txBuf[1]=APDU_CMD_READ_BINARY;
        txBuf[2]=(offset>>8)&0xFF;
        txBuf[3]=(offset&0xFF);
        //なぜか1度に84byteまでしか読み出せない
        txBuf[4]=84;
        txLen=5;
        err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
        platformLog("TX:%s\n",hex2Str(txBuf,txLen));
        platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));
    }

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_SELECT_FILE;
    txBuf[2]=0x02;
    txBuf[3]=0x0C;
    txBuf[4]=0x02;
    memcpy(&txBuf[5],DRIVERS_LICENSE_SIGNATURE_EFID,2);
    txLen=7;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("TX:%s\n",hex2Str(txBuf,txLen));
    platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

    txBuf[0]=0x00;
    txBuf[1]=APDU_CMD_READ_BINARY;
    txBuf[2]=0x00;
    txBuf[3]=0x00;
    //なぜか1度に84byteまでしか読み出せない
    txBuf[4]=84;
    txLen=5;
    err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
    platformLog("%s\n",hex2Str(txBuf,txLen));
    platformLog("%s\n",hex2Str(rxData,*rcvLen));

    offset=0;

    while(2<*rcvLen){
        offset+=84;
        txBuf[0]=0x00;
        txBuf[1]=APDU_CMD_READ_BINARY;
        txBuf[2]=(offset>>8)&0xFF;
        txBuf[3]=(offset&0xFF);
        //なぜか1度に84byteまでしか読み出せない
        txBuf[4]=84;
        txLen=5;
        err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
        platformLog("%s\n",hex2Str(txBuf,txLen));
        platformLog("%s\n",hex2Str(rxData,*rcvLen));
    }
}

void emulate(){
    ReturnCode err=RFAL_ERR_NONE;
    uint8_t *rxData;
    uint16_t *rcvLen;
    uint8_t txBuf[1024];
    uint16_t txLen;

    do{
        rfalNfcWorker();

        switch(rfalNfcGetState()){
            case RFAL_NFC_STATE_ACTIVATED:
                err=TransceiveBlocking(NULL,0,&rxData,&rcvLen,0);
            break;

            case RFAL_NFC_STATE_DATAEXCHANGE:
                platformLog("State:EXCHANGE\n");
            case RFAL_NFC_STATE_DATAEXCHANGE_DONE:
                platformLog("State:EXCHANGE DONE\n");
                platformLog("RX:%s\n",hex2Str(rxData,*rcvLen));

                switch(rxData[APDU_INS_LEN]){
                    case APDU_CMD_SELECT_FILE:{
                        platformLog("CMD:SELECT FILE\n");
                        const uint8_t *dfname=&rxData[APDU_CLA_LEN+APDU_INS_LEN+APDU_P1_LEN+APDU_P2_LEN+1];
                        const uint8_t dflen=rxData[APDU_CLA_LEN+APDU_INS_LEN+APDU_P1_LEN+APDU_P2_LEN];
                        if(dflen==10&&(memcmp(dfname,MYNUMBER_KENMENNYURYOKUHOJO,10)==0)){
                            current_df=MYNUMBER_KENMENNYURYOKUHOJO;
                            current_ef=NULL;
                            platformLog("Select mynumber kenmennyuuyokuhojo AP\n");
                            txBuf[0]=0x90;
                            txBuf[1]=0x00;
                            txLen=2;
                        }
                        else if(dflen==16&&(memcmp(dfname,DRIVERS_LICENSE_AID,5)==0)){
                            current_df=DRIVERS_LICENSE_AID;
                            current_ef=NULL;
                            platformLog("Select Drivers License AP\n");
                            txBuf[0]=0x90;
                            txBuf[1]=0x00;
                            txLen=2;
                        }
                        else if(dflen==2&&((current_df==DRIVERS_LICENSE_AID)&&(memcmp(dfname,DRIVERS_LICENSE_PIN_SETTING_EFID,2)==0))){
                            current_ef=DRIVERS_LICENSE_PIN_SETTING_EFID;
                            platformLog("Select Drivers License PIN Setting EF\n");
                            txBuf[0]=0x90;
                            txBuf[1]=0x00;
                            txLen=2;
                        }
                        else if(dflen==2&&((current_df==DRIVERS_LICENSE_AID)&&(memcmp(dfname,DRIVERS_LICENSE_PIN_EFID,2)==0))){
                            current_ef=DRIVERS_LICENSE_PIN_EFID;
                            platformLog("Select Drivers License PIN EF\n");
                            txBuf[0]=0x90;
                            txBuf[1]=0x00;
                            txLen=2;
                        }
                        else if(dflen==2&&((current_df==DRIVERS_LICENSE_AID)&&(memcmp(dfname,DRIVERS_LICENSE_DATA_EFID,2)==0))){
                            current_ef=DRIVERS_LICENSE_DATA_EFID;
                            platformLog("Select Drivers License Data EF\n");
                            txBuf[0]=0x90;
                            txBuf[1]=0x00;
                            txLen=2;
                        }
                        else{
                            txBuf[0]=0x6A;
                            txBuf[1]=0x82;
                            txLen=2;
                        }

                    }
                    break;

                    case APDU_CMD_READ_BINARY:{
                        platformLog("CMD:READ BINARY\n");
                        uint8_t rlen=rxData[APDU_CLA_LEN+APDU_INS_LEN+APDU_P1_LEN+APDU_P2_LEN];
                        memset(txBuf,0,rlen);

                        if(current_ef==DRIVERS_LICENSE_PIN_SETTING_EFID){
                            //暗証番号が設定されているかどうか
                            rlen=3;
                            txBuf[0]=0xC1;
                            txBuf[1]=0x01;
                            //設定されている場合0x01 されてない場合0x00
                            txBuf[2]=0x00;

                            //マイナ免許読み取りアプリでパスワードが設定されていないを選択すると文字列"****"で検証される
                        }
                        else if(current_ef==DRIVERS_LICENSE_DATA_EFID){
                            //免許証データ
                            rlen=64;
                            memcpy(txBuf,driversdata,64);
                        }

                        txBuf[rlen]=0x90;
                        txBuf[rlen+1]=0x00;
                        txLen=2+rlen;
                    }
                    break;
                    case APDU_CMD_VERIFY:{
                        platformLog("CMD:VERIFY\n");
                        const uint8_t verifydatalen=rxData[APDU_CLA_LEN+APDU_INS_LEN+APDU_P1_LEN+APDU_P2_LEN];
                        platformLog("Verify data:%s\n",hex2Str(&rxData[APDU_CLA_LEN+APDU_INS_LEN+APDU_P1_LEN+APDU_P2_LEN+1],verifydatalen));
                        txBuf[0]=0x90;
                        txBuf[1]=0x00;
                        txLen=2;
                    }
                    break;
                    default:
                    platformLog("INS:%02X\n",rxData[APDU_INS_LEN]);
                }
                
                
                err=TransceiveBlocking(txBuf,txLen,&rxData,&rcvLen,0);
                platformLog("TX:%s\n",hex2Str(txBuf,txLen));

            break;

            case RFAL_NFC_STATE_START_DISCOVERY:
                return;

            case RFAL_NFC_STATE_LISTEN_SLEEP:
            default:
            break;
        }
    }while((err==RFAL_ERR_NONE)||(err==RFAL_ERR_SLEEP_REQ));
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
    uint8_t devcount;
    rfalNfcDevice *dev;

    if(st==RFAL_NFC_STATE_WAKEUP_MODE){
        platformLog("Wake up mode started\n");
    }
    else if(st==RFAL_NFC_STATE_POLL_SELECT){
        if(!multiSel){
            multiSel=true;
            rfalNfcGetDevicesFound(&dev,&devcount);
            rfalNfcSelect(0);
            platformLog("Tags detected: %d\n",devcount);
        }
        else{
            rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_DISCOVERY);
        }
    }
    else if(st==RFAL_NFC_STATE_START_DISCOVERY){
        multiSel=false;
    }

    //platformLog("State:%d\n",st);
}

static char* hex2Str(uint8_t *arr,uint16_t len){
    static char str[256];
    for(uint16_t i=0;i<len;i++){
        sprintf(&str[i*3],"%02X ",arr[i]);
    }

    return str;
}