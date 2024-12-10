/**
 * @attention
 * MIT License
 * 
 * Copyright (c) 2024 ろ
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifndef FELICA_HPP
#define FELICA_HPP

#include<stdio.h>
#include<cstring>
#include<cstdint>

/***************************************************************
 * utils
 */

/**
 * リトルエンディアン16bit
 */
class _uint16_l{
    uint8_t data[2];
    public:
    _uint16_l():data{0x00,0x00}{};
    _uint16_l(uint16_t x):data{(uint8_t)(x&0xFF),(uint8_t)((x>>8)&0xFF)}{}
    operator uint16_t()const{
        uint16_t tmp=data[1];
        tmp=data[0]+(tmp<<8);
        return tmp;
    }
};
/**
 * ビッグエンディアン16bit
 */
class _uint16_b{
    uint8_t data[2];
    public:
    _uint16_b():data{0x00,0x00}{};
    _uint16_b(uint16_t x):data{(uint8_t)((x>>8)&0xFF),(uint8_t)(x&0xFF)}{}
    operator uint16_t()const{
        uint16_t tmp=data[0];
        tmp=data[1]+(tmp<<8);
        return tmp;
    }
};


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
using systemcode_t=_uint16_b;
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

class BlockListElement{
    public:
    const uint8_t len:FELICA_BLOCK_LIST_ELEMENT_LEN_BIT;
    const uint8_t access_mode:FELICA_BLOCK_LIST_ELEMENT_ACCESS_MODE_BIT;
    const uint8_t service_code_list_order:FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT;

    const uint16_t block_num;

    BlockListElement(const uint8_t &access_mode,const uint8_t &service_code_list_order,const uint16_t &block_num);
    /**
     * @brief 配列からブロックリストエレメントを生成する
     * @param[in] buf
     */
    BlockListElement(const uint8_t *buf);

    /**
     * @brief ブロックリストエレメントを配列に格納する
     * @param[out] buf 格納する配列の先頭ポインタ FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE以上の長さが必要
     * @return 格納したブロックリストエレメントの長さ
     */
    uint8_t set_element_to_buf(uint8_t *buf)const;
};


struct FelicaSystem{
    systemcode_t systemcode;
    uint16_t system_key_ver;
    uint16_t issuer_id;
    blockcount_t system_block_count;
    uint8_t idm[FELICA_IDM_SIZE];
    //割り当て済みのエリア/サービス数
    blockcount_t area_service_data_used_count;
    //割り当て済みブロック数
    blockcount_t block_used_count;
};

struct FelicaArea{
    areacode_t areacode;
    servicecode_t end_servicecode;
    uint16_t area_key_ver;
    areacode_t parent_code;
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
    areacode_t parent_code;
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
    FelicaArea *get_area(const areacode_t &areacode);

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

    /**
     * @brief 現在のシステムを切り替える 失敗した場合NULLが返される
     */
    FelicaSystem* switch_system(const uint8_t (&idm)[FELICA_IDM_SIZE]);
    /**
     * @brief 現在のシステムを切り替える 失敗した場合NULLが返される
     */
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
    FelicaArea* add_area(const areacode_t &areacode,const servicecode_t &end_servicecode,const uint16_t &area_key_ver);
    /**
     * @brief サービスを追加
     */
    FelicaService* add_service(const blockcount_t &new_service_size,const servicecode_t &servicecode,const uint16_t &service_key_ver);

    /**
     * @brief 2次初期化 サービス/エリアを確定させる これ以降サービスやエリアを追加できない
     */
    void initialize_2nd();

    /**
     * @brief 指定したサービスの指定したブロックにデータを書き込む
     */
    bool write(const servicecode_t &servicecode,const blockcount_t &blocknum,const uint8_t (&data)[FELICA_BLOCK_SIZE]);
    bool write_force(const servicecode_t &servicecode,const blockcount_t &blocknum,const uint8_t (&data)[FELICA_BLOCK_SIZE]);
    /**
     * @brief 指定したサービスの指定したブロックのデータを読み込む
     */
    bool read(const servicecode_t &servicecode,const blockcount_t &blocknum,uint8_t (&data)[FELICA_BLOCK_SIZE]);
    bool read_force(const servicecode_t &servicecode,const blockcount_t &blocknum,uint8_t (&data)[FELICA_BLOCK_SIZE]);

