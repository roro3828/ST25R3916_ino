#ifndef PLATFORM_H
#define PLATFORM_H
/*
******************************************************************************
* INCLUDES
******************************************************************************
*/

#include <Arduino.h>
#include <SPI.h>

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

#define ST25R3916
#define SPI_BUF_SIZE 520U

/*!< Use single Transceive */
#define ST25R_COM_SINGLETXRX
/*!< GPIO pin used for ST25R SPI SS */
#define ST25R_SS_PIN 1
/*!< GPIO port used for ST25R SPI SS port */
#define ST25R_SS_PORT ST25R_SS_PIN
/*!< GPIO pin used for ST25R External Interrupt */
#define ST25R_INT_PIN 26
/*!< GPIO port used for ST25R External Interrupt */
#define ST25R_INT_PORT ST25R_INT_PIN
#ifdef LED_FIELD_Pin
    /*!< GPIO pin used as field LED */
    #define PLATFORM_LED_FIELD_PIN LED_FIELD_Pin
#endif
#ifdef LED_FIELD_GPIO_Port
    /*!< GPIO port used as field LED */
    #define PLATFORM_LED_FIELD_PORT LED_FIELD_GPIO_Port
#endif
#ifdef LED_RX_Pin
    /*!< GPIO pin used as field LED */
    #define PLATFORM_LED_RX_PIN LED_RX_Pin
#endif
#ifdef LED_RX_GPIO_Port
    /*!< GPIO port used as field LED */
    #define PLATFORM_LED_RX_PORT LED_RX_GPIO_Port
#endif
/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
/* Global Protection Counter provided per platform - instantiated in main.c */
extern uint8_t globalCommProtectCnt;

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

/*!< Protect unique access to ST25R communication channel - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment */
#define platformProtectST25RComm() noInterrupts()
/*!< Unprotect unique access to ST25R communication channel - IRQ enable on a single thread environment(MCU) ; Mutex unlock on a multi thread environment */
#define platformUnprotectST25RComm() interrupts()
/*!< Protect unique access to IRQ status var - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment */
#define platformProtectST25RIrqStatus() platformProtectST25RComm()
/*!< Unprotect the IRQ status var - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment */
#define platformUnprotectST25RIrqStatus() platformUnprotectST25RComm()
/*!< Turns the given GPIO High*/
#define platformGpioSet( port, pin ) digitalWrite(pin,HIGH)
/*!< Turns the given GPIO Low*/
#define platformGpioClear( port, pin ) digitalWrite(pin,LOW)
/*!< Toogles the given GPIO*/
#define platformGpioToogle( port, pin ) digitalWrite(pin,!digitalRead(pin));
/*!< Checks if the given GPIO is High*/
#define platformGpioIsHigh( port, pin ) (digitalRead(pin)==HIGH)
/*!< Checks if the given GPIO is Low */
#define platformGpioIsLow( port, pin ) (!platformGpioIsHigh(port,pin))
/*!< Create a timer with the given time (ms) */
#define platformTimerCreate( t ) (t+millis())
/*!< Checks if the given timer is expired */
#define platformTimerIsExpired( timer ) (timer<=millis())
/*!< Performs a delay for the given time (ms) */
#define platformDelay( t ) delay(t)
/*!< SPI SS\CS: Chip|Slave Select */
#define platformSpiSelect() spiSelect()
/*!< SPI SS\CS: Chip|Slave Deselect */
#define platformSpiDeselect() spiDeselect()
/*!< SPI transceive */
#define platformSpiTxRx( txBuf, rxBuf, len ) spiTxRx(txBuf, rxBuf,len)

#define platformI2CTx( txBuf, len, last, txOnly ) /*!< I2C Transmit */
#define platformI2CRx( txBuf, len )/*!< I2C Receive */
#define platformI2CStart() /*!< I2C Start condition */
#define platformI2CStop() /*!< I2C Stop condition */
#define platformI2CRepeatStart() /*!< I2C Repeat Start */
#define platformI2CSlaveAddrWR(add) /*!< I2C Slave address for Write operation */
#define platformI2CSlaveAddrRD(add) /*!< I2C Slave address for Read operation */
#define platformLog( ... ) Serial.printf( __VA_ARGS__ )

#define platformIrqST25RPinInitialize() pinMode(ST25R_INT_PIN,INPUT_PULLUP)
#define platformIrqST25RSetCallback(cb) attachInterrupt(digitalPinToInterrupt(ST25R_INT_PIN),cb,RISING)

void spiTxRx(uint8_t* txbuf,uint8_t* rxbuf,size_t len);
void spiSelect();
void spiDeselect();
void spiInit();

