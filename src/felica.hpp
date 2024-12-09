#ifndef FELICA_HPP
#define FELICA_HPP

#include<stdio.h>
#include<cstring>
#include<cstdint>


/**
 * FelicaのIDmのサイズ
 */
#define FELICA_IDM_SIZE 8U
/**
 * FelicaのPMmのサイズ
 */
#define FELICA_PMM_SIZE 8U
/**
 * Felicaのシステムコードのサイズ
 */
#define FELICA_SYSTEM_CODE_SIZE 2U
using systemcode_t=uint16_t;
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
#define felica_Area_Code_to_Num(c)              ((c&FELICA_AREA_NUM_MASK)>>FELICA_AREA_ATTR_LEN_BIT)
#define felica_Area_Code_to_Attr(c)             (c&FELICA_AREA_ATTR_MASK)
#define FELICA_AREA_ATTR_CAN_CREATE_SUB         0b000000
#define FELICA_AREA_ATTR_CANNOT_CREATE_SUB      0b000001
#define FELICA_AREA0_AREA_CODE                  0x0000
#define FELICA_AREA0_END_SERVICE_CODE           0xFFFE

/**
 * サービス用定数
 */
using servicecode_t=uint16_t;
#define FELICA_SERVICE_CODE_SIZE                2U
#define FELICA_SERVICE_CODE_NUM_MASK            0b1111111111000000
#define FELICA_SERVICE_CODE_ATTR_MASK           0b0000000000111111
#define FELICA_SERVICE_ATTR_LEN_BIT             6U
#define felica_Service_Code_to_Num(c)           ((c&FELICA_SERVICE_CODE_NUM_MASK)>>FELICA_SERVICE_ATTR_LEN_BIT)
#define felica_Service_Code_to_Attr(c)          (c&FELICA_SERVICE_CODE_ATTR_MASK)
#define FELICA_SERVICE_ATTR_AUTH_MASK           0b000001
#define FELICA_SERVICE_ATTR_AUTH_NEED           0b000000
#define FELICA_SERVICE_ATTR_AUTH_NO_NEED        0b000001
#define felica_Service_is_Auth_Required(c)      ((c&FELICA_SERVICE_ATTR_AUTH_MASK)==FELICA_SERVICE_ATTR_AUTH_NEED)
#define FELICA_SERVICE_ATTR_ACCESS_MASK         0b010010
#define FELICA_SERVICE_ATTR_ACCESS_RW           0b000000
#define FELICA_SERVICE_ATTR_ACCESS_R            0b000010
#define FELICA_SERVICE_ATTR_TYPE_MASK           0b001100
#define FELICA_SERVICE_ATTR_TYPE_RANDOM         0b001000
#define FELICA_SERVICE_ATTR_TYPE_CYCLIC         0b001100

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
enum FelicaReturnCode{
    SUCCESS,
    ERROR,
    NO_SPACE,
    OUT_OF_RANGE
};

class FelicaBlocks{
    public:
    uint16_t cyclic_index;
    uint8_t **blocks;
    const uint16_t block_count;

    FelicaBlocks(const uint16_t &block_count);
    ~FelicaBlocks()noexcept;
};

class FelicaService{
    const bool original;
    public:
    FelicaBlocks * const blocks;
    const servicecode_t service_code;

    //この辺りは情報がないので詳細は不明 とりあえずてきとうな値を設定
    const uint8_t service_key=0xFF;
    //鍵バージョンは鍵の識別用IDっぽいけど鍵とか使えないのでてきとう
    uint16_t service_key_ver=0xFFFF;

    FelicaService(const servicecode_t &service_code,const uint16_t &block_count);
    FelicaService(const servicecode_t &service_code,FelicaBlocks *blocks);
    ~FelicaService()noexcept;