        /**
     * @brief Felicaのカードエミュレーションを実行する
     * @param[in] rxBuf 受信したデータ 1バイト目がコマンドコード
     * @param[in] rxBufLen 受信したデータの長さ
     * @param[out] txBuf 送信するデータを格納する 配列は FELICA_TX_BUF_SIZE 以上の長さである必要がある。
     * @param[out] txBufLen 送信するデータの長さを格納する。
     */
    void listen(const uint8_t *rxBuf,const uint16_t rxBufLen,uint8_t *txBuf,uint16_t *txBufLen);
    /**
     * @brief Felicaのカードエミュレーション Pollingを実行する https://www.sony.co.jp/Products/felica/business/tech-support/data/card_usersmanual_2.21j.pdf
     * @param[in] system_code 
     * @param[out] txBuf 送信するデータを格納する 配列は FELICA_TX_BUF_SIZE 以上の長さである必要がある。1バイト目がレスポンスコード
     * @param[out] txBufLen 送信するデータの長さを格納する。
     */
    void listen_Polling(const systemcode_t &system_code,const uint8_t &request_code,const uint8_t &time_slot,uint8_t *txBuf,uint16_t *txBufLen);
    /**
     * @brief Felicaのカードエミュレーション Request Serviceを実行する
     * @param[in] idm IDm
     * @param[in] node_count ノード数 1以上32以下
     * @param[in] node_code_list コードの配列 長さは2*node_count
     * @param[out] txBuf 送信するデータを格納する 配列は FELICA_TX_BUF_SIZE 以上の長さである必要がある。1バイト目がレスポンスコード
     * @param[out] txBufLen 送信するデータの長さを格納する。
     */
    void listen_Request_Service(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &node_count,const uint8_t *node_code_list,uint8_t *txBuf,uint16_t *txBufLen);
    /**
     * @brief Felicaのカードエミュレーション Request Responseを実行する
     * @param[in] idm IDm
     * @param[out] txBuf 送信するデータを格納する 配列は FELICA_TX_BUF_SIZE 以上の長さである必要がある。1バイト目がレスポンスコード
     * @param[out] txBufLen 送信するデータの長さを格納する。
     */
    void listen_Request_Response(const uint8_t (&idm)[FELICA_IDM_SIZE],uint8_t *txBuf,uint16_t *txBufLen);

    /**
     * @brief Felicaのカードエミュレーション Read Without Encryptionを実行する
     * @param[in] idm IDm
     * @param[in] service_count サービス数 1以上32以下
     * @param[in] service_code_list サービスコードリスト リトルエンディアンで記述する 長さは2*service_count
     * @param[in] block_count ブロック数
     * @param[out] txBuf 送信するデータを格納する 配列は FELICA_TX_BUF_SIZE 以上の長さである必要がある。1バイト目がレスポンスコード
     * @param[out] txBufLen 送信するデータの長さを格納する。
     */
    void listen_Read_Without_Encryption(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &service_count,const uint8_t *service_code_list,const uint8_t &block_count,const BlockListElement **block_list,uint8_t *txBuf,uint16_t *txBufLen);

    /**
     * @brief Felicaのカードエミュレーション Request System Codeを実行する
     * @param[in] idm IDm
     * @param[out] txBuf 送信するデータを格納する 配列は FELICA_TX_BUF_SIZE 以上の長さである必要がある。1バイト目がレスポンスコード
     * @param[out] txBufLen 送信するデータの長さを格納する。
     */
    void listen_Request_System_Code(const uint8_t (&idm)[FELICA_IDM_SIZE],uint8_t *txBuf,uint16_t *txBufLen);
};

#endif /*FELICA_HPP*/