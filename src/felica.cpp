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

#include"felica.hpp"

/**********************************************************************************************
 * Felica
 **********************************************************************************************/

Felica::Felica(const uint8_t (&pmm)[FELICA_PMM_SIZE],const systemcode_t &systemcode,const uint8_t (&idm)[FELICA_IDM_SIZE]){
    //ブロックをすべて0で初期化
    for(blockcount_t i=0;i<FELICA_BLOCK_COUNT;i++){
        memset(&this->block[i],0U,FELICA_BLOCK_SIZE);
    }

    this->initialized=FELICA_NOT_INITIALIZED;
    memcpy(this->pmm,pmm,FELICA_PMM_SIZE);
    FelicaSystem *const system=(FelicaSystem*)this->block[FELICA_SYSTEM0_INDEX];
    system->systemcode=systemcode;
    memcpy(system->idm,idm,FELICA_IDM_SIZE);
    system->system_block_count=FELICA_BLOCK_COUNT;

    //現在のシステムをシステム0に合わせる
    this->current_system=system;
}

bool Felica::compare_idm(const uint8_t (&idm1)[FELICA_IDM_SIZE],const uint8_t (&idm2)[FELICA_IDM_SIZE]){
    return memcmp(idm1,idm2,FELICA_IDM_SIZE)==0;
}

void Felica::get_pmm(uint8_t (&pmm)[FELICA_PMM_SIZE]){
    memcpy(pmm,this->pmm,FELICA_PMM_SIZE);
}

FelicaSystem* Felica::get_system(const uint8_t (&idm)[FELICA_IDM_SIZE])const{
    blockcount_t i=FELICA_SYSTEM0_INDEX;
    while(i<FELICA_BLOCK_COUNT){
        const FelicaSystem &target=(FelicaSystem&)this->block[i];
        //idmが一致しない場合次へ
        if(!compare_idm(target.idm,idm)){
            i+=target.system_block_count;
            continue;
        }
        return (FelicaSystem*)&target;
    }
    //見つからなかった場合
    return NULL;
}
FelicaSystem* Felica::get_system(const systemcode_t &systemcode)const{
    const systemcode_t wildcard_mask=(((systemcode&0xFF00)==0xFF00)?0xFF00:0x0000)|(((systemcode&0x00FF)==0x00FF)?0x00FF:0x0000);
    blockcount_t i=FELICA_SYSTEM0_INDEX;
    while(i<FELICA_BLOCK_COUNT){
        const FelicaSystem &target=(FelicaSystem&)this->block[i];
        //システムコードが一致しない場合次へ
        if(((~wildcard_mask)&target.systemcode)!=((~wildcard_mask)&systemcode)){
            i+=target.system_block_count;
            continue;
        }
        return (FelicaSystem*)&target;
    }
    //見つからなかった場合
    return NULL;
}
FelicaSystem* Felica::switch_system(const uint8_t (&idm)[FELICA_IDM_SIZE]){
    //現在のシステムのIDmと比較して等しい場合そのまま
    if(compare_idm(this->current_system->idm,idm)){
        return this->current_system;
    }

    FelicaSystem* new_system=this->get_system(idm);

    if(new_system==NULL){
        return NULL;
    }

    this->current_system=new_system;

    return new_system;
}
FelicaSystem* Felica::switch_system(const systemcode_t &systemcode){
    //現在のシステムのシステムコードと比較して等しい場合そのまま
    if(this->current_system->systemcode==systemcode){
        return this->current_system;
    }

    FelicaSystem* new_system=this->get_system(systemcode);

    if(new_system==NULL){
        return NULL;
    }

    this->current_system=new_system;

    return new_system;
}

