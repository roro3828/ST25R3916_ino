#include<iostream>
#include"felica.hpp"
#include"transport_ic.hpp"

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

    fe.listen_Read_Without_Encryption(idm,1,(_uint16_l*)rxBuf,2,&rxBuf[2],txBuf,&txBufLen);
    //fe.listen_Request_System_Code(idm,txBuf,&txBufLen);

    printf("tx:");
    for(uint16_t i=0;i<txBufLen;i++){
        printf("%02X ",txBuf[i]);
    }
    printf("\n");

    uint8_t td[]={0x11,0x22,0x33,0x44,0x15,0x21,0x0F,0x40,0x08,0x09,0xFF,0xFF,0x0C,0x0D,0x0E,0x0F};
    TransportIC::usage_history_data* hd=(TransportIC::usage_history_data*)td;
    printf("year:%d month:%d day:%d\n",(uint8_t)hd->date.year,(uint8_t)hd->date.month,(uint8_t)hd->date.day);
    hd->device_type=0x11;
    hd->usage_type=0x22;
    hd->payment_type=0x33;
    hd->enter_type=0x44;
    hd->date.year=10;
    hd->date.month=9;
    hd->date.day=1;
    hd->enter_station.line_code=0x11;
    hd->enter_station.station_order_code=0x12;
    hd->exit_station.line_code=0x21;
    hd->exit_station.station_order_code=0x22;
    hd->balance=0xABCD;
    for(uint8_t i=0;i<16;i++){
        printf("0x%02X,",td[i]);
    }
    printf("\n");

    FelicaCMD felicacmd;

    felicacmd.polling.setup(0x0003,FELICA_POLLING_REQUEST_CODE_SYSTEMCODE,0x00);
    fe.listen((uint8_t*)&felicacmd,felicacmd.polling.get_size(),txBuf,&txBufLen);
        printf("res:");
        for(uint8_t i=0;i<txBufLen;i++){
            printf("0x%02X,",txBuf[i]);
        }
        printf("\n");
    FelicaRES::Polling pollres(txBuf,txBufLen);

    printf("idm:");
    for(uint8_t i=0;i<FELICA_IDM_SIZE;i++){
        printf("%02X ",pollres.idm[i]);
    }
    printf("\n");


}