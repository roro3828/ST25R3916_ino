#include "rfal_utils.h"
#include "rfal_analogConfig.h"

#include "rfal_platform.h"
#include "rfal_rf.h"
#include "rfal_nfc.h"
#include <Arduino.h>

#include"felica.hpp"
#include"transport_ic.hpp"


#define EMULATE_STATE_NOTINIT 0
#define EMULATE_STATE_START_DISCOVERY 1
#define EMULATE_STATE_DISCOVERY 2
static systemcode_t ceNFCF_SC      = 0x0003;
static uint8_t ceNFCF_SENSF_RES[]  = {0x01,                                                   /* SENSF_RES                                */
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             /* NFCID2                                   */
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             /* PAD0, PAD01, MRTIcheck, MRTIupdate, PAD2 */
                                    0x00, 0x00 };


static uint8_t felica_idm[FELICA_IDM_SIZE];
static uint8_t felica_pmm[FELICA_PMM_SIZE];

rfalNfcDiscoverParam discoverparam;

uint8_t state=0;
Felica felica;

//felica読み込み初期化
bool nfcinit_pollf(){
    ReturnCode err=rfalNfcInitialize();
    if(err!=RFAL_ERR_NONE){
        return false;
    }
    rfalNfcDefaultDiscParams(&discoverparam);
    discoverparam.devLimit=1U;
    discoverparam.notifyCb=notify;
    discoverparam.totalDuration=10U;
    discoverparam.techs2Find=RFAL_NFC_TECH_NONE;

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
    //NFC Fエミュレート用パラメータ
    memcpy(discoverparam.lmConfigPF.SC,(uint8_t*)&ceNFCF_SC,RFAL_LM_SENSF_SC_LEN);
    memcpy(&ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN],felica_idm,RFAL_NFCID2_LEN);
    memcpy(&ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN+RFAL_NFCID2_LEN],felica_pmm,RFAL_NFCID2_LEN);
    memcpy(discoverparam.lmConfigPF.SENSF_RES,ceNFCF_SENSF_RES,RFAL_LM_SENSF_RES_LEN);
    discoverparam.techs2Find|=RFAL_NFC_LISTEN_TECH_F;

    err=rfalNfcDiscover(&discoverparam);
    rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
    if(err!=RFAL_ERR_NONE){
        return false;
    }
    state=EMULATE_STATE_START_DISCOVERY;

    return true;
}
void setup(){
    Serial.begin(9600);
    delay(1000);
    Serial.printf("Setup\n");
    spiInit();
    delay(1000);

    if(nfcinit_pollf()){
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
                    NFCF(&nfcDevice->dev.nfcf);
                break;

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

    FelicaCMD cmd;

    //SEND polling 0x0003
    cmd.polling.setup(0xFFFF,FELICA_POLLING_REQUEST_CODE_SYSTEMCODE,0x00);
    err=TransceiveBlocking((uint8_t*)&cmd,cmd.polling.get_size()*8,&rxData,&rcvLen,RFAL_FWT_NONE);
    //配列の一つ目はサイズ
    FelicaRES::Polling pollres(&rxData[1],rxData[0]-1);

    //idm pmmを保存
    memcpy(felica_idm,pollres.idm,FELICA_IDM_SIZE);
    memcpy(felica_pmm,pollres.pmm,FELICA_PMM_SIZE);
    systemcode_t mainsyscode=*(_uint16_b*)pollres.req_data;
    ceNFCF_SC=mainsyscode;
    //felica初期化
    felica=Felica(felica_pmm,mainsyscode,felica_idm);
    platformLog("Systemcode:0x%04X\n",(uint16_t)mainsyscode);
    platformLog("idm:");
    for(uint8_t i=0;i<FELICA_IDM_SIZE;i++){
        platformLog("%02X ",pollres.idm[i]);
    }
    platformLog("\n");
    platformLog("pmm:");
    for(uint8_t i=0;i<FELICA_PMM_SIZE;i++){
        platformLog("%02X ",pollres.pmm[i]);
    }
    platformLog("\n");

    //システム読み取り
    systemcode_t systemcodes[FELICA_MAX_SYSTEM_COUNT];
    uint8_t systemidms[FELICA_MAX_SYSTEM_COUNT][FELICA_IDM_SIZE];
    cmd.request_system_code.setup(felica_idm);
    err=TransceiveBlocking((uint8_t*)&cmd,cmd.request_system_code.get_size()*8,&rxData,&rcvLen,RFAL_FWT_NONE);
    const uint8_t systemcount=min(rxData[10],FELICA_MAX_SYSTEM_COUNT);
    platformLog("%d systems found\n",systemcount);
    for(uint8_t i=0;i<systemcount;i++){
        systemcodes[i]=*(systemcode_t*)&rxData[11+2*i];
    }
    for(uint8_t i=0;i<systemcount;i++){
        cmd.polling.setup(systemcodes[i],FELICA_POLLING_REQUEST_CODE_NONE,0x00);
        err=TransceiveBlocking((uint8_t*)&cmd,cmd.polling.get_size()*8,&rxData,&rcvLen,RFAL_FWT_NONE);
        //配列の一つ目はサイズ
        FelicaRES::Polling currentpollres(&rxData[1],rxData[0]-1);
        memcpy(systemidms[i],currentpollres.idm,FELICA_IDM_SIZE);

        if(1U<=i){
            felica.separate_system(FELICA_BLOCK_COUNT/4,systemcodes[i],systemidms[i]);
        }
        platformLog("System%02d:0x%04X\n",i,(uint16_t)systemcodes[i]);
        platformLog("idm:");
        for(uint8_t idmi=0;idmi<FELICA_IDM_SIZE;idmi++){
            platformLog("%02X ",systemidms[i][idmi]);
        }
        platformLog("\n");
    }

    felica.initialize_1st();
    platformLog("Read BLOCKS\n");

    for(uint8_t systemi=0;systemi<systemcount;systemi++){
        platformLog("Switch system:%d\n",felica.switch_system(systemidms[systemi]));

        
        servicecode_t services[32];
        uint8_t scount=0;
        //サービスコード全探索
        for(uint8_t i=1;i<0xFF;i++){
            cmd.search_service_code.setup(systemidms[systemi],0);
            cmd.search_service_code.service_index=i;
            err=TransceiveBlocking((uint8_t*)&cmd,cmd.search_service_code.get_size()*8,&rxData,&rcvLen,RFAL_FWT_NONE);
            areacode_t code=(areacode_t)(*(_uint16_l*)&rxData[10]);
            servicecode_t end_service_code;
            if(code==0xFFFF){
                break;
            }

            if(felica_is_Area_Code(code)){
                end_service_code=(servicecode_t)(*(_uint16_l*)&rxData[12]);
            }

            //鍵バージョン取得
            cmd.request_service.setup(systemidms[systemi],1,(_uint16_l*)&code);
            err=TransceiveBlocking((uint8_t*)&cmd,cmd.request_service.get_size()*8,&rxData,&rcvLen,RFAL_FWT_NONE);
            const uint16_t keyver=*(_uint16_l*)&rxData[11];

            if(felica_is_Area_Code(code)){
                platformLog("area:%04X end:%04X\n",code,end_service_code);
                platformLog("keyver%04X\n",keyver);
                platformLog("addarea:%d\n",felica.add_area(code,end_service_code,keyver));
            }
            else if(!felica_Service_is_Auth_Required(code)){
                platformLog("service code :%04X\n",code);

                services[scount]=code;
                scount++;

                cmd.read_without_encryption.setup(systemidms[systemi],1,1);
                cmd.read_without_encryption.set_service(0,code);
                uint8_t b=0;
                uint16_t status;
                do{
                    cmd.read_without_encryption.set_block_list_element(0,BlockListElement(0,0,b));
                    err=TransceiveBlocking((uint8_t*)&cmd,cmd.read_without_encryption.get_size()*8,&rxData,&rcvLen,RFAL_FWT_NONE);
                    status=rxData[10];
                    status=(status<<8)|rxData[11];
                    /*
                    if(status==0x0000){
                        memcpy(block_data_buf[b],&rxData[13],FELICA_BLOCK_SIZE);
                    }
                    //*/
                    b++;
                    /*
                    platformLog("rx:");
                    for(uint8_t i=0;i<(*rcvLen)/8;i++){
                        platformLog("%02X ",rxData[i]);
                    }
                    platformLog("\n");
                    //*/
                }while(status==0x0000);
                b--;
                platformLog("service size:%d\n",b);
                platformLog("addservice:%d\n",felica.add_service(b,code,keyver));
            }
            else{
                platformLog("Auth req service:%04X\n",code);
            }
            /*
            platformLog("rx:");
            for(uint8_t i=0;i<(*rcvLen)/8;i++){
                platformLog("%02X ",rxData[i]);
            }
            platformLog("\n");
            //*/
        }
        platformLog("init2:%d\n",felica.initialize_2nd(systemidms[systemi]));

        cmd.read_without_encryption.setup(systemidms[systemi],1,1);
        for(uint8_t i=0;i<scount;i++){
            servicecode_t servicecode=services[i];
            platformLog("service code:%04X\n",servicecode);
            uint8_t b=0;
            uint16_t status;
            uint8_t block_data_buf[FELICA_BLOCK_SIZE];
            cmd.read_without_encryption.set_service(0,servicecode);
            do{
                cmd.read_without_encryption.set_block_list_element(0,BlockListElement(0,0,b));
                err=TransceiveBlocking((uint8_t*)&cmd,cmd.read_without_encryption.get_size()*8,&rxData,&rcvLen,RFAL_FWT_NONE);
                status=rxData[10];
                status=(status<<8)|rxData[11];
                if(status==0x0000){
                    memcpy(block_data_buf,&rxData[13],FELICA_BLOCK_SIZE);
                    platformLog("blocknum:%d ",b);
                    platformLog("write:%d\n",felica.write_force(servicecode,b,block_data_buf));
                    platformLog("data:");
                    for(uint8_t i=0;i<FELICA_BLOCK_SIZE;i++){
                        platformLog("%02X ",block_data_buf[i]);
                    }
                    platformLog("\n");
                }
                b++;
                /*
                platformLog("rx:");
                for(uint8_t i=0;i<(*rcvLen)/8;i++){
                    platformLog("%02X ",rxData[i]);
                }
                platformLog("\n");
                //*/
            }while(status==0x0000);
            b--;
        }
    }

    show_block();

    nfcinit();
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
                if(nfcDev->type==RFAL_NFC_POLL_TYPE_NFCF){
                    const uint8_t rxLen=rxData[0]-1;
                    felica.listen(&rxData[1],rxLen,txBuf,&txLen);
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

void show_block(){
        for(uint16_t i=0;i<FELICA_BLOCK_COUNT;i++){
            platformLog("addr:%016p ",felica.block[i]);
            platformLog("%04d : ",i);
            for(uint8_t j=0;j<FELICA_BLOCK_SIZE;j++){
                platformLog("%02X ",felica.block[i].data[j]);
            }
            platformLog("\n");
        }
    }