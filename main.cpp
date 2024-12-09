#include<iostream>
#include"felica.hpp"

static const uint8_t idm[]={0xF1,0x02,0x03,0x04,0x05,0x06,0x07,0x8F};
static const uint8_t pmm[]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77};
static const systemcode_t system_code=0x0003;

static const servicecode_t service_code=0x090F;

static uint16_t rxBufLen;
static uint8_t rxBuf[512];
static uint16_t txBufLen;
static uint8_t txBuf[512];

uint8_t block_data[20][16]={
    {18,0x02,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x42,0x82,0xc7,0x01,0x00,0x00,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
    {26,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F}
    };

int main(){

    Felica felica(pmm);
    felica.add_system(system_code,idm);

    felica.listen_Polling(system_code,FELICA_POLLING_REQUEST_CODE_SYSTEMCODE,0x00,txBuf,&txBufLen);

    printf("poll tx:");
    for(int i=0;i<txBufLen;i++){
        printf("%02X ",txBuf[i]);
    }
    printf("\n");

    FelicaSystem *fsystem;
    FelicaService *service;
    felica.get_system(system_code,&fsystem);
    fsystem->add_service(service_code,33);
    fsystem->get_service(service_code,&service);
    service->service_key_ver=0x1234;

    uint8_t list[16]={service_code&0xFF,(service_code>>8)&0xFF};
    felica.listen_Request_Service(idm,1,list,txBuf,&txBufLen);

    printf("reqs tx:");
    for(int i=0;i<txBufLen;i++){
        printf("%02X ",txBuf[i]);
    }
    printf("\n");

    felica.listen_Request_Response(idm,txBuf,&txBufLen);
    printf("reqr tx:");
    for(int i=0;i<txBufLen;i++){
        printf("%02X ",txBuf[i]);
    }
    printf("\n");

    fsystem->get_service(service_code,&service);

    for(uint16_t i=0;i<20;i++){
        block_data[i][12]=0x00;
        block_data[i][13]=0x00;
        block_data[i][14]=i*2;
        printf("write %d\n",service->write_block_force(0,block_data[i]));
        uint8_t buf[16];
        service->read_block_force(0,buf);
        printf("read ");
        for(uint16_t i=0;i<16;i++){
            printf("%02X ",buf[i]);
        }
        printf("\n");
    }

    const BlockListElement *block_list[32];
    for(int i=0;i<32;i++){
        block_list[i]=new BlockListElement(0,0,1+i);
    }
    uint16_t idx=0;
    rxBuf[0]=FELICA_READ_WITHOUT_ENCRYPTION_CMD_CODE;
    memcpy(&rxBuf[1],idm,FELICA_IDM_SIZE);
    rxBuf[9]=0x01;
    rxBuf[10]=service_code&0xFF;
    rxBuf[11]=(service_code>>8)&0xFF;
    rxBuf[12]=32;
    for(int i=0;i<32;i++){
        block_list[i]->set_element_to_buf(&rxBuf[13+idx]);
        if(block_list[i]->len==1){
            idx+=2;
        }
        else{
            idx+=3;
        }
    }
    rxBufLen=13+idx;
    printf("read rx:");
    for(int i=0;i<rxBufLen;i++){
        printf("%02X ",rxBuf[i]);
    }
    printf("\n");



    felica.listen(rxBuf,rxBufLen,txBuf,&txBufLen);
    printf("read tx:");
    for(int i=0;i<txBufLen;i++){
        printf("%02X ",txBuf[i]);
    }
    printf("\n");

    felica.listen_Request_System_Code(idm,txBuf,&txBufLen);
    printf("reqs tx:");
    for(int i=0;i<txBufLen;i++){
        printf("%02X ",txBuf[i]);
    }
    printf("\n");
}