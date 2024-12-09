#ifndef FELICA_HPP
#define FELICA_HPP

#include<stdio.h>
#include<cstring>
#include<cstdint>


//デバッグ
#define _DEBUG_

using blockcount_t=uint16_t;


#define FELICA_BLOCK_COUNT 128U
/**
 * FelicaのIDmのサイズ
 */
#define FELICA_IDM_SIZE 8U
/**
 * FelicaのPMmのサイズ
 */
#define FELICA_PMM_SIZE 8U
/**
 * イニシャライズ
 */
#define FELICA_NOT_INITIALIZED 0U
#define FELICA_1ST_INITIALIZED 1U
#define FELICA_2ND_INITIALIZED 2U

#define FELICA_SYSTEM0_INDEX 0U


/**
 * Felicaのシステムコードのサイズ
 */
#define FELICA_SYSTEM_CODE_SIZE 2U
using systemcode_t=uint16_t;
#define FELICA_SYSTEM_DATA_SIZE       2U
#define FELICA_SYSTEM_MIN_BLOCK_COUNT (FELICA_SYSTEM_DATA_SIZE+1U)
/**
 * Felicaのコマンドの長さ
 */
#define FELICA_CMD_SIZE 1U

/**
 * Polling用定数
 */
#define FELICA_POLLING_CMD_CODE 0x00
#define FELICA_POLLING_RES_CODE 0x01
#define FELICA_POLLING_REQUEST_CODE_SIZE 1U
#define FELICA_POLLING_REQUEST_CODE_NONE 0x00
#define FELICA_POLLING_REQUEST_CODE_SYSTEMCODE 0x01
#define FELICA_POLLING_TIME_SLOT_SIZE 1U

/**
 * エリア用定数
 */
using areacode_t=uint16_t;
#define FELICA_AREA_CODE_SIZE                   2U
#define FELICA_AREA_NUM_MASK                    0b1111111111000000
#define FELICA_AREA_ATTR_MASK                   0b0000000000111111
#define FELICA_AREA_ATTR_LEN_BIT                6U
#define felica_Area_Code_to_Attr(c)             (c&FELICA_AREA_ATTR_MASK)
//入力値がエリアコードかどうか
#define felica_is_Area_Code(c)                  (((c&FELICA_AREA_ATTR_MASK)==FELICA_AREA_ATTR_CAN_CREATE_SUB)||((c&FELICA_AREA_ATTR_MASK)==FELICA_AREA_ATTR_CANNOT_CREATE_SUB))
#define FELICA_AREA_ATTR_CAN_CREATE_SUB         0b000000
#define FELICA_AREA_ATTR_CANNOT_CREATE_SUB      0b000001
#define FELICA_AREA0_AREA_CODE                  0x0000
#define FELICA_AREA0_END_SERVICE_CODE           0xFFFE
#define FELICA_AREA_KEY_VER_NOT_SET             0xFFFF
#define FELICA_AREA_DATA_SIZE                   1U

/**
 * サービス用定数
 */
using servicecode_t=uint16_t;
#define FELICA_SERVICE_CODE_SIZE                2U
#define FELICA_SERVICE_CODE_NUM_MASK            0b1111111111000000
#define FELICA_SERVICE_CODE_ATTR_MASK           0b0000000000111111
#define FELICA_SERVICE_ATTR_LEN_BIT             6U
#define felica_Service_Code_to_Attr(c)          (c&FELICA_SERVICE_CODE_ATTR_MASK)
#define FELICA_SERVICE_ATTR_AUTH_MASK           0b000001
#define FELICA_SERVICE_ATTR_AUTH_NEED           0b000000
#define FELICA_SERVICE_ATTR_AUTH_NO_NEED        0b000001
#define felica_Service_is_Auth_Required(c)      ((felica_Service_Code_to_Attr(c)&FELICA_SERVICE_ATTR_AUTH_MASK)==FELICA_SERVICE_ATTR_AUTH_NEED)
#define FELICA_SERVICE_ATTR_ACCESS_MASK         0b010010
#define FELICA_SERVICE_ATTR_ACCESS_RW           0b000000
#define FELICA_SERVICE_ATTR_ACCESS_R            0b000010
#define FELICA_SERVICE_ATTR_TYPE_MASK           0b111100
#define FELICA_SERVICE_ATTR_TYPE_RANDOM         0b001000
#define FELICA_SERVICE_ATTR_TYPE_CYCLIC         0b001100
#define felica_is_Service_Cyclic(c)             ((c&FELICA_SERVICE_ATTR_TYPE_MASK)==FELICA_SERVICE_ATTR_TYPE_CYCLIC)
#define FELICA_SERVICE_PURSE_TYPE_MASK          0b110000
#define felica_is_Service_Code(c)               (!felica_is_Area_Code(c))
//サービスコードがパースサービスかどうか
#define felica_Service_is_Purse_service(c)      (!(((felica_Service_Code_to_Attr(c)&FELICA_SERVICE_ATTR_TYPE_MASK)==FELICA_SERVICE_ATTR_TYPE_RANDOM)||((felica_Service_Code_to_Attr(c)&FELICA_SERVICE_ATTR_TYPE_MASK)==FELICA_SERVICE_ATTR_TYPE_CYCLIC)))
#define felica_Service_Code_to_Num(c)           (felica_Service_is_Purse_service(c)?(FELICA_SERVICE_CODE_NUM_MASK|FELICA_SERVICE_PURSE_TYPE_MASK)&c:(FELICA_SERVICE_CODE_NUM_MASK|FELICA_SERVICE_ATTR_TYPE_MASK)&c)
#define FELICA_SERVICE_KEY_VER_NOT_SET          0xFFFF
#define FELICA_SERVICE_DATA_SIZE                1U