/*
******************************************************************************
* RFAL FEATURES CONFIGURATION
******************************************************************************
*/
/*!< Enable/Disable RFAL support for Listen Mode */
#define RFAL_FEATURE_LISTEN_MODE true
/*!< Enable/Disable RFAL support for the Wake-Up mode */
#define RFAL_FEATURE_WAKEUP_MODE true
/*!< Enable/Disable RFAL support for the Low Power mode */
#define RFAL_FEATURE_LOWPOWER_MODE true
/*!< Enable/Disable RFAL support for NFC-A (ISO14443A) */
#define RFAL_FEATURE_NFCA true
/*!< Enable/Disable RFAL support for NFC-B (ISO14443B) */
#define RFAL_FEATURE_NFCB true
/*!< Enable/Disable RFAL support for NFC-F (FeliCa) */
#define RFAL_FEATURE_NFCF true
/*!< Enable/Disable RFAL support for NFC-V (ISO15693) */
#define RFAL_FEATURE_NFCV false
/*!< Enable/Disable RFAL support for T1T (Topaz) */
#define RFAL_FEATURE_T1T true
/*!< Enable/Disable RFAL support for T2T (MIFARE Ultralight) */
#define RFAL_FEATURE_T2T true
/*!< Enable/Disable RFAL support for T4T */
#define RFAL_FEATURE_T4T true
/*!< Enable/Disable RFAL support for ST25TB */
#define RFAL_FEATURE_ST25TB false
/*!< Enable/Disable RFAL support for ST25TV/ST25DV */
#define RFAL_FEATURE_ST25xV false
/*!< Enable/Disable Analog Configs to be dynamically updated (RAM) */
#define RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG false
/*!< Enable/Disable RFAL Dynamic Power Output upport */
#define RFAL_FEATURE_DPO false
/*!< Enable/Disable RFAL support for ISO-DEP (ISO14443-4) */
#define RFAL_FEATURE_ISO_DEP true
/*!< Enable/Disable RFAL support for Poller mode (PCD) ISO-DEP (ISO14443-4) */
#define RFAL_FEATURE_ISO_DEP_POLL true
/*!< Enable/Disable RFAL support for Listen mode (PICC) ISO-DEP (ISO14443-4) */
#define RFAL_FEATURE_ISO_DEP_LISTEN true
/*!< Enable/Disable RFAL support for NFC-DEP (NFCIP1/P2P) */
#define RFAL_FEATURE_NFC_DEP true
/*!< ISO-DEP I-Block max length. Please use values as defined by rfalIsoDepFSx */
#define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN 256U
/*!< NFC-DEP Block/ Payload length. Allowed values: 64, 128, 192, 254 */
#define RFAL_FEATURE_NFC_DEP_BLOCK_MAX_LEN 254U
/*!< RF buffer length used by RFAL NFC layer */
#define RFAL_FEATURE_NFC_RF_BUF_LEN 256U
/*!< ISO-DEP APDU max length. */
#define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN 512U
/*!< NFC-DEP PDU max length. */
#define RFAL_FEATURE_NFC_DEP_PDU_MAX_LEN 512U
/*
 ******************************************************************************
 * RFAL OPTIONAL MACROS (Do not change)
 ******************************************************************************
 */
#ifndef platformProtectST25RIrqStatus
    /*!< Protect unique access to IRQ status var - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment */
    #define platformProtectST25RIrqStatus()
#endif /* platformProtectST25RIrqStatus */
#ifndef platformUnprotectST25RIrqStatus
    /*!< Unprotect the IRQ status var - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment */
    #define platformUnprotectST25RIrqStatus()
#endif /* platformUnprotectST25RIrqStatus */
#ifndef platformProtectWorker
    /* Protect RFAL Worker/Task/Process from concurrent execution on multi thread platforms */
    #define platformProtectWorker()
#endif /* platformProtectWorker */
#ifndef platformUnprotectWorker
    /* Unprotect RFAL Worker/Task/Process from concurrent execution on multi thread platforms */
    #define platformUnprotectWorker()
#endif /* platformUnprotectWorker */
#ifndef platformIrqST25RPinInitialize
    /*!< Initializes ST25R IRQ pin */
    #define platformIrqST25RPinInitialize()
#endif /* platformIrqST25RPinInitialize */
#ifndef platformIrqST25RSetCallback
    /*!< Sets ST25R ISR callback */
    #define platformIrqST25RSetCallback( cb )
#endif /* platformIrqST25RSetCallback */
#ifndef platformLedsInitialize
    /*!< Initializes the pins used as LEDs to outputs */
    #define platformLedsInitialize()
#endif /* platformLedsInitialize */
#ifndef platformLedOff
    /*!< Turns the given LED Off */
    #define platformLedOff( port, pin )
#endif /* platformLedOff*/
#ifndef platformLedOn
    /*!< Turns the given LED On */
    #define platformLedOn( port, pin )
#endif /* platformLedOn*/
#ifndef platformLedToogle
    /*!< Toggles the given LED */
    #define platformLedToogle( port, pin )
#endif /* platformLedToogle*/
#ifndef platformGetSysTick
    /*!< Get System Tick (1 tick = 1 ms) */
    #define platformGetSysTick()
#endif /* platformGetSysTick*/
#ifndef platformTimerDestroy
    /*!< Stops and released the given timer */
    #define platformTimerDestroy( timer )
#endif /* platformTimerDestroy */
#ifndef platformAssert
    /*!< Asserts whether the given expression is true */
    #define platformAssert( exp )
#endif /* platformAssert */
#ifndef platformErrorHandle
    /*!< Global error handler or trap */
    #define platformErrorHandle()
#endif /* platformErrorHandle */

#endif /* PLATFORM_H */