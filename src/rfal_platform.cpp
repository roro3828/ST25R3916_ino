#include"rfal_platform.h"
//#define SPI_DEBUG


uint8_t _spi_buf[SPI_BUF_SIZE];
SPISettings _spi_settings;

void spiTxRx(uint8_t* txbuf,uint8_t* rxbuf,size_t len){
    if((txbuf!=NULL)&&(rxbuf!=NULL)){
        #ifdef SPI_DEBUG
            platformLog("tx[%4d] <- :",len);
            for(size_t i=0;i<len;i++){
                platformLog("%02X ",txbuf[i]);
            }
            platformLog("\n");
        #endif
        SPI.transfer(txbuf,rxbuf,len);
        #ifdef SPI_DEBUG
            platformLog("rx[%4d] -> :",len);
            for(size_t i=0;i<len;i++){
                platformLog("%02X ",rxbuf[i]);
            }
            platformLog("\n");
        #endif
    }
    else if(txbuf!=NULL){
        #ifdef SPI_DEBUG
            platformLog("tx[%4d] <- :",len);
            for(size_t i=0;i<len;i++){
                platformLog("%02X ",txbuf[i]);
            }
            platformLog("\n");
        #endif
        memcpy(_spi_buf,txbuf,len);
        SPI.transfer(_spi_buf,len);
    }
    else if(rxbuf!=NULL){
        memset(rxbuf,0,len);
        SPI.transfer(rxbuf,len);
        #ifdef SPI_DEBUG
            platformLog("rx[%4d] -> :",len);
            for(size_t i=0;i<len;i++){
                platformLog("%02X ",rxbuf[i]);
            }
            platformLog("\n");
        #endif
    }
}
void spiSelect(){
    SPI.beginTransaction(_spi_settings);
    digitalWrite(ST25R_SS_PIN,LOW);
}
void spiDeselect(){
    digitalWrite(ST25R_SS_PIN,HIGH);
    SPI.endTransaction();
}

void spiInit(){
    #ifdef ARDUINO_ARCH_RP2040
        SPI.setTX(3);
        SPI.setRX(4);
        SPI.setSCK(2);
    #endif
    _spi_settings=SPISettings(1000000,MSBFIRST,SPI_MODE1);
    pinMode(ST25R_SS_PIN,OUTPUT);
    SPI.begin();
}