    /**
     * @brief 指定した番号のブロックを読む
     * @param[in] block_num ブロック番号
     * @param[out] data ブロックから読みだされた16バイトのデータ
     * @return SUCCESS:正常に読めた場合
     * @return ERROR:ブロック番号がブロック数を超えていた場合
     */
    FelicaReturnCode read_block(const uint16_t &block_num,uint8_t *data);
    /**
     * @brief 指定した番号のブロックにデータを書き込む
     * @param[in] block_num ブロック番号
     * @param[out] data ブロックに書き込む16バイトのデータ
     * @return SUCCESS:正常に読めた場合
     * @return ERROR:ブロック番号がブロック数を超えていた場合
     */
    FelicaReturnCode write_block(const uint16_t &block_num,const uint8_t *data);

    /**
     * @brief アクセス制限を無視して指定した番号のブロックを読む
     * @param[in] block_num ブロック番号
     * @param[out] data ブロックから読みだされた16バイトのデータ
     * @return SUCCESS:正常に読めた場合
     * @return ERROR:ブロック番号がブロック数を超えていた場合
     */
    FelicaReturnCode read_block_force(const uint16_t &block_num,uint8_t *data);
    /**
     * @brief アクセス制限を無視して指定した番号のブロックにデータを書き込む
     * @param[in] block_num ブロック番号
     * @param[out] data ブロックに書き込む16バイトのデータ
     * @return SUCCESS:正常に読めた場合
     * @return ERROR:ブロック番号がブロック数を超えていた場合
     */
    FelicaReturnCode write_block_force(const uint16_t &block_num,const uint8_t *data);
};

class FelicaArea{
    FelicaService *services[FELICA_MAX_SERVICE_COUNT];
    FelicaArea *areas[FELICA_MAX_AREA_COUNT];
    public:
    const areacode_t areacode;
    const servicecode_t end_servicecode;
    //この辺りは情報がないので詳細は不明 とりあえずてきとうな値を設定
    const uint8_t area_key=0xFF;
    //鍵バージョンは鍵の識別用IDっぽいけど鍵とか使えないのでてきとう
    uint16_t area_key_ver=0xFFFF;

    FelicaArea(const areacode_t &areacode,const servicecode_t &end_servicecode);
    ~FelicaArea()noexcept;

    /**
     * @brief エリアコードからエリアを取得する
     * @param[in] areacode
     * @param[out] area エリアポインタのアドレス エリアのアドレスが格納される 見つからなかった場合NULLが格納される NULLでも可
     * @return SUCCESS:エリアが見つかった場合
     * @return ERROR:エリアが見つからなかった場合
     */
    FelicaReturnCode get_area(const areacode_t &areacode,FelicaArea const **area)const;
    /**
     * @brief エリアを追加する
     * @param[in] areacode
     * @param[in] end_servicecode
     * @return SUCCESS:エリアを追加できた場合
     * @return ERROR:エリアコードがすでに存在していた場合や作成不可なエリアコードの場合
     * @return NO_SPACE:領域が足りない場合
     * @return OUT_OF_RANGE:エリアコードが登録可能な範囲外だった場合
     */
    FelicaReturnCode add_area(const areacode_t &areacode,const servicecode_t &end_servicecode);
    /**
     * @brief サービスコードからサービスを取得する
     * @param[in] service_code
     * @param[out] service サービスポインタのアドレス サービスのアドレスが格納される 見つからなかった場合NULLが格納される NULLでも可
     * @return SUCCESS:サービスが見つかった場合
     * @return ERROR:サービスが見つからなかった場合
     */
    FelicaReturnCode get_service(const servicecode_t &service_code,FelicaService **service)const;

    /**
     * @brief エリアにサービスを追加する
     * @param[in] service_code 追加するサービスコード
     * @return SUCCESS:サービスを追加できた場合
     * @return ERROR:サービスコードがすでに存在していた場合
     * @return NO_SPACE:領域が足りない場合
     * @return OUT_OF_RANGE:サービスコードが登録可能な範囲外だった場合
     */
    FelicaReturnCode add_service(const servicecode_t &service_code,const uint16_t &block_count);
};

class FelicaSystem{
    FelicaArea area0;
    public:
    const systemcode_t system_code;
    uint8_t idm[FELICA_IDM_SIZE];

    FelicaSystem(const uint16_t &system_code,const uint8_t (&idm)[FELICA_IDM_SIZE]);
    ~FelicaSystem()noexcept;

