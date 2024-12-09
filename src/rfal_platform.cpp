#include"rfal_platform.h"
//#define SPI_DEBUG


uint8_t _spi_buf[SPI_BUF_SIZE];
SPISettings _spi_settings;

void spiTxRx(uint8_t* txbuf,uint8_t* rxbuf,size_t len){
    if((txbuf!=NULL)&&(rxbuf!=NULL)){
        #ifdef SPI_DEBUG
            Serial.printf("tx <- :");
            for(size_t i=0;i<len;i++){
                Serial.printf("%02X ",txbuf[i]);
            }
            Serial.printf("\n");
        #endif
        SPI.transfer(txbuf,rxbuf,len);
        #ifdef SPI_DEBUG
            Serial.printf("rx -> :");
            for(size_t i=0;i<len;i++){
                Serial.printf("%02X ",rxbuf[i]);
            }
            Serial.printf("\n");
        #endif
    }
    else if(txbuf!=NULL){
        #ifdef SPI_DEBUG
            Serial.printf("tx <- :");
            for(size_t i=0;i<len;i++){
                Serial.printf("%02X ",txbuf[i]);
            }
        #endif
        memcpy(_spi_buf,txbuf,len);
        SPI.transfer(_spi_buf,len);
    }
    else if(rxbuf!=NULL){
        memset(rxbuf,0,len);
        SPI.transfer(rxbuf,len);
        #ifdef SPI_DEBUG
            Serial.printf("rx -> :");
            for(size_t i=0;i<len;i++){
                Serial.printf("%02X ",rxbuf[i]);
            }
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