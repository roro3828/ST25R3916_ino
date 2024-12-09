#include"felica.hpp"


/**********************************************************************************************
 * Felica
 **********************************************************************************************/

Felica::Felica(const uint8_t (&pmm)[FELICA_PMM_SIZE]){
    memcpy(this->pmm,pmm,FELICA_PMM_SIZE);
    for(uint16_t i=0;i<FELICA_MAX_SYSTEM_COUNT;i++){
        systems[i]=NULL;
    }
}
Felica::~Felica()noexcept{
    for(uint16_t i=0;i<FELICA_MAX_SYSTEM_COUNT;i++){
        if(systems[i]!=NULL){
            delete systems[i];
        }
    }
}

bool Felica::switch_system(const uint8_t (&idm)[FELICA_IDM_SIZE]){
    //現在のシステムとIDmが異なる場合切り替える
    if(!compare_idm(this->current_system->idm,idm)){
        if(this->get_system_by_idm(idm,&this->current_system)!=FelicaReturnCode::SUCCESS){
            return false;
        }
        this->current_mode=0x00;
    }
    return true;
}

FelicaReturnCode Felica::get_system(const systemcode_t &system_code,FelicaSystem **system)const{
    if(system!=NULL){
        *system=NULL;
    }
    for(uint16_t i=0;i<FELICA_MAX_SYSTEM_COUNT;i++){
        if(this->systems[i]==NULL){
            continue;
        }
        const uint16_t &c_system_code=this->systems[i]->system_code;
        if((c_system_code==system_code)||
            (system_code==0xFFFF)||                                                             //0xFFはワイルドカード
            (((system_code&0xFF00)==0xFF00)&&((system_code&0x00FF)==(c_system_code&0x00FF)))||
            (((system_code&0x00FF)==0x00FF)&&((system_code&0xFF00)==(c_system_code&0xFF00)))
            ){
            if(system!=NULL){
                *system=this->systems[i];
            }
            return FelicaReturnCode::SUCCESS;
        }
    }

    return FelicaReturnCode::ERROR;
}

FelicaReturnCode Felica::get_system_by_idm(const uint8_t (&idm)[FELICA_IDM_SIZE],FelicaSystem **system)const{
    if(system!=NULL){
        *system=NULL;
    }
    for(uint16_t i=0;i<FELICA_MAX_SYSTEM_COUNT;i++){
        if(this->systems[i]==NULL){
            continue;
        }
        if(compare_idm(this->systems[i]->idm,idm)){
            if(system!=NULL){
                *system=this->systems[i];
            }
            return FelicaReturnCode::SUCCESS;
        }
    }

    return FelicaReturnCode::ERROR;
}

FelicaReturnCode Felica::add_system(const systemcode_t &system_code,const uint8_t (&idm)[FELICA_IDM_SIZE]){
    if(this->get_system(system_code,NULL)==FelicaReturnCode::SUCCESS){
        return FelicaReturnCode::ERROR;
    }
    if(this->get_system_by_idm(idm,NULL)==FelicaReturnCode::SUCCESS){
        return FelicaReturnCode::ERROR;
    }
    for(uint16_t i=0;i<FELICA_MAX_SYSTEM_COUNT;i++){
        if(this->systems[i]==NULL){
            this->systems[i]=new FelicaSystem(system_code,idm);
            return FelicaReturnCode::SUCCESS;
        }
    }

    return FelicaReturnCode::NO_SPACE;
}