FelicaSystem* Felica::separate_system(const blockcount_t &new_system_size,const systemcode_t &systemcode,const uint8_t (&idm)[FELICA_IDM_SIZE]){
    if(new_system_size<FELICA_SYSTEM_MIN_BLOCK_COUNT){
        return NULL;
    }
    //システム0を取得
    FelicaSystem &system0=(FelicaSystem&)this->block[FELICA_SYSTEM0_INDEX];

    //初期化済みの場合失敗
    if(this->initialized!=FELICA_NOT_INITIALIZED){
        return NULL;
    }
    //分割後にシステム0のサイズが小さすぎる場合失敗
    if((system0.system_block_count-new_system_size)<FELICA_SYSTEM_MIN_BLOCK_COUNT){
        return NULL;
    }
    //同じシステムコードがすでに存在するとき失敗
    if(this->get_system(systemcode)!=NULL){
        return NULL;
    }
    //同じIDmがすでに存在するとき失敗
    if(this->get_system(idm)!=NULL){
        return NULL;
    }

    system0.system_block_count-=new_system_size;
    //システム0の次に新しいシステムを作成
    FelicaSystem &new_system=(FelicaSystem&)this->block[system0.system_block_count];
    new_system.system_block_count=new_system_size;
    new_system.systemcode=systemcode;
    memcpy(new_system.idm,idm,FELICA_IDM_SIZE);

    return &new_system;
}
FelicaSystem* Felica::separate_system(const blockcount_t &new_system_size,const systemcode_t &systemcode){
    //システム0を取得
    const FelicaSystem &system0=(FelicaSystem&)this->block[FELICA_SYSTEM0_INDEX];
    uint8_t idm[FELICA_IDM_SIZE];
    memcpy(idm,system0.idm,FELICA_IDM_SIZE);
    //IDmを1加算する
    idm[FELICA_IDM_SIZE-1]++;

    return this->separate_system(new_system_size,systemcode,idm);
}
void Felica::initialize_1st(){
    if(this->initialized!=FELICA_NOT_INITIALIZED){
        return;
    }
    blockcount_t i=FELICA_SYSTEM0_INDEX;
    while(i<FELICA_BLOCK_COUNT){
        FelicaSystem &target_system=(FelicaSystem&)this->block[i];
        //エリア0を設定
        FelicaArea &area0=(FelicaArea&)this->block[i+FELICA_SYSTEM_DATA_SIZE];
        area0.areacode=FELICA_AREA0_AREA_CODE;
        area0.end_servicecode=FELICA_AREA0_END_SERVICE_CODE;
        area0.area_key_ver=FELICA_AREA_KEY_VER_NOT_SET;
        area0.parent_code=FELICA_AREA0_AREA_CODE;
        //エリアを一つ登録したので
        target_system.area_service_data_used_count=1;
        target_system.block_used_count=FELICA_AREA_DATA_SIZE;

        i+=target_system.system_block_count;
    }

    this->initialized=FELICA_1ST_INITIALIZED;
}

FelicaArea* Felica::add_area(const areacode_t &areacode,const servicecode_t &end_servicecode,const uint16_t &area_key_ver){
    if(this->initialized!=FELICA_1ST_INITIALIZED){
        return NULL;
    }

    //エンドサービスコードがエリアコードよりも小さかったら不正
    if(end_servicecode<areacode){
        return NULL;
    }
    //エリアコードの形が正しいか
    if(!felica_is_Area_Code(areacode)){
        return NULL;
    }

    //システムに空きがあるか
    if(this->current_system->system_block_count<(FELICA_AREA_DATA_SIZE+this->current_system->block_used_count)){
        return NULL;
    }

    //現在のシステムの場所
    blockcount_t index=(((uint8_t(*)[FELICA_BLOCK_SIZE])this->current_system)-this->block);
    const blockcount_t &used_area_size=this->current_system->area_service_data_used_count;

    FelicaArea *parent_area=NULL;
    //親エリアを探索
    for(blockcount_t i=0;i<used_area_size;i++){
        FelicaArea *area=(FelicaArea*)this->block[index+FELICA_SYSTEM_DATA_SIZE+i];
        //登録できるエリアかどうか
        if(felica_Area_Code_to_Attr(area->areacode)!=FELICA_AREA_ATTR_CAN_CREATE_SUB){
            continue;
        }
        //登録しようとしているエリアコードが親エリアの範囲内に入っているかどうか
        if(!((area->areacode<=areacode)&&(areacode<=area->end_servicecode))){
            continue;
        }
        //エンドサービスコードが親エリア範囲外の時不正
        if(area->end_servicecode<end_servicecode){
            return NULL;
        }

        parent_area=area;
    }
    if(parent_area==NULL){
        return NULL;
    }

    this->current_system->area_service_data_used_count+=1;
    this->current_system->block_used_count+=FELICA_AREA_DATA_SIZE;

    FelicaArea *new_area=(FelicaArea*)this->block[index+FELICA_SYSTEM_DATA_SIZE+used_area_size-1];
    new_area->area_key_ver=area_key_ver;
    new_area->areacode=areacode;
    new_area->end_servicecode=end_servicecode;
    new_area->parent_code=parent_area->areacode;

    return new_area;
}

