#include<iostream>
#include"felica2.hpp"

static const uint8_t idm[]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
static const uint8_t pmm[]={0xAF,0xAF,0xAF,0xAF,0xAF,0xAF,0xAF,0xAF};


uint8_t data[FELICA_BLOCK_SIZE]={0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
uint8_t txBuf[FELICA_TX_BUF_SIZE];
uint16_t txBufLen;
uint8_t rxBuf[FELICA_TX_BUF_SIZE];
uint16_t rxBufLen;
int main(){
    Felica fe(pmm,0x0003,idm);
    fe.separate_system(10,0xAABB);
    fe.initialize_1st();

    fe.add_area(0x0040,0x07FF,0xAABB);
    fe.add_service(3,0x0088,0xEEFF);
    fe.add_service(3,0x008B,0xEFEF);
    fe.add_service(20,0x090C,0xEEFF);
    fe.add_service(20,0x090F,0xABCD);
    fe.add_area(0x00C0,0x00FF,0xBBBB);
    fe.initialize_2nd();

    fe.write(0x0088,0,data);
    data[0]=1;
    fe.write(0x090F,0,data);
    data[0]=2;
    fe.write(0x090F,0,data);
    data[0]=3;
    fe.write(0x090F,0,data);
    data[0]=4;
    fe.write(0x090F,0,data);
    fe.write(0x090F,0,data);
    fe.write(0x090F,0,data);
    fe.write(0x090F,0,data);
    fe.write(0x090F,0,data);
    fe.write(0x090F,0,data);
    fe.show_block();
    printf("\n");
    fe.read_force(0x090F,4,data);
    for(int i=0;i<FELICA_BLOCK_SIZE;i++){
        printf("%02X ",data[i]);
    }
    printf("\n");

    fe.listen_Polling(0x00FF,FELICA_POLLING_REQUEST_CODE_SYSTEMCODE,0x00,txBuf,&txBufLen);

    rxBuf[0]=0x0F;
    rxBuf[1]=0x09;
    BlockListElement b1(0x00,0,0);
    BlockListElement b2(0x00,0,2);
    b1.set_element_to_buf(&rxBuf[2]);
    b2.set_element_to_buf(&rxBuf[2+b1.get_element_len()]);
    printf("rx:");
    for(uint16_t i=0;i<10;i++){
        printf("%02X ",rxBuf[i]);
    }
    printf("\n");

    fe.listen_Read_Without_Encryption(idm,1,rxBuf,2,&rxBuf[2],txBuf,&txBufLen);
    //fe.listen_Request_System_Code(idm,txBuf,&txBufLen);

    printf("tx:");
    for(uint16_t i=0;i<txBufLen;i++){
        printf("%02X ",txBuf[i]);
    }
    printf("\n");
}