    /**
     * @brief エリアコードからエリアを取得する
     * @param[in] areacode
     * @param[out] area エリアポインタのアドレス エリアのアドレスが格納される 見つからなかった場合NULLが格納される NULLでも可
     * @return SUCCESS:エリアが見つかった場合
     * @return ERROR:エリアが見つからなかった場合
     */
    FelicaReturnCode get_area(const areacode_t &areacode,FelicaArea const **area)const;
    /**
     * @brief エリアを追加する
     * @param[in] areacode
     * @param[in] end_servicecode
     * @return SUCCESS:エリアを追加できた場合
     * @return ERROR:エリアコードがすでに存在していた場合や作成不可なエリアコードの場合
     * @return NO_SPACE:領域が足りない場合
     */
    FelicaReturnCode add_area(const areacode_t &areacode,const servicecode_t &end_servicecode);
    /**
     * @brief サービスコードからサービスを取得する
     * @param[in] service_code
     * @param[out] service サービスポインタのアドレス サービスのアドレスが格納される 見つからなかった場合NULLが格納される NULLでも可
     * @return SUCCESS:サービスが見つかった場合
     * @return ERROR:サービスが見つからなかった場合
     */
    FelicaReturnCode get_service(const servicecode_t &service_code,FelicaService **service)const;

    /**
     * @brief システムにサービスを追加する
     * @param[in] service_code 追加するサービスコード
     * @return SUCCESS:サービスを追加できた場合
     * @return ERROR:サービスコードがすでに存在していた場合
     * @return NO_SPACE:領域が足りない場合
     */
    FelicaReturnCode add_service(const servicecode_t &service_code,const uint16_t &block_count);
};

class Felica{
    /**
     * システムのポインタ配列
     */
    FelicaSystem *systems[FELICA_MAX_SYSTEM_COUNT];
    uint8_t current_mode=0x00;
    FelicaSystem *current_system=NULL;

    /**
     * @brief IDmを持つシステムに切り替える 現在のシステムのIDmの場合切り替えない
     * @param[in] idm
     * @return true:切り替えが成功した場合
     * @return false:IDmを持つシステムが存在しなかった場合
     */
    bool switch_system(const uint8_t (&idm)[FELICA_IDM_SIZE]);

    public:
    /**
     * PMm
     */
    uint8_t pmm[FELICA_PMM_SIZE];


    Felica(const uint8_t (&pmm)[FELICA_PMM_SIZE]);
    ~Felica() noexcept;
    /**
     * @brief システムコードからシステムを取得する
     * @param[in] system_code
     * @param[out] system システムポインタのアドレス システムのアドレスが返される 見つからなかった場合NULLが格納される NULLでも可
     * @return SUCCESS:システムが見つかった場合
     * @return ERROR:システムが見つからなかった場合
     */
    FelicaReturnCode get_system(const systemcode_t &system_code,FelicaSystem **system)const;
    /**
     * @brief IDmからシステムを取得する
     * @param[in] idm
     * @param[out] system システムポインタのアドレス システムのアドレスが返される 見つからなかった場合NULLが格納される NULLでも可
     * @return SUCCESS:システムが見つかった場合
     * @return ERROR:システムが見つからなかった場合
     */
    FelicaReturnCode get_system_by_idm(const uint8_t (&idm)[FELICA_IDM_SIZE],FelicaSystem **system)const;
    /**
     * @brief Felicaにシステムを追加する
     * @param[in] system_code 追加するシステムコード
     * @param[in] idm 追加するIDm
     * @return SUCCESS:システムを追加できた場合
     * @return ERROR:システムコードかIDmがすでに存在していた場合
     * @return NO_SPACE:領域が足りない場合
     */
    FelicaReturnCode add_system(const systemcode_t &system_code,const uint8_t (&idm)[FELICA_IDM_SIZE]);

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



bool compare_idm(const uint8_t (&idm1)[FELICA_IDM_SIZE],const uint8_t (&idm2)[FELICA_IDM_SIZE]);

#endif /*FELICA_HPP*/