FelicaService* Felica::add_service(const blockcount_t &new_service_size,const servicecode_t &servicecode,const uint16_t &service_key_ver){
    if(this->initialized!=FELICA_1ST_INITIALIZED){
        return NULL;
    }

    //サービスコードの形が正しいか
    if(!felica_is_Service_Code(servicecode)){
        return NULL;
    }
    //システムの空きがあるか
    if(this->current_system->system_block_count<(FELICA_SERVICE_DATA_SIZE+FELICA_BLOCK_METADATA_SIZE+new_service_size+this->current_system->block_used_count)){
        return NULL;
    }

    //現在のシステムの場所
    blockcount_t index=(((uint8_t(*)[FELICA_BLOCK_SIZE])this->current_system)-this->block);
    const blockcount_t &used_service_size=this->current_system->area_service_data_used_count;

    FelicaArea *parent_area=NULL;
    FelicaService *ref=NULL;
    //親エリアを探索
    for(blockcount_t i=0;i<used_service_size;i++){
        FelicaArea *area=(FelicaArea*)this->block[index+FELICA_SYSTEM_DATA_SIZE+i];
        //エリアかどうか
        if(!felica_is_Area_Code(area->areacode)){
            //サービスだった場合サービス番号が同じかどうか
            if(felica_Service_Code_to_Num(area->areacode)==felica_Service_Code_to_Num(servicecode)){
                //同じサービス番号の場合参照する
                ref=(FelicaService*)area;
            }

            continue;
        }

        //登録しようとしているサービスコードが親エリアの範囲内に入っているかどうか
        if(!((area->areacode<=servicecode)&&(servicecode<=area->end_servicecode))){
            continue;
        }

        parent_area=area;
    }
    if(parent_area==NULL){
        return NULL;
    }

    this->current_system->area_service_data_used_count+=1;
    this->current_system->block_used_count+=(FELICA_SERVICE_DATA_SIZE+FELICA_BLOCK_METADATA_SIZE+new_service_size);
    
    FelicaService *new_service=(FelicaService*)this->block[index+FELICA_SYSTEM_DATA_SIZE+used_service_size-1];
    new_service->service_key_ver=service_key_ver;
    new_service->servicecode=servicecode;
    new_service->parent_code=parent_area->areacode;

    if(ref==NULL){
        //参照しない場合自分をさす
        new_service->reference_index=(((uint8_t(*)[FELICA_BLOCK_SIZE])new_service)-this->block);
        new_service->service_block_count=new_service_size;
    }
    else{
        new_service->reference_index=(((uint8_t(*)[FELICA_BLOCK_SIZE])ref)-this->block);
        new_service->service_block_count=ref->service_block_count;
    }

    return new_service;
}