#define FELICA_BLOCK_METADATA_SIZE              1U

/**
 * Request Service用定数
 */
#define FELICA_REQUEST_SERVICE_CMD_CODE 0x02
#define FELICA_REQUEST_SERVICE_RES_CODE 0x03
#define FELICA_BLOCK_SIZE 16U

/**
 * Request Responseよう定数
 */
#define FELICA_REQUEST_RESPONSE_CMD_CODE 0x04
#define FELICA_REQUEST_RESPONSE_RES_CODE 0x05

/**
 * Read Without Encryption用定数
 */
#define FELICA_READ_WITHOUT_ENCRYPTION_CMD_CODE 0x06
#define FELICA_READ_WITHOUT_ENCRYPTION_RES_CODE 0x07
#define FELICA_READ_WITHOUT_ENCRYPTION_BUF_SIZE 32U


/**
 * Request System Code用定数
 */
#define FELICA_REQUEST_SYSTEM_CODE_CMD_CODE 0x0C
#define FELICA_REQUEST_SYSTEM_CODE_RES_CODE 0x0D

/**
 * Felicaに登録できるシステムの最大数
 */
#define FELICA_MAX_SYSTEM_COUNT 4U
/**
 * エリアに登録できるサービスの最大数
 */
#define FELICA_MAX_SERVICE_COUNT 4U
/**
 * 一つのシステム/エリアに登録できるエリアの最大数
 */
#define FELICA_MAX_AREA_COUNT   4U
/**
 * 送信に必要なバッファのサイズ
 */
#define FELICA_TX_BUF_SIZE 128U

/**
 * BlockListElement
 */
#define FELICA_BLOCK_LIST_ELEMENT_MIN_SIZE 2U
#define FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE 3U
#define FELICA_BLOCK_LIST_ELEMENT_LEN_BIT 1U
#define FELICA_BLOCK_LIST_ELEMENT_ACCESS_MODE_BIT 3U
#define FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT 4U

struct FelicaSystem{
    systemcode_t systemcode;
    uint16_t system_key_ver;
    uint16_t issuer_id;
    blockcount_t system_block_count;
    uint8_t idm[FELICA_IDM_SIZE];
    //割り当て済みのエリア/サービス数
    blockcount_t area_service_data_used_count;
};


struct FelicaArea{
    areacode_t areacode;
    servicecode_t end_servicecode;
    uint16_t area_key_ver;
    //エリアが管理するブロック数
    blockcount_t area_block_count;
    //使用済みのブロック数
    blockcount_t area_block_used;
};
struct FelicaService{
    servicecode_t servicecode;
    uint16_t service_key_ver;
    //サービスが管理するブロック数
    blockcount_t service_block_count;
    //サービスが管理するブロックの場所
    blockcount_t service_block_index;
    //サービが参照するサービスの場所
    blockcount_t reference_index;
};
struct FelicaBlockData{
    blockcount_t cyclic_index;
};

class Felica{
    uint8_t block[FELICA_BLOCK_COUNT][FELICA_BLOCK_SIZE];
    uint8_t current_mode=0x00;
    FelicaSystem *current_system;
    uint8_t pmm[FELICA_PMM_SIZE];
    uint8_t initialized=0x00;


    FelicaService *get_service(const servicecode_t &servicecode);

    public:

    //デバッグ用
    #ifdef _DEBUG_
        void show_block();
    #endif//_DEBUG_

    Felica(const uint8_t (&pmm)[FELICA_PMM_SIZE],const systemcode_t &systemcode,const uint8_t (&idm)[FELICA_IDM_SIZE]);

    /**
     * @brief idmの比較
     * @param idm1
     * @param idm2
     * @return true:idmが等しいとき
     */
    static bool compare_idm(const uint8_t (&idm1)[FELICA_IDM_SIZE],const uint8_t (&idm2)[FELICA_IDM_SIZE]);

    /**
     * @brief pmmを取得
     * @param[out] pmm
     */
    void get_pmm(uint8_t (&pmm)[FELICA_PMM_SIZE]);

    FelicaSystem* get_system(const uint8_t (&idm)[FELICA_IDM_SIZE])const;
    FelicaSystem* get_system(const systemcode_t &systemcode)const;

    FelicaSystem* switch_system(const uint8_t (&idm)[FELICA_IDM_SIZE]);
    FelicaSystem* switch_system(const systemcode_t &systemcode);

    /**
     * @brief システム0を分割して新しいシステムを作る
     */
    FelicaSystem* separate_system(const blockcount_t &new_system_size,const systemcode_t &systemcode,const uint8_t (&idm)[FELICA_IDM_SIZE]);
    FelicaSystem* separate_system(const blockcount_t &new_system_size,const systemcode_t &systemcode);

    /**
     * @brief 1次初期化 システムを確定させる これ以降システムを分割できない
     */
    void initialize_1st();

    /**
     * @brief エリアを追加
     */
    FelicaArea* add_area(const blockcount_t &new_area_size,const areacode_t &areacode,const servicecode_t &end_servicecode,const uint16_t &area_key_ver);
    /**
     * @brief サービスを追加
     */
    FelicaService* add_service(const blockcount_t &new_service_size,const servicecode_t &servicecode,const uint16_t &service_key_ver);

    /**
     * @brief 2次初期化 サービス/エリアを確定させる これ以降サービスやエリアを追加できない
     */
    void initialize_2nd();

    void write(const servicecode_t &servicecode,const blockcount_t &blocknum,const uint8_t (&data)[FELICA_BLOCK_SIZE]);
    void write_force(const servicecode_t &servicecode,const blockcount_t &blocknum,const uint8_t (&data)[FELICA_BLOCK_SIZE]);
};

#endif /*FELICA_HPP*/