void Felica::listen(const uint8_t *rxBuf,const uint16_t rxBufLen,uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;
    const uint8_t felica_cmd=rxBuf[0];

    switch(felica_cmd){
        case FELICA_POLLING_CMD_CODE:{
            //受信したデータ長が正しくなかった場合終了
            if((FELICA_CMD_SIZE+FELICA_SYSTEM_CODE_SIZE+FELICA_POLLING_REQUEST_CODE_SIZE+FELICA_POLLING_TIME_SLOT_SIZE)!=rxBufLen){
                return;
            }
            //システムコードはビッグエンディアン
            const systemcode_t system_code=(((systemcode_t)rxBuf[FELICA_CMD_SIZE])<<8)+(systemcode_t)rxBuf[FELICA_CMD_SIZE+1];

            const uint8_t request_code=rxBuf[FELICA_CMD_SIZE+FELICA_SYSTEM_CODE_SIZE];
            const uint8_t timeslot=rxBuf[FELICA_CMD_SIZE+FELICA_SYSTEM_CODE_SIZE+FELICA_POLLING_REQUEST_CODE_SIZE];
            listen_Polling(system_code,request_code,timeslot,txBuf,txBufLen);
        }
        break;
        case FELICA_REQUEST_SERVICE_CMD_CODE:{
            const uint8_t node_count=rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE];
            //受信したデータ長が正しくなかった場合終了
            if((FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*node_count)!=rxBufLen){
                return;
            }
            uint8_t idm[FELICA_IDM_SIZE];
            memcpy(idm,&rxBuf[FELICA_CMD_SIZE],FELICA_IDM_SIZE);
            listen_Request_Service(idm,node_count,&rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1],txBuf,txBufLen);
        }
        break;
        case FELICA_REQUEST_RESPONSE_CMD_CODE:{
            if((FELICA_CMD_SIZE+FELICA_IDM_SIZE+1)!=rxBufLen){
                return;
            }
            uint8_t idm[FELICA_IDM_SIZE];
            memcpy(idm,&rxBuf[FELICA_CMD_SIZE],FELICA_IDM_SIZE);
            listen_Request_Response(idm,txBuf,txBufLen);
        }
        break;
        case FELICA_READ_WITHOUT_ENCRYPTION_CMD_CODE:{
            //サイズをチェック
            const uint8_t service_count=rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE];
            const uint8_t block_count=rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*service_count];
            if(!(((FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*service_count+2*block_count)<=rxBufLen)&&
                (rxBufLen<=(FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*service_count+3*block_count)))){
                return;
            }
            uint8_t idm[FELICA_IDM_SIZE];
            memcpy(idm,&rxBuf[FELICA_CMD_SIZE],FELICA_IDM_SIZE);

            const BlockListElement **block_list=new const BlockListElement*[block_count];
            uint16_t idx=0;
            for(uint8_t i=0;i<block_count;i++){
                block_list[i]=new BlockListElement(&rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*service_count+idx]);
                if(block_list[i]->len==1){
                    idx+=FELICA_BLOCK_LIST_ELEMENT_MIN_SIZE;
                }
                else{
                    idx+=FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE;
                }
            }

            listen_Read_Without_Encryption(idm,service_count,&rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1],block_count,block_list,txBuf,txBufLen);
            //メモリ解放
            for(uint8_t i=0;i<block_count;i++){
                delete block_list[i];
            }
            delete[] block_list;
        }
        break;
        case FELICA_REQUEST_SYSTEM_CODE_CMD_CODE:{
            //サイズをチェック
            if((FELICA_CMD_SIZE+FELICA_IDM_SIZE)!=rxBufLen){
                return;
            }
            uint8_t idm[FELICA_IDM_SIZE];
            memcpy(idm,&rxBuf[FELICA_CMD_SIZE],FELICA_IDM_SIZE);
            listen_Request_System_Code(idm,txBuf,txBufLen);
        }
        break;
        default:
        break;
    }

}

void Felica::listen_Polling(const systemcode_t &system_code,const uint8_t &request_code,const uint8_t &time_slot,uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;
    if(this->get_system(system_code,&this->current_system)!=FelicaReturnCode::SUCCESS){
        return;
    }

    //モードを0にリセット
    this->current_mode=0x00;
    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_POLLING_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->current_system->idm,FELICA_IDM_SIZE);
    memcpy(&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE],this->pmm,FELICA_PMM_SIZE);
    //txBufの長さを一時的に確定させる
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE;

    switch(request_code){
        case FELICA_POLLING_REQUEST_CODE_SYSTEMCODE:{
            *txBufLen+=FELICA_SYSTEM_CODE_SIZE;
            //システムコードはビックエンディアン
            txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE]=((this->current_system->system_code>>8)&0xFF);
            txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE+1]=(this->current_system->system_code&0xFF);
        }
        break;
        case FELICA_POLLING_REQUEST_CODE_NONE:
        default:
        break;
    }
}

void Felica::listen_Request_Service(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &node_count,const uint8_t *node_code_list,uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;

    //現在のシステムとidmが違った場合システムを切り替え
    if(!this->switch_system(idm)){
        return;
    }
    
    //ノード数が1以上32以下じゃないとき
    if(!(1<=node_count&&node_count<=32)){
        return;
    }
    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_REQUEST_SERVICE_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->current_system->idm,FELICA_IDM_SIZE);
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=node_count;
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*node_count;

    for(uint8_t i=0;i<node_count;i++){

        //与えられたノードコードがサービスコードがエリアコードか判定
        if(((node_code_list[2*i]&FELICA_AREA_ATTR_MASK)==FELICA_AREA_ATTR_CAN_CREATE_SUB)||((node_code_list[2*i]&FELICA_AREA_ATTR_MASK)==FELICA_AREA_ATTR_CANNOT_CREATE_SUB)){
            //エリアコード
            const areacode_t area_code=((areacode_t)node_code_list[2*i])+(((areacode_t)node_code_list[2*i+1])<<8);
            FelicaArea const *area;
            if(this->current_system->get_area(area_code,&area)==FelicaReturnCode::SUCCESS){
                txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*i]=area->area_key_ver&0xFF;
                txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*i]=(area->area_key_ver>>8)&0xFF;
            }
            else{
                //エリアが見つからなかった場合0xFFFF
                txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*i]=0xFF;
                txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*i]=0xFF;
            }
        }
        else{
            //サービスコード
            //リトルエンディアン
            const servicecode_t service_code=((servicecode_t)node_code_list[2*i])+(((servicecode_t)node_code_list[2*i+1])<<8);

            FelicaService *service;
            if(this->current_system->get_service(service_code,&service)==FelicaReturnCode::SUCCESS){
                txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*i]=service->service_key_ver&0xFF;
                txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*i]=(service->service_key_ver>>8)&0xFF;
            }
            else{
                //サービスが見つからなかった場合0xFF
                txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*i]=0xFF;
                txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*i]=0xFF;
            }
        }
    }
}

void Felica::listen_Request_Response(const uint8_t (&idm)[FELICA_IDM_SIZE],uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;

    //現在のシステムとidmが違った場合システムを切り替え
    if(!this->switch_system(idm)){
        return;
    }

    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+1;
    
    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_REQUEST_RESPONSE_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->current_system->idm,FELICA_IDM_SIZE);
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=this->current_mode;
}

void Felica::listen_Read_Without_Encryption(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &service_count,const uint8_t *service_code_list,const uint8_t &block_count,const BlockListElement **block_list,uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;
    //現在のシステムとidmが違った場合システムを切り替え
    if(!this->switch_system(idm)){
        return;
    }

    //service_countは1以上16以下
    if(!(1<=service_count&&service_count<=16)){
        return;
    }
    if(!(1<=block_count&&block_count<=FELICA_READ_WITHOUT_ENCRYPTION_BUF_SIZE)){
        return;
    }

    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_READ_WITHOUT_ENCRYPTION_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->current_system->idm,FELICA_IDM_SIZE);
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=0x00;
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1]=0x00;
    //長さを一時的に確定
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+2;

    uint16_t idx=0;
    uint8_t block_data[FELICA_READ_WITHOUT_ENCRYPTION_BUF_SIZE][FELICA_BLOCK_SIZE];

    for(uint8_t i=0;i<block_count;i++){
        //アクセスモードが0以外の時終了
        if(block_list[i]->access_mode!=0x000){
            txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=i+1;
            return;
        }

        //サービスコードはリトルエンディアン
        const servicecode_t service_code=((servicecode_t)service_code_list[2*block_list[i]->service_code_list_order])+(((servicecode_t)service_code_list[2*block_list[i]->service_code_list_order+1])<<8);

        FelicaService *service;
        if(this->current_system->get_service(service_code,&service)!=FelicaReturnCode::SUCCESS){
            txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=i+1;
            return;
        }
        
        //ブロックの読み込みに失敗した場合終了
        if(service->read_block(block_list[i]->block_num,block_data[i])!=FelicaReturnCode::SUCCESS){
            txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=i+1;
            return;
        }
    }
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2]=block_count;
    memcpy(&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+3],block_data,FELICA_BLOCK_SIZE*block_count);
    *txBufLen+=1+FELICA_BLOCK_SIZE*block_count;
}

void Felica::listen_Request_System_Code(const uint8_t (&idm)[FELICA_IDM_SIZE],uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;
    //現在のシステムとidmが違った場合システムを切り替え
    if(!this->switch_system(idm)){
        return;
    }

    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_REQUEST_SYSTEM_CODE_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->current_system->idm,FELICA_IDM_SIZE);
    uint8_t system_count=0;
    for(uint8_t i=0;i<FELICA_MAX_SYSTEM_COUNT;i++){
        if(this->systems[i]==NULL){
            continue;
        }
        txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*system_count]=(this->systems[i]->system_code>>8)&0xFF;
        txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*system_count]=(this->systems[i]->system_code&0xFF);
        system_count++;
    }
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=system_count;
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*system_count;
}

/**********************************************************************************************
 * Felica System
 **********************************************************************************************/
FelicaSystem::FelicaSystem(const uint16_t &system_code,const uint8_t (&idm)[FELICA_IDM_SIZE]):
    system_code(system_code),
    area0(FELICA_AREA0_AREA_CODE,FELICA_AREA0_END_SERVICE_CODE){
    memcpy(this->idm,idm,FELICA_IDM_SIZE);
}
FelicaSystem::~FelicaSystem()noexcept{}
FelicaReturnCode FelicaSystem::get_area(const areacode_t &areacode,FelicaArea const **area)const{
    //エリア0とエリアコードが等しかった場合
    if(areacode==this->area0.areacode){
        if(area!=NULL){
            *area=&this->area0;
        }
        return FelicaReturnCode::SUCCESS;
    }
    return this->area0.get_area(areacode,area);
}
FelicaReturnCode FelicaSystem::add_area(const areacode_t &areacode,const servicecode_t &end_servicecode){
    return this->area0.add_area(areacode,end_servicecode);
}
FelicaReturnCode FelicaSystem::get_service(const servicecode_t &service_code,FelicaService **service)const{
    return this->area0.get_service(service_code,service);
}
FelicaReturnCode FelicaSystem::add_service(const servicecode_t &service_code,const uint16_t &block_count){
    return this->area0.add_service(service_code,block_count);
}
/**********************************************************************************************
 * Felica Area
 **********************************************************************************************/
FelicaArea::FelicaArea(const areacode_t &areacode,const servicecode_t &end_servicecode):
    areacode(areacode),
    end_servicecode(end_servicecode){
        //エリア配列とサービス配列を初期化
        for(uint16_t i=0;i<FELICA_MAX_AREA_COUNT;i++){
            this->areas[i]=NULL;
        }
        for(uint16_t i=0;i<FELICA_MAX_SERVICE_COUNT;i++){
            this->services[i]=NULL;
        }
}
FelicaArea::~FelicaArea()noexcept{
    for(uint16_t i=0;i<FELICA_MAX_SERVICE_COUNT;i++){
        if(this->services[i]!=NULL){
            delete this->services[i];
        }
    }
    for(uint16_t i=0;i<FELICA_MAX_AREA_COUNT;i++){
        if(this->areas[i]!=NULL){
            delete this->areas[i];
        }
    }
}

FelicaReturnCode FelicaArea::get_area(const areacode_t &areacode,FelicaArea const **area)const{
    if(area!=NULL){
        *area=NULL;
    }
    //自分と同じ場合、自分を返す
    if(this->areacode==areacode){
        if(area!=NULL){
            *area=this;
        }
        return FelicaReturnCode::SUCCESS;
    }

    //サブエリアを作成不可な場合エラーを返す
    if((this->areacode&FELICA_AREA_ATTR_MASK)==FELICA_AREA_ATTR_CANNOT_CREATE_SUB){
        return FelicaReturnCode::ERROR;
    }
    for(uint16_t i=0;i<FELICA_MAX_AREA_COUNT;i++){
        if(this->areas[i]==NULL){
            continue;
        }
        if(this->areas[i]->get_area(areacode,area)==FelicaReturnCode::SUCCESS){
            return FelicaReturnCode::SUCCESS;
        }
    }
    return FelicaReturnCode::ERROR;
}
FelicaReturnCode FelicaArea::add_area(const areacode_t &areacode,const servicecode_t &end_servicecode){
    //エリアコードが親エリア(このインスタンス)のコード以上、エンドコード以下でないときエラーを返す
    if(!((this->areacode<=areacode)&&(areacode<=this->end_servicecode))){
        return FelicaReturnCode::OUT_OF_RANGE;
    }

    //サブエリアを作成不可な場合エラーを返す
    if((this->areacode&FELICA_AREA_ATTR_MASK)==FELICA_AREA_ATTR_CANNOT_CREATE_SUB){
        return FelicaReturnCode::ERROR;
    }

    //エリアコードが存在する場合エラーを返す
    if(this->get_area(areacode,NULL)==FelicaReturnCode::SUCCESS){
        return FelicaReturnCode::ERROR;
    }
    
    //下位のエリアに登録する
    for(uint16_t i=0;i<FELICA_MAX_AREA_COUNT;i++){
        if(this->areas[i]==NULL){
            continue;
        }
        const FelicaReturnCode returncode=this->areas[i]->add_area(areacode,end_servicecode);
        //エリアコードが範囲外だった時次へ進む
        if(returncode!=FelicaReturnCode::OUT_OF_RANGE){
            return returncode;
        }
    }

    //配列を探索して空いている個所に入れる
    for(uint16_t i=0;i<FELICA_MAX_AREA_COUNT;i++){
        if(this->areas[i]!=NULL){
            continue;
        }
        this->areas[i]=new FelicaArea(areacode,end_servicecode);
        return FelicaReturnCode::SUCCESS;
    }
    return FelicaReturnCode::NO_SPACE;
}
FelicaReturnCode FelicaArea::get_service(const servicecode_t &service_code,FelicaService **service)const{
    if(service!=NULL){
        *service=NULL;
    }
    //自身のサービス配列から探す
    for(uint16_t i=0;i<FELICA_MAX_SERVICE_COUNT;i++){
        if(this->services[i]==NULL){
            continue;
        }


        if(this->services[i]->service_code!=service_code){
            continue;
        }
        if(service!=NULL){
            *service=this->services[i];
        }
        return FelicaReturnCode::SUCCESS;
    }

    //下位エリアからも探す
    for(uint16_t i=0;i<FELICA_MAX_AREA_COUNT;i++){
        if(this->areas[i]==NULL){
            continue;
        }

        if(this->areas[i]->get_service(service_code,service)==FelicaReturnCode::SUCCESS){
            return FelicaReturnCode::SUCCESS;
        }
    }

    return FelicaReturnCode::ERROR;
}
FelicaReturnCode FelicaArea::add_service(const servicecode_t &service_code,const uint16_t &block_count){
    //サービスコードが親エリア(このインスタンス)のコード以上、エンドコード以下でないときエラーを返す
    if(!((this->areacode<=service_code)&&(service_code<=this->end_servicecode))){
        return FelicaReturnCode::OUT_OF_RANGE;
    }

    //サービスコードが存在する場合エラーを返す
    if(this->get_service(service_code,NULL)==FelicaReturnCode::SUCCESS){
        return FelicaReturnCode::ERROR;
    }
    
    if((this->areacode&FELICA_AREA_ATTR_MASK)==FELICA_AREA_ATTR_CAN_CREATE_SUB){
        //下位のエリアに登録する
        for(uint16_t i=0;i<FELICA_MAX_AREA_COUNT;i++){
            if(this->areas[i]==NULL){
                continue;
            }
            FelicaReturnCode returncode=this->areas[i]->add_service(service_code,block_count);
            //エリアコードが範囲外だった時次へ進む
            if(returncode!=FelicaReturnCode::OUT_OF_RANGE){
                return returncode;
            }
        }
    }


    //配列を探索して空いている個所に入れる
    for(uint16_t i=0;i<FELICA_MAX_SERVICE_COUNT;i++){
        if(this->services[i]!=NULL){
            continue;
        }
        this->services[i]=new FelicaService(service_code,block_count);
        return FelicaReturnCode::SUCCESS;
    }
    return FelicaReturnCode::NO_SPACE;
}
/**********************************************************************************************
 * Felica Service
 **********************************************************************************************/
FelicaService::FelicaService(const servicecode_t &service_code,const uint16_t &block_count):
    service_code(service_code),
    original(true),
    blocks(new FelicaBlocks(block_count)){}
FelicaService::FelicaService(const servicecode_t &service_code,FelicaBlocks *blocks):
    service_code(service_code),
    original(false),
    blocks(blocks){}
FelicaService::~FelicaService()noexcept{
    if(this->original){
        delete this->blocks;
    }
}

FelicaReturnCode FelicaService::read_block_force(const uint16_t &block_num,uint8_t *data){
    if(this->blocks->block_count<=block_num){
        return FelicaReturnCode::ERROR;
    }

    switch(this->service_code&FELICA_SERVICE_ATTR_TYPE_MASK){
        case FELICA_SERVICE_ATTR_TYPE_RANDOM:{
            memcpy(data,this->blocks->blocks[block_num],FELICA_BLOCK_SIZE);
        }
        break;
        case FELICA_SERVICE_ATTR_TYPE_CYCLIC:{
            //cyclic_indexだけずらした位置から読み取る
            memcpy(data,this->blocks->blocks[(this->blocks->cyclic_index+block_num)%this->blocks->block_count],FELICA_BLOCK_SIZE);
        }
        break;
        default:
            return FelicaReturnCode::ERROR;
    }

    return FelicaReturnCode::SUCCESS;

}

FelicaReturnCode FelicaService::read_block(const uint16_t &block_num,uint8_t *data){
    switch(this->service_code&FELICA_AREA_ATTR_MASK){
        case (FELICA_SERVICE_ATTR_AUTH_NO_NEED|FELICA_SERVICE_ATTR_ACCESS_RW)|FELICA_SERVICE_ATTR_TYPE_RANDOM:
        case (FELICA_SERVICE_ATTR_AUTH_NO_NEED|FELICA_SERVICE_ATTR_ACCESS_RW)|FELICA_SERVICE_ATTR_TYPE_CYCLIC:
        case (FELICA_SERVICE_ATTR_AUTH_NO_NEED|FELICA_SERVICE_ATTR_ACCESS_R)|FELICA_SERVICE_ATTR_TYPE_RANDOM:
        case (FELICA_SERVICE_ATTR_AUTH_NO_NEED|FELICA_SERVICE_ATTR_ACCESS_R)|FELICA_SERVICE_ATTR_TYPE_CYCLIC:
            return this->read_block_force(block_num,data);
        break;
        default:
            return FelicaReturnCode::ERROR;
    }
}
FelicaReturnCode FelicaService::write_block_force(const uint16_t &block_num,const uint8_t *data){
    if(this->blocks->block_count<=block_num){
        return FelicaReturnCode::ERROR;
    }

    switch(this->service_code&FELICA_SERVICE_ATTR_TYPE_MASK){
        case FELICA_SERVICE_ATTR_TYPE_RANDOM:{
            memcpy(this->blocks->blocks[block_num],data,FELICA_BLOCK_SIZE);
        }
        break;
        case FELICA_SERVICE_ATTR_TYPE_CYCLIC:{
            const uint16_t next_index=(this->blocks->cyclic_index+this->blocks->block_count-1)%this->blocks->block_count;
            //値が等しい場合更新しない
            if(memcmp(this->blocks->blocks[this->blocks->cyclic_index],data,FELICA_BLOCK_SIZE)==0){
                break;
            }
            memcpy(this->blocks->blocks[next_index],data,FELICA_BLOCK_SIZE);
            this->blocks->cyclic_index=next_index;
        }
        break;
        default:
            return FelicaReturnCode::ERROR;
    }

    return FelicaReturnCode::SUCCESS;
}
FelicaReturnCode FelicaService::write_block(const uint16_t &block_num,const uint8_t *data){
    switch(this->service_code&FELICA_AREA_ATTR_MASK){
        case (FELICA_SERVICE_ATTR_AUTH_NO_NEED|FELICA_SERVICE_ATTR_ACCESS_RW)|FELICA_SERVICE_ATTR_TYPE_RANDOM:
        case (FELICA_SERVICE_ATTR_AUTH_NO_NEED|FELICA_SERVICE_ATTR_ACCESS_RW)|FELICA_SERVICE_ATTR_TYPE_CYCLIC:
            return this->write_block_force(block_num,data);
        break;
        default:
            return FelicaReturnCode::ERROR;
    }
}


/**********************************************************************************************
 * Blocks
 **********************************************************************************************/
FelicaBlocks::FelicaBlocks(const uint16_t &block_count):
    block_count(block_count){

    cyclic_index=0;
    this->blocks=new uint8_t*[this->block_count];
    for(uint16_t i=0;i<this->block_count;i++){
        this->blocks[i]=new uint8_t[FELICA_BLOCK_SIZE];
        memset(this->blocks[i],0U,FELICA_BLOCK_SIZE);
    }
}
FelicaBlocks::~FelicaBlocks()noexcept{
    for(uint16_t i=0;i<this->block_count;i++){
        delete[] this->blocks[i];
    }
    delete[] this->blocks;
}

/**********************************************************************************************
 * Block list element
 **********************************************************************************************/
BlockListElement::BlockListElement(const uint8_t &access_mode,const uint8_t &service_code_list_order,const uint16_t &block_num):
    len((block_num<=0xFF)?1U:0U),                           //1ビット
    access_mode(access_mode&0b111),                         //3ビット
    service_code_list_order(service_code_list_order&0b1111),//4ビット
    block_num(block_num){}
BlockListElement::BlockListElement(const uint8_t *buf):
    BlockListElement(
        (buf[0]>>FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT),
        buf[0],
        ((buf[0]>>(FELICA_BLOCK_LIST_ELEMENT_ACCESS_MODE_BIT+FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT))==1U)?(buf[1]):(((uint16_t)buf[1])+(((uint16_t)buf[2])<<8))){}
uint8_t BlockListElement::set_element_to_buf(uint8_t *buf)const{
    buf[0]=(this->len<<(FELICA_BLOCK_LIST_ELEMENT_ACCESS_MODE_BIT+FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT))+(this->access_mode<<FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT)+this->service_code_list_order;
    //リトルエンディアン
    buf[1]=(this->block_num&0xFF);
    if(this->len==0){
        buf[2]=(this->block_num>>8)&0xFF;
        return FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE;
    }
    return FELICA_BLOCK_LIST_ELEMENT_MIN_SIZE;
}




bool compare_idm(const uint8_t (&idm1)[FELICA_IDM_SIZE],const uint8_t (&idm2)[FELICA_IDM_SIZE]){
    bool flag=true;
    for(uint8_t i=0;i<FELICA_IDM_SIZE;i++){
        flag&=(idm1[i]==idm2[i]);
    }
    return flag;
}