//ブロックをソートする
void sort_area_service(uint8_t (*start_block)[FELICA_BLOCK_SIZE],const blockcount_t &size){
    uint8_t tmp_block[FELICA_BLOCK_SIZE];
    //ソート そんなに速度はいらないので簡単な選択ソート
    for(blockcount_t i=0;i<size-1;i++){
        areacode_t min_areacode=0xFFFF;
        blockcount_t min_area_index=0;
        for(blockcount_t j=i;j<size;j++){
            if(((FelicaArea*)start_block[j])->areacode<min_areacode){
                min_area_index=j;
                min_areacode=((FelicaArea*)start_block[j])->areacode;
            }
        }
        //交換
        memcpy(tmp_block,start_block[i],FELICA_BLOCK_SIZE);
        memcpy(start_block[i],start_block[min_area_index],FELICA_BLOCK_SIZE);
        memcpy(start_block[min_area_index],tmp_block,FELICA_BLOCK_SIZE);
    }
}

void Felica::initialize_2nd(){
    if(this->initialized!=FELICA_1ST_INITIALIZED){
        return;
    }
    blockcount_t system_index=FELICA_SYSTEM0_INDEX;
    while(system_index<FELICA_BLOCK_COUNT){
        const FelicaSystem &target_system=(FelicaSystem&)this->block[system_index];
        //データブロックの始まりの位置
        const blockcount_t data_block_index=system_index+FELICA_SYSTEM_DATA_SIZE+target_system.area_service_data_used_count;
        //割り当て済みのデータブロックのサイズ
        blockcount_t data_block_used_size=0;
        for(blockcount_t i=0;i<target_system.area_service_data_used_count;i++){
            FelicaService *service=(FelicaService*)this->block[system_index+FELICA_SYSTEM_DATA_SIZE+i];
            if(!felica_is_Service_Code(service->servicecode)){
                continue;
            }

            FelicaService *ref_service=(FelicaService*)this->block[service->reference_index];
            if(ref_service==service){
                service->service_block_index=data_block_index+data_block_used_size;
                data_block_used_size+=(service->service_block_count+FELICA_BLOCK_METADATA_SIZE);
            }
            else{
                service->service_block_index=ref_service->service_block_index;
            }
        }
        sort_area_service(&this->block[system_index+FELICA_SYSTEM_DATA_SIZE],target_system.area_service_data_used_count);


        system_index+=target_system.system_block_count;
    }

    this->initialized=FELICA_2ND_INITIALIZED;
}

FelicaService* Felica::get_service(const servicecode_t &servicecode){
    if(this->initialized!=FELICA_2ND_INITIALIZED){
        return NULL;
    }
    
    //サービスコードの形が正しいか
    if(!felica_is_Service_Code(servicecode)){
        return NULL;
    }

    //現在のシステムの場所
    const blockcount_t index=(((uint8_t(*)[FELICA_BLOCK_SIZE])this->current_system)-this->block);

    blockcount_t l=index+FELICA_SYSTEM_DATA_SIZE;
    blockcount_t r=index+FELICA_SYSTEM_DATA_SIZE+this->current_system->area_service_data_used_count;
    while(l<r){
        blockcount_t m=(l+r)/2;
        if(((FelicaService*)this->block[m])->servicecode==servicecode){
            return (FelicaService*)this->block[m];
        }
        else if(((FelicaService*)this->block[m])->servicecode<servicecode){
            l=m+1;
        }
        else{
            r=m;
        }
    }

    return NULL;
}
FelicaArea* Felica::get_area(const areacode_t &areacode){
    if(this->initialized!=FELICA_2ND_INITIALIZED){
        return NULL;
    }
    
    //サービスコードの形が正しいか
    if(!felica_is_Area_Code(areacode)){
        return NULL;
    }

    //現在のシステムの場所
    const blockcount_t index=(((uint8_t(*)[FELICA_BLOCK_SIZE])this->current_system)-this->block);

    blockcount_t l=index+FELICA_SYSTEM_DATA_SIZE;
    blockcount_t r=index+FELICA_SYSTEM_DATA_SIZE+this->current_system->area_service_data_used_count;
    while(l<r){
        blockcount_t m=(l+r)/2;
        if(((FelicaArea*)this->block[m])->areacode==areacode){
            return (FelicaArea*)this->block[m];
        }
        else if(((FelicaArea*)this->block[m])->areacode<areacode){
            l=m+1;
        }
        else{
            r=m;
        }
    }

    return NULL;
}

bool Felica::write_force(const servicecode_t &servicecode,const blockcount_t &blocknum,const uint8_t (&data)[FELICA_BLOCK_SIZE]){
    FelicaService *service=get_service(servicecode);
    if(service==NULL){
        return false;
    }
    if(service->service_block_count<=blocknum){
        return false;
    }

    //書き込むブロックのメタデータ
    FelicaBlockData *blockdata=(FelicaBlockData*)this->block[service->service_block_index];
    uint8_t (*data_block)[FELICA_BLOCK_SIZE]=&this->block[service->service_block_index+FELICA_BLOCK_METADATA_SIZE];

    if(felica_is_Service_Cyclic(service->servicecode)){
        //書き込み先とデータが同じ場合終了
        if(memcmp(data_block[blockdata->cyclic_index],data,FELICA_BLOCK_SIZE)==0){
            return true;
        }
        blockdata->cyclic_index=(blockdata->cyclic_index+service->service_block_count-1)%service->service_block_count;
        memcpy(data_block[blockdata->cyclic_index],data,FELICA_BLOCK_SIZE);
    }
    else{
        memcpy(data_block[blocknum],data,FELICA_BLOCK_SIZE);
    }

    return true;
}

bool Felica::write(const servicecode_t &servicecode,const blockcount_t &blocknum,const uint8_t (&data)[FELICA_BLOCK_SIZE]){
    return write_force(servicecode,blocknum,data);
}

bool Felica::read_force(const servicecode_t &servicecode,const blockcount_t &blocknum,uint8_t (&data)[FELICA_BLOCK_SIZE]){
    FelicaService *service=get_service(servicecode);
    if(service==NULL){
        return false;
    }
    if(service->service_block_count<=blocknum){
        return false;
    }

    //読み込むブロックのメタデータ
    const FelicaBlockData *blockdata=(FelicaBlockData*)this->block[service->service_block_index];
    const uint8_t (*data_block)[FELICA_BLOCK_SIZE]=&this->block[service->service_block_index+FELICA_BLOCK_METADATA_SIZE];
    if(felica_is_Service_Cyclic(service->servicecode)){
        memcpy(data,data_block[(blocknum+blockdata->cyclic_index)%service->service_block_count],FELICA_BLOCK_SIZE);
    }
    else{
        memcpy(data,data_block[blocknum],FELICA_BLOCK_SIZE);
    }
    return true;
}
bool Felica::read(const servicecode_t &servicecode,const blockcount_t &blocknum,uint8_t (&data)[FELICA_BLOCK_SIZE]){
    return read_force(servicecode,blocknum,data);
}

/**********************************************************************************************
 * Felica listen
 **********************************************************************************************/
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
            listen_Request_Service(idm,node_count,(_uint16_l*)&rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1],txBuf,txBufLen);
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
            const uint8_t block_count=rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+FELICA_SERVICE_CODE_SIZE*service_count];
            if(!(((FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*service_count+2*block_count)<=rxBufLen)&&
                (rxBufLen<=(FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+2*service_count+3*block_count)))){
                return;
            }
            uint8_t idm[FELICA_IDM_SIZE];
            memcpy(idm,&rxBuf[FELICA_CMD_SIZE],FELICA_IDM_SIZE);

            listen_Read_Without_Encryption(idm,service_count,(_uint16_l*)&rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1],block_count,&rxBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2+FELICA_SERVICE_CODE_SIZE*service_count],txBuf,txBufLen);
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
    if(this->initialized!=FELICA_2ND_INITIALIZED){
        return;
    }
    if(this->switch_system(system_code)==NULL){
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
            _uint16_b *tx_systemcode=(_uint16_b*)&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE];
            *tx_systemcode=this->current_system->systemcode;
        }
        break;
        case FELICA_POLLING_REQUEST_CODE_NONE:
        default:
        break;
    }
}
void Felica::listen_Request_Service(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &node_count,const _uint16_l *node_code_list,uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;
    if(this->initialized!=FELICA_2ND_INITIALIZED){
        return;
    }

    //現在のシステムとidmが違った場合システムを切り替え
    if(this->switch_system(idm)==NULL){
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
        const _uint16_l *node_code=&node_code_list[i];
        _uint16_l *node_code_tx=(_uint16_l*)&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*i];

        //与えられたノードコードがサービスコードがエリアコードか判定
        if(felica_is_Area_Code(((areacode_t)*node_code))){
            //エリアコード
            const areacode_t areacode=*node_code;
            const FelicaArea *area=this->get_area(areacode);
            if(area==NULL){
                //エリアが見つからなかった場合0xFFFF
                *node_code_tx=0xFFFF;
            }
            else{
                *node_code_tx=area->area_key_ver;
            }
        }
        else{
            //サービスコード
            //リトルエンディアン
            const servicecode_t service_code=*node_code;
            FelicaService *service=get_service(service_code);
            if(service==NULL){
                //サービスが見つからなかった場合0xFF
                *node_code_tx=0xFFFF;
            }
            else{
                *node_code_tx=service->service_key_ver;
            }
        }
    }
}
void Felica::listen_Request_Response(const uint8_t (&idm)[FELICA_IDM_SIZE],uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;
    if(this->initialized!=FELICA_2ND_INITIALIZED){
        return;
    }

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
void Felica::listen_Read_Without_Encryption(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &service_count,const _uint16_l *service_code_list,const uint8_t &block_count,const uint8_t *block_list,uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;
    if(this->initialized!=FELICA_2ND_INITIALIZED){
        return;
    }
    //現在のシステムとidmが違った場合システムを切り替え
    if(!this->switch_system(idm)){
        return;
    }

    //service_countは1以上1
    if(!(1<=service_count&&service_count<=FELICA_READ_WITHOUT_ENCRYPTION_BUF_SIZE)){
        return;
    }
    if(!(1<=block_count&&block_count<=FELICA_READ_WITHOUT_ENCRYPTION_BUF_SIZE)){
        return;
    }

    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_READ_WITHOUT_ENCRYPTION_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->current_system->idm,FELICA_IDM_SIZE);
    //レスポンスフラグ
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=0x00;
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1]=0x00;
    //長さを一時的に確定
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+2;

    uint16_t idx=0;
    uint8_t block_data[FELICA_READ_WITHOUT_ENCRYPTION_BUF_SIZE][FELICA_BLOCK_SIZE];

    for(uint8_t i=0;i<block_count;i++){
        //アクセスモードが0以外の時終了
        const BlockListElement element(&block_list[idx]);
        idx+=element.get_element_len();
        if(element.access_mode!=0x00){
            txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=i+1;
            return;
        }

        //サービスコードはリトルエンディアン
        const servicecode_t service_code=(servicecode_t)service_code_list[element.service_code_list_order];

        //ブロックの読み込みに失敗した場合終了
        if(!this->read(service_code,element.block_num,block_data[i])){
            txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=i+1;
            return;
        }
    }
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+2]=block_count;
    for(uint8_t i=0;i<block_count;i++){
        memcpy(&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+3+i*FELICA_BLOCK_SIZE],block_data[i],FELICA_BLOCK_SIZE);
    }
    *txBufLen+=1+FELICA_BLOCK_SIZE*block_count;
}

void Felica::listen_Request_System_Code(const uint8_t (&idm)[FELICA_IDM_SIZE],uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;
    if(this->initialized!=FELICA_2ND_INITIALIZED){
        return;
    }
    //現在のシステムとidmが違った場合システムを切り替え
    if(!this->switch_system(idm)){
        return;
    }

    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_REQUEST_SYSTEM_CODE_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->current_system->idm,FELICA_IDM_SIZE);
    uint8_t system_count=0;
    blockcount_t system_index=FELICA_SYSTEM0_INDEX;
    while(system_index<FELICA_BLOCK_COUNT){
        const FelicaSystem *system=(FelicaSystem*)this->block[system_index];
        _uint16_b *system_code=(_uint16_b*)&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*system_count];
        *system_code=system->systemcode;
        system_count++;
        system_index+=system->system_block_count;
    }
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=system_count;
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*system_count;
}

/**********************************************************************************************
 * Block list element
 **********************************************************************************************/
BlockListElement::BlockListElement(const uint8_t &access_mode,const uint8_t &service_code_list_order,const uint16_t &block_num):
    len((block_num<=0xFF)?FELICA_BLOCK_LIST_ELEMENT_SHORT:FELICA_BLOCK_LIST_ELEMENT_LONG),//1ビット
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
    if(this->len==FELICA_BLOCK_LIST_ELEMENT_LONG){
        buf[2]=(this->block_num>>8)&0xFF;
        return FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE;
    }
    return FELICA_BLOCK_LIST_ELEMENT_MIN_SIZE;
}
uint8_t BlockListElement::get_element_len()const{
    return (this->len==FELICA_BLOCK_LIST_ELEMENT_SHORT)?FELICA_BLOCK_LIST_ELEMENT_MIN_SIZE:FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE;
}

/**********************************************************************************************
 * Felicaコマンド
 **********************************************************************************************/
Felica::FelicaCMD::FelicaCMD(){}

void Felica::FelicaCMD::Polling::setup(const systemcode_t &systemcode,const uint8_t &requestcode,const uint8_t &timeslot){
    this->cmd=FELICA_POLLING_CMD_CODE;
    this->systemcode=systemcode;
    this->requestcode=requestcode;
    this->timeslot=timeslot;
}
uint8_t Felica::FelicaCMD::Polling::get_size()const{
    return FELICA_CMD_SIZE+FELICA_SYSTEM_CODE_SIZE+FELICA_POLLING_REQUEST_CODE_SIZE+FELICA_POLLING_TIME_SLOT_SIZE;
}

void Felica::FelicaCMD::Request_Service::setup(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &node_count,const _uint16_l *node_list){
    this->cmd=FELICA_REQUEST_SERVICE_CMD_CODE;
    memcpy(this->idm,idm,FELICA_IDM_SIZE);
    this->node_count=node_count;
    memcpy(this->node_list,node_list,node_count*2);
}
uint8_t Felica::FelicaCMD::Request_Service::get_size()const{
    return FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+this->node_count;
}

void Felica::FelicaCMD::Request_Response::setup(const uint8_t (&idm)[FELICA_IDM_SIZE]){
    this->cmd=FELICA_REQUEST_RESPONSE_CMD_CODE;
    memcpy(this->idm,idm,FELICA_IDM_SIZE);
}
uint8_t Felica::FelicaCMD::Request_Response::get_size()const{
    return FELICA_CMD_SIZE+FELICA_IDM_SIZE;
}


/**********************************************************************************************
 * デバッグ
 **********************************************************************************************/
#ifdef _DEBUG_
    void Felica::show_block(){
        for(uint16_t i=0;i<FELICA_BLOCK_COUNT;i++){
            printf("addr:%p,%04X: ",this->block[i],i);
            for(uint8_t j=0;j<FELICA_BLOCK_SIZE;j++){
                printf("%02X ",this->block[i][j]);
            }
            printf("\n");
        }
    }
#endif//_DEBUG_