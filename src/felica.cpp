/**
 * @attention
 * MIT License
 * 
 * Copyright (c) 2024 ろろ (https://roro.ro)
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
Felica::Felica(){
    //ブロックをすべて0で初期化
    for(blockcount_t i=0;i<FELICA_BLOCK_COUNT;i++){
        this->block[i]=FelicaBlock();
    }

    this->initialized=FELICA_NOT_INITIALIZED;
    FelicaBlock::System_data1 &systemdata1=this->block[FELICA_SYSTEM0_INDEX].system_data1;
    FelicaBlock::System_data2 &systemdata2=this->block[FELICA_SYSTEM0_INDEX+1].system_data2;
    systemdata1.system_block_count=FELICA_BLOCK_COUNT;
    systemdata2.is_initialized=FELICA_NOT_INITIALIZED;

    //現在のシステムをシステム0に合わせる
    this->current_system_index=FELICA_SYSTEM0_INDEX;
}
Felica::Felica(const uint8_t (&pmm)[FELICA_PMM_SIZE],const systemcode_t &systemcode,const uint8_t (&idm)[FELICA_IDM_SIZE]){
    //ブロックをすべて0で初期化
    for(blockcount_t i=0;i<FELICA_BLOCK_COUNT;i++){
        this->block[i]=FelicaBlock();
    }

    this->initialized=FELICA_NOT_INITIALIZED;
    memcpy(this->pmm,pmm,FELICA_PMM_SIZE);
    FelicaBlock::System_data1 &systemdata1=this->block[FELICA_SYSTEM0_INDEX].system_data1;
    FelicaBlock::System_data2 &systemdata2=this->block[FELICA_SYSTEM0_INDEX+1].system_data2;
    systemdata1.system_block_count=FELICA_BLOCK_COUNT;
    systemdata1.systemcode=systemcode;
    memcpy(systemdata1.idm,idm,FELICA_IDM_SIZE);
    systemdata1.system_block_count=FELICA_BLOCK_COUNT;
    systemdata2.is_initialized=FELICA_NOT_INITIALIZED;

    //現在のシステムをシステム0に合わせる
    this->current_system_index=FELICA_SYSTEM0_INDEX;
}

bool Felica::compare_idm(const uint8_t (&idm1)[FELICA_IDM_SIZE],const uint8_t (&idm2)[FELICA_IDM_SIZE]){
    return memcmp(idm1,idm2,FELICA_IDM_SIZE)==0;
}

void Felica::get_pmm(uint8_t (&pmm)[FELICA_PMM_SIZE]){
    memcpy(pmm,this->pmm,FELICA_PMM_SIZE);
}

blockcount_t Felica::get_system(const uint8_t (&idm)[FELICA_IDM_SIZE])const{
    blockcount_t i=FELICA_SYSTEM0_INDEX;
    while(i<FELICA_BLOCK_COUNT){
        const FelicaBlock::System_data1 &target=this->block[i].system_data1;
        //idmが一致しない場合次へ
        if(!compare_idm(target.idm,idm)){
            i+=target.system_block_count;
            continue;
        }
        return i;
    }
    //見つからなかった場合
    return FELICA_BLOCK_INDEX_ERROR;
}
blockcount_t Felica::get_system(const systemcode_t &systemcode)const{
    const systemcode_t wildcard_mask=(((systemcode&0xFF00)==0xFF00)?0xFF00:0x0000)|(((systemcode&0x00FF)==0x00FF)?0x00FF:0x0000);
    blockcount_t i=FELICA_SYSTEM0_INDEX;
    while(i<FELICA_BLOCK_COUNT){
        const FelicaBlock::System_data1 &target=this->block[i].system_data1;
        //システムコードが一致しない場合次へ
        if(((~wildcard_mask)&target.systemcode)!=((~wildcard_mask)&systemcode)){
            i+=target.system_block_count;
            continue;
        }
        return i;
    }
    //見つからなかった場合
    return FELICA_BLOCK_INDEX_ERROR;
}
Felica_Return_Code Felica::switch_system(const uint8_t (&idm)[FELICA_IDM_SIZE]){
    const FelicaBlock::System_data1 current_system=this->block[current_system_index].system_data1;
    //現在のシステムのIDmと比較して等しい場合そのまま
    if(compare_idm(current_system.idm,idm)){
        return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
    }

    const blockcount_t new_system_index=this->get_system(idm);

    if(new_system_index==FELICA_BLOCK_INDEX_ERROR){
        return Felica_Return_Code::FELICA_RETURN_CODE_SYSTEM_NOT_FOUND;
    }

    this->current_system_index=new_system_index;

    return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
}
Felica_Return_Code Felica::switch_system(const systemcode_t &systemcode){
    const FelicaBlock::System_data1 current_system=this->block[current_system_index].system_data1;
    //現在のシステムのシステムコードと比較して等しい場合そのまま
    if(current_system.systemcode==systemcode){
        return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
    }

    const blockcount_t new_system_index=this->get_system(systemcode);

    if(new_system_index==FELICA_BLOCK_INDEX_ERROR){
        return Felica_Return_Code::FELICA_RETURN_CODE_SYSTEM_NOT_FOUND;
    }

    this->current_system_index=new_system_index;

    return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
}

Felica_Return_Code Felica::separate_system(const blockcount_t &new_system_size,const systemcode_t &systemcode,const uint8_t (&idm)[FELICA_IDM_SIZE]){
    if(new_system_size<FELICA_SYSTEM_MIN_BLOCK_COUNT){
        return Felica_Return_Code::FELICA_RETURN_CODE_OUT_OF_MEMORY;
    }
    //システム0を取得
    FelicaBlock::System_data1 &system0=this->block[FELICA_SYSTEM0_INDEX].system_data1;

    //初期化済みの場合失敗
    if(this->initialized!=FELICA_NOT_INITIALIZED){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_INITIALIZE;
    }
    //分割後にシステム0のサイズが小さすぎる場合失敗
    if((system0.system_block_count-new_system_size)<FELICA_SYSTEM_MIN_BLOCK_COUNT){
        return Felica_Return_Code::FELICA_RETURN_CODE_OUT_OF_MEMORY;
    }
    //同じシステムコードがすでに存在するとき失敗
    if(this->get_system(systemcode)!=FELICA_BLOCK_INDEX_ERROR){
        return Felica_Return_Code::FELICA_RETURN_CODE_SYSTEM_CODE_EXIST;
    }
    //同じIDmがすでに存在するとき失敗
    if(this->get_system(idm)!=FELICA_BLOCK_INDEX_ERROR){
        return Felica_Return_Code::FELICA_RETURN_CODE_IDM_EXIST;
    }

    system0.system_block_count-=new_system_size;
    //システム0の次に新しいシステムを作成
    FelicaBlock::System_data1 &new_system1=this->block[system0.system_block_count].system_data1;
    FelicaBlock::System_data2 &new_system2=this->block[system0.system_block_count+1].system_data2;
    new_system1.system_block_count=new_system_size;
    new_system1.systemcode=systemcode;
    memcpy(new_system1.idm,idm,FELICA_IDM_SIZE);
    new_system2.is_initialized=FELICA_NOT_INITIALIZED;

    return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
}
Felica_Return_Code Felica::separate_system(const blockcount_t &new_system_size,const systemcode_t &systemcode){
    //システム0を取得
    FelicaBlock::System_data1 &system0=this->block[FELICA_SYSTEM0_INDEX].system_data1;
    uint8_t idm[FELICA_IDM_SIZE];
    memcpy(idm,system0.idm,FELICA_IDM_SIZE);
    //IDmを1加算する
    idm[FELICA_IDM_SIZE-1]++;

    return this->separate_system(new_system_size,systemcode,idm);
}
Felica_Return_Code Felica::initialize_1st(){
    if(this->initialized!=FELICA_NOT_INITIALIZED){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_INITIALIZE;
    }
    blockcount_t b=FELICA_SYSTEM0_INDEX;
    while(b<FELICA_BLOCK_COUNT){
        FelicaBlock::System_data1 &target_system=this->block[b].system_data1;
        //エリア0を設定
        FelicaBlock::Area &area0=this->block[b+FELICA_SYSTEM_DATA_SIZE].area;
        area0.areacode=FELICA_AREA0_AREA_CODE;
        area0.end_servicecode=FELICA_AREA0_END_SERVICE_CODE;
        area0.area_key_ver=FELICA_AREA_KEY_VER_NOT_SET;
        area0.parent_code=FELICA_AREA0_AREA_CODE;
        //エリアを一つ登録したので
        target_system.area_service_data_used_count=1;
        target_system.block_used_count=FELICA_AREA_DATA_SIZE;

        b+=target_system.system_block_count;
    }

    this->initialized=FELICA_1ST_INITIALIZED;
    return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
}

Felica_Return_Code Felica::add_area(const areacode_t &areacode,const servicecode_t &end_servicecode,const uint16_t &area_key_ver){
    if(this->initialized!=FELICA_1ST_INITIALIZED){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_INITIALIZE;
    }

    //エンドサービスコードがエリアコードよりも小さかったら不正
    if(end_servicecode<areacode){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_PARAM;
    }
    //エリアコードの形が正しいか
    if(!felica_is_Area_Code(areacode)){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_PARAM;
    }

    FelicaBlock::System_data1 &current_system=this->block[this->current_system_index].system_data1;

    //システムに空きがあるか
    if(current_system.system_block_count<(FELICA_AREA_DATA_SIZE+current_system.block_used_count)){
        return Felica_Return_Code::FELICA_RETURN_CODE_OUT_OF_MEMORY;
    }

    FelicaBlock::Area *parent_area=NULL;
    //親エリアを探索
    for(blockcount_t i=0;i<current_system.area_service_data_used_count;i++){
        FelicaBlock::Area *area=&this->block[this->current_system_index+FELICA_SYSTEM_DATA_SIZE+i].area;
        //登録できるエリアかどうか
        if(felica_Area_Code_to_Attr(area->areacode)!=FELICA_AREA_ATTR_CAN_CREATE_SUB){
            continue;
        }
        //登録しようとしているエリアコードが親エリアの範囲内に入っているかどうか
        if(!((area->areacode<areacode)&&(areacode<=area->end_servicecode))){
            continue;
        }
        //エンドサービスコードが親エリア範囲外の時不正
        if(area->end_servicecode<end_servicecode){
            return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_END_SERVICE_CODE;
        }

        parent_area=area;
    }
    if(parent_area==NULL){
        return Felica_Return_Code::FELICA_RETURN_CODE_CAN_NOT_FIND_PARENT;
    }


    FelicaBlock::Area &new_area=this->block[current_system_index+FELICA_SYSTEM_DATA_SIZE+current_system.area_service_data_used_count].area;

    new_area.area_key_ver=area_key_ver;
    new_area.areacode=areacode;
    new_area.end_servicecode=end_servicecode;
    new_area.parent_code=parent_area->areacode;

    current_system.area_service_data_used_count+=1;
    current_system.block_used_count+=FELICA_AREA_DATA_SIZE;

    return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
}

Felica_Return_Code Felica::add_service(const blockcount_t &new_service_size,const servicecode_t &servicecode,const uint16_t &service_key_ver){
    if(this->initialized!=FELICA_1ST_INITIALIZED){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_INITIALIZE;
    }

    //サービスコードの形が正しいか
    if(!felica_is_Service_Code(servicecode)){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_PARAM;
    }

    FelicaBlock::System_data1 &current_system=this->block[this->current_system_index].system_data1;
    //システムの空きがあるか
    if(current_system.system_block_count<(FELICA_SERVICE_DATA_SIZE+FELICA_BLOCK_METADATA_SIZE+new_service_size+current_system.block_used_count)){
        return Felica_Return_Code::FELICA_RETURN_CODE_OUT_OF_MEMORY;
    }

    FelicaBlock::Area *parent_area=NULL;
    blockcount_t ref_index=FELICA_BLOCK_INDEX_ERROR;
    //親エリアを探索
    for(blockcount_t i=0;i<current_system.area_service_data_used_count;i++){
        FelicaBlock::Area *area=&this->block[this->current_system_index+FELICA_SYSTEM_DATA_SIZE+i].area;
        //エリアかどうか
        if(felica_is_Service_Code(area->areacode)){
            //サービスだった場合サービス番号が同じかどうか
            if(felica_Service_Code_to_Num(area->areacode)==felica_Service_Code_to_Num(servicecode)){
                //同じサービス番号の場合参照する
                ref_index=this->current_system_index+FELICA_SYSTEM_DATA_SIZE+i;
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
        return Felica_Return_Code::FELICA_RETURN_CODE_CAN_NOT_FIND_PARENT;
    }

    
    FelicaBlock::Service &new_service=this->block[this->current_system_index+FELICA_SYSTEM_DATA_SIZE+current_system.area_service_data_used_count].service;
    new_service.service_key_ver=service_key_ver;
    new_service.servicecode=servicecode;
    new_service.parent_code=parent_area->areacode;

    if(ref_index==FELICA_BLOCK_INDEX_ERROR){
        //参照しない場合自分をさす
        new_service.reference_index=this->current_system_index+FELICA_SYSTEM_DATA_SIZE+current_system.area_service_data_used_count;
        new_service.service_block_count=new_service_size;
    }
    else{
        new_service.reference_index=ref_index;
        new_service.service_block_count=this->block[ref_index].service.service_block_count;
    }
    current_system.area_service_data_used_count+=1;
    current_system.block_used_count+=(FELICA_SERVICE_DATA_SIZE+FELICA_BLOCK_METADATA_SIZE+new_service_size);

    return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
}

//ブロックをソートする
void sort_area_service(FelicaBlock *start_block,const blockcount_t &size){
    FelicaBlock tmp_block;
    //ソート そんなに速度はいらないので簡単な選択ソート
    for(blockcount_t i=0;i<size-1;i++){
        areacode_t min_areacode=0xFFFF;
        blockcount_t min_area_index=0;
        for(blockcount_t j=i;j<size;j++){
            if(start_block[j].area.areacode<min_areacode){
                min_area_index=j;
                min_areacode=start_block[j].area.areacode;
            }
        }
        //交換
        memcpy(&tmp_block,&start_block[i],FELICA_BLOCK_SIZE);
        memcpy(&start_block[i],&start_block[min_area_index],FELICA_BLOCK_SIZE);
        memcpy(&start_block[min_area_index],&tmp_block,FELICA_BLOCK_SIZE);
    }
}

Felica_Return_Code Felica::initialize_2nd(){
    if(this->initialized!=FELICA_1ST_INITIALIZED){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_INITIALIZE;
    }
    const FelicaBlock::System_data1 &system_data1=this->block[this->current_system_index].system_data1;
    FelicaBlock::System_data2 &system_data2=this->block[this->current_system_index+1].system_data2;
    if(system_data2.is_initialized!=FELICA_NOT_INITIALIZED){
        return Felica_Return_Code::FELICA_RETURN_CODE_INVALID_INITIALIZE;
    }
    //データブロックの始まりの位置
    const blockcount_t data_block_index=this->current_system_index+FELICA_SYSTEM_DATA_SIZE+system_data1.area_service_data_used_count;
    //割り当て済みのデータブロックのサイズ
    blockcount_t data_block_used_size=0;
    for(blockcount_t i=0;i<system_data1.area_service_data_used_count;i++){
        FelicaBlock::Service &service=this->block[this->current_system_index+FELICA_SYSTEM_DATA_SIZE+i].service;
        if(!felica_is_Service_Code(service.servicecode)){
            continue;
        }

        const FelicaBlock::Service &ref_service=this->block[service.reference_index].service;
        if(ref_service.servicecode==service.servicecode){
            service.service_block_index=data_block_index+data_block_used_size;
            data_block_used_size+=(service.service_block_count+FELICA_BLOCK_METADATA_SIZE);
        }
        else{
            service.service_block_index=ref_service.service_block_index;
        }
    }
    sort_area_service(&this->block[this->current_system_index+FELICA_SYSTEM_DATA_SIZE],system_data1.area_service_data_used_count);
    system_data2.is_initialized=FELICA_2ND_INITIALIZED;

    return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
}
Felica_Return_Code Felica::initialize_2nd(const systemcode_t &systemcode){
    this->switch_system(systemcode);
    return this->initialize_2nd();
}
Felica_Return_Code Felica::initialize_2nd(const uint8_t (&idm)[FELICA_IDM_SIZE]){
    this->switch_system(idm);
    return this->initialize_2nd();
}

FelicaBlock::Service* Felica::get_service(const servicecode_t &servicecode){
    const FelicaBlock::System_data2 &system_data2=this->block[this->current_system_index+1].system_data2;
    if(system_data2.is_initialized!=FELICA_2ND_INITIALIZED){
        return NULL;
    }
    
    //サービスコードの形が正しいか
    if(!felica_is_Service_Code(servicecode)){
        return NULL;
    }

    blockcount_t l=this->current_system_index+FELICA_SYSTEM_DATA_SIZE;
    blockcount_t r=this->current_system_index+FELICA_SYSTEM_DATA_SIZE+this->block[this->current_system_index].system_data1.area_service_data_used_count;
    while(l<r){
        blockcount_t m=(l+r)/2;
        if(this->block[m].service.servicecode==servicecode){
            return &this->block[m].service;
        }
        else if(this->block[m].service.servicecode<servicecode){
            l=m+1;
        }
        else{
            r=m;
        }
    }

    return NULL;
}
FelicaBlock::Area* Felica::get_area(const areacode_t &areacode){
    const FelicaBlock::System_data2 &system_data2=this->block[this->current_system_index+1].system_data2;
    if(system_data2.is_initialized!=FELICA_2ND_INITIALIZED){
        return NULL;
    }
    
    //サービスコードの形が正しいか
    if(!felica_is_Area_Code(areacode)){
        return NULL;
    }

    blockcount_t l=this->current_system_index+FELICA_SYSTEM_DATA_SIZE;
    blockcount_t r=this->current_system_index+FELICA_SYSTEM_DATA_SIZE+this->block[this->current_system_index].system_data1.area_service_data_used_count;
    while(l<r){
        blockcount_t m=(l+r)/2;
        if(this->block[m].area.areacode==areacode){
            return &this->block[m].area;
        }
        else if(this->block[m].area.areacode<areacode){
            l=m+1;
        }
        else{
            r=m;
        }
    }

    return NULL;
}

Felica_Return_Code Felica::write_force(const servicecode_t &servicecode,const blockcount_t &blocknum,const uint8_t (&data)[FELICA_BLOCK_SIZE]){
    const FelicaBlock::Service *service=get_service(servicecode);
    if(service==NULL){
        return Felica_Return_Code::FELICA_RETURN_CODE_SERVICE_NOT_FOUND;
    }
    if(service->service_block_count<=blocknum){
        return Felica_Return_Code::FELICA_RETURN_CODE_SYSTEM_INDEX_OUT_OF_RANGE;
    }

    //書き込むブロックのメタデータ
    FelicaBlock::Meta &blockdata=this->block[service->service_block_index].meta;
    uint8_t (*data_block)[FELICA_BLOCK_SIZE]=&this->block[service->service_block_index+FELICA_BLOCK_METADATA_SIZE].data;

    if(felica_is_Service_Cyclic(service->servicecode)){
        //書き込み先とデータが同じ場合終了
        if(memcmp(data_block[blockdata.cyclic_index],data,FELICA_BLOCK_SIZE)==0){
            return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
        }
        blockdata.cyclic_index=(blockdata.cyclic_index+service->service_block_count-1)%service->service_block_count;
        memcpy(data_block[blockdata.cyclic_index],data,FELICA_BLOCK_SIZE);
    }
    else{
        memcpy(data_block[blocknum],data,FELICA_BLOCK_SIZE);
    }

    return Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS;
}

Felica_Return_Code Felica::write(const servicecode_t &servicecode,const blockcount_t &blocknum,const uint8_t (&data)[FELICA_BLOCK_SIZE]){
    return write_force(servicecode,blocknum,data);
}

bool Felica::read_force(const servicecode_t &servicecode,const blockcount_t &blocknum,uint8_t (&data)[FELICA_BLOCK_SIZE]){
    const FelicaBlock::Service *service=get_service(servicecode);
    if(service==NULL){
        return false;
    }
    if(service->service_block_count<=blocknum){
        return false;
    }

    //読み込むブロックのメタデータ
    const FelicaBlock::Meta &blockdata=this->block[service->service_block_index].meta;
    const uint8_t (*data_block)[FELICA_BLOCK_SIZE]=&this->block[service->service_block_index+FELICA_BLOCK_METADATA_SIZE].data;
    if(felica_is_Service_Cyclic(service->servicecode)){
        memcpy(data,data_block[(blocknum+blockdata.cyclic_index)%service->service_block_count],FELICA_BLOCK_SIZE);
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
    if(this->switch_system(system_code)!=Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS){
        return;
    }
    const FelicaBlock::System_data2 &system_data2=this->block[this->current_system_index+1].system_data2;
    if(system_data2.is_initialized!=FELICA_2ND_INITIALIZED){
        return;
    }

    //モードを0にリセット
    this->current_mode=0x00;
    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_POLLING_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->block[this->current_system_index].system_data1.idm,FELICA_IDM_SIZE);
    memcpy(&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE],this->pmm,FELICA_PMM_SIZE);
    //txBufの長さを一時的に確定させる
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE;

    switch(request_code){
        case FELICA_POLLING_REQUEST_CODE_SYSTEMCODE:{
            *txBufLen+=FELICA_SYSTEM_CODE_SIZE;
            //システムコードはビックエンディアン
            _uint16_b *tx_systemcode=(_uint16_b*)&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE];
            *tx_systemcode=this->block[this->current_system_index].system_data1.systemcode;
        }
        break;
        case FELICA_POLLING_REQUEST_CODE_NONE:
        default:
        break;
    }
}
void Felica::listen_Request_Service(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &node_count,const _uint16_l *node_code_list,uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;

    //現在のシステムとidmが違った場合システムを切り替え
    if(this->switch_system(idm)!=Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS){
        return;
    }
    const FelicaBlock::System_data2 &system_data2=this->block[this->current_system_index+1].system_data2;
    if(system_data2.is_initialized!=FELICA_2ND_INITIALIZED){
        return;
    }
    
    //ノード数が1以上32以下じゃないとき
    if(!((FELICA_REQUEST_SERVICE_NODE_MIN<=node_count)&&(node_count<=FELICA_REQUEST_SERVICE_NODE_MAX))){
        return;
    }
    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_REQUEST_SERVICE_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->block[this->current_system_index].system_data1.idm,FELICA_IDM_SIZE);
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=node_count;
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*node_count;

    for(uint8_t i=0;i<node_count;i++){
        const _uint16_l *node_code=&node_code_list[i];
        _uint16_l *node_code_tx=(_uint16_l*)&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*i];

        //与えられたノードコードがサービスコードがエリアコードか判定
        if(felica_is_Area_Code(((areacode_t)*node_code))){
            //エリアコード
            const areacode_t areacode=*node_code;
            const FelicaBlock::Area *area=this->get_area(areacode);
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
            const FelicaBlock::Service *service=get_service(service_code);
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

    //現在のシステムとidmが違った場合システムを切り替え
    if(this->switch_system(idm)!=Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS){
        return;
    }
    const FelicaBlock::System_data2 &system_data2=this->block[this->current_system_index+1].system_data2;
    if(system_data2.is_initialized!=FELICA_2ND_INITIALIZED){
        return;
    }

    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+1;
    
    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_REQUEST_RESPONSE_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->block[this->current_system_index].system_data1.idm,FELICA_IDM_SIZE);
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=this->current_mode;
}
void Felica::listen_Read_Without_Encryption(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &service_count,const _uint16_l *service_code_list,const uint8_t &block_count,const uint8_t *block_list,uint8_t *txBuf,uint16_t *txBufLen){
    *txBufLen=0;

    //現在のシステムとidmが違った場合システムを切り替え
    if(this->switch_system(idm)!=Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS){
        return;
    }
    const FelicaBlock::System_data2 &system_data2=this->block[this->current_system_index+1].system_data2;
    if(system_data2.is_initialized!=FELICA_2ND_INITIALIZED){
        return;
    }

    //service_countは1以上1
    if(!(1<=service_count&&service_count<=FELICA_READ_WITHOUT_ENCRYPTION_MAX_SERVICE_COUNT)){
        return;
    }
    if(!(1<=block_count&&block_count<=FELICA_READ_WITHOUT_ENCRYPTION_MAX_BLOCK_COUNT)){
        return;
    }

    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_READ_WITHOUT_ENCRYPTION_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->block[this->current_system_index].system_data1.idm,FELICA_IDM_SIZE);
    //レスポンスフラグ
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=0x00;
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1]=0x00;
    //長さを一時的に確定
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+2;

    uint16_t idx=0;
    uint8_t block_data[FELICA_READ_WITHOUT_ENCRYPTION_MAX_BLOCK_COUNT][FELICA_BLOCK_SIZE];

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

    //現在のシステムとidmが違った場合システムを切り替え
    if(this->switch_system(idm)!=Felica_Return_Code::FELICA_RETURN_CODE_SUCCESS){
        return;
    }
    const FelicaBlock::System_data2 &system_data2=this->block[this->current_system_index+1].system_data2;
    if(system_data2.is_initialized!=FELICA_2ND_INITIALIZED){
        return;
    }

    //1バイト目がレスポンスコード
    txBuf[0]=FELICA_REQUEST_SYSTEM_CODE_RES_CODE;
    memcpy(&txBuf[FELICA_CMD_SIZE],this->block[this->current_system_index].system_data1.idm,FELICA_IDM_SIZE);
    uint8_t system_count=0;
    blockcount_t system_index=FELICA_SYSTEM0_INDEX;
    while(system_index<FELICA_BLOCK_COUNT){
        const FelicaBlock::System_data1 &system=this->block[system_index].system_data1;
        _uint16_b *system_code=(_uint16_b*)&txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*system_count];
        *system_code=system.systemcode;
        system_count++;
        system_index+=system.system_block_count;
    }
    txBuf[FELICA_CMD_SIZE+FELICA_IDM_SIZE]=system_count;
    *txBufLen=FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+2*system_count;
}

/************
 * Block
 */
FelicaBlock::FelicaBlock(){
    memset(this->data,0U,FELICA_BLOCK_SIZE);
}

/**********************************************************************************************
 * Block list element
 **********************************************************************************************/
BlockListElement::BlockListElement(){
    access_mode=0b111;
    block_num=0xFFFF;
    service_code_list_order=0b1111;
    len=FELICA_BLOCK_LIST_ELEMENT_LONG;
}
BlockListElement::BlockListElement(const uint8_t &access_mode,const uint8_t &service_code_list_order,const uint16_t &block_num):
    len((block_num<=0xFF)?FELICA_BLOCK_LIST_ELEMENT_SHORT:FELICA_BLOCK_LIST_ELEMENT_LONG),
    access_mode(access_mode&0b111),                         //3ビット
    service_code_list_order(service_code_list_order&0b1111),//4ビット
    block_num(block_num){}
BlockListElement::BlockListElement(const uint8_t &access_mode,const uint8_t &service_code_list_order,const uint16_t &block_num,const uint8_t &len):
    len(len),
    access_mode(access_mode&0b111),                         //3ビット
    service_code_list_order(service_code_list_order&0b1111),//4ビット
    block_num((len==FELICA_BLOCK_LIST_ELEMENT_SHORT)?(block_num&0xFF):block_num){}
BlockListElement::BlockListElement(const uint8_t *buf):
    BlockListElement(
        (buf[0]>>FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT),
        buf[0],
        (((uint16_t)buf[1])+(((uint16_t)buf[2])<<8)),
        (buf[0]>>(FELICA_BLOCK_LIST_ELEMENT_ACCESS_MODE_BIT+FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT))
        ){}
uint8_t BlockListElement::set_element_to_buf(uint8_t *buf)const{
    buf[0]=(this->len<<(FELICA_BLOCK_LIST_ELEMENT_ACCESS_MODE_BIT+FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT))|(this->access_mode<<FELICA_BLOCK_LIST_ELEMENT_SERVICE_CODE_LIST_ORDER_BIT)|this->service_code_list_order;
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
FelicaCMD::FelicaCMD(){}

void FelicaCMD::Polling::setup(const systemcode_t &systemcode,const uint8_t &requestcode,const uint8_t &timeslot){
    this->cmd=FELICA_POLLING_CMD_CODE;
    this->systemcode=systemcode;
    this->requestcode=requestcode;
    this->timeslot=timeslot;
}
uint8_t FelicaCMD::Polling::get_size()const{
    return FELICA_CMD_SIZE+FELICA_SYSTEM_CODE_SIZE+FELICA_POLLING_REQUEST_CODE_SIZE+FELICA_POLLING_TIME_SLOT_SIZE;
}

void FelicaCMD::Request_Service::setup(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &node_count,const _uint16_l *node_list){
    this->cmd=FELICA_REQUEST_SERVICE_CMD_CODE;
    memcpy(this->idm,idm,FELICA_IDM_SIZE);
    this->node_count=node_count;
    memcpy(this->node_list,node_list,node_count*2);
}
uint8_t FelicaCMD::Request_Service::get_size()const{
    return FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+this->node_count*2;
}

void FelicaCMD::Request_Response::setup(const uint8_t (&idm)[FELICA_IDM_SIZE]){
    this->cmd=FELICA_REQUEST_RESPONSE_CMD_CODE;
    memcpy(this->idm,idm,FELICA_IDM_SIZE);
}
uint8_t FelicaCMD::Request_Response::get_size()const{
    return FELICA_CMD_SIZE+FELICA_IDM_SIZE;
}

/***********************************************
 * ここからRead without encryption
 ***********************************************/
void FelicaCMD::Read_Without_Encryption::setup(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint8_t &service_count,const uint8_t &block_count){
    this->cmd=FELICA_READ_WITHOUT_ENCRYPTION_CMD_CODE;
    memcpy(this->idm,idm,FELICA_IDM_SIZE);
    //データを初期化
    memset(this->data,0xFFU,(FELICA_READ_WITHOUT_ENCRYPTION_MAX_SERVICE_COUNT*FELICA_SERVICE_CODE_SIZE+1+FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE*FELICA_READ_WITHOUT_ENCRYPTION_MAX_BLOCK_COUNT));
    //サービスコード数が最大値よりも大きかった場合最大値にする
    this->service_count=(FELICA_READ_WITHOUT_ENCRYPTION_MAX_SERVICE_COUNT<service_count)?FELICA_READ_WITHOUT_ENCRYPTION_MAX_SERVICE_COUNT:service_count;
    //ブロック数が最大値よりも大きかった場合最大値にする
    this->data[service_count*FELICA_SERVICE_CODE_SIZE]=(FELICA_READ_WITHOUT_ENCRYPTION_MAX_BLOCK_COUNT<block_count)?FELICA_READ_WITHOUT_ENCRYPTION_MAX_BLOCK_COUNT:block_count;
}
uint8_t FelicaCMD::Read_Without_Encryption::get_size()const{
    uint8_t idx=this->get_service_count()*FELICA_SERVICE_CODE_SIZE+1;
    for(uint8_t j=0;j<this->get_block_count();j++){
        const BlockListElement tmp(&this->data[idx]);
        idx+=tmp.get_element_len();
    }
    return FELICA_CMD_SIZE+FELICA_IDM_SIZE+1+idx;
}

uint8_t FelicaCMD::Read_Without_Encryption::get_service_count()const{
    return this->service_count;
}
servicecode_t FelicaCMD::Read_Without_Encryption::get_service(const uint8_t &i)const{
    if(this->service_count<=i){
        return 0xFFFF;
    }
    
    return *((_uint16_l*)&this->data[i*2]);
}
bool FelicaCMD::Read_Without_Encryption::set_service(const uint8_t &i,const servicecode_t service_code){
    if(this->service_count<=i){
        return false;
    }
    //リトルエンディアン
    *((_uint16_l*)&this->data[i*FELICA_SERVICE_CODE_SIZE])=service_code;
    return true;
}

uint8_t FelicaCMD::Read_Without_Encryption::get_block_count()const{
    return this->data[this->service_count*FELICA_SERVICE_CODE_SIZE];
}
bool FelicaCMD::Read_Without_Encryption::get_block_list_element(const uint8_t &i,BlockListElement &element)const{
    if(this->get_block_count()<=i){
        return false;
    }
    uint8_t idx=this->get_service_count()*FELICA_SERVICE_CODE_SIZE+1;
    for(uint8_t j=0;j<=i;j++){
        const BlockListElement tmp(&this->data[idx]);
        idx+=tmp.get_element_len();
        if(j==i){
            element.access_mode=tmp.access_mode;
            element.block_num=tmp.block_num;
            element.service_code_list_order=tmp.service_code_list_order;
        }
    }
    return true;
}
bool FelicaCMD::Read_Without_Encryption::set_block_list_element(const uint8_t &i,const BlockListElement &element){
    if(this->get_block_count()<=i){
        return false;
    }
    uint8_t idx=this->get_service_count()*FELICA_SERVICE_CODE_SIZE+1;
    for(uint8_t j=0;j<=i;j++){
        const BlockListElement tmp(&this->data[idx]);
        if(j==i){
            if(tmp.get_element_len()<element.get_element_len()){
                memmove(&this->data[idx+FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE],&this->data[idx+FELICA_BLOCK_LIST_ELEMENT_MIN_SIZE],(FELICA_READ_WITHOUT_ENCRYPTION_MAX_SERVICE_COUNT*FELICA_SERVICE_CODE_SIZE+1+FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE*FELICA_READ_WITHOUT_ENCRYPTION_MAX_BLOCK_COUNT)-idx-FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE);
            }
            else if(tmp.get_element_len()>element.get_element_len()){
                memmove(&this->data[idx+FELICA_BLOCK_LIST_ELEMENT_MIN_SIZE],&this->data[idx+FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE],(FELICA_READ_WITHOUT_ENCRYPTION_MAX_SERVICE_COUNT*FELICA_SERVICE_CODE_SIZE+1+FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE*FELICA_READ_WITHOUT_ENCRYPTION_MAX_BLOCK_COUNT)-idx-FELICA_BLOCK_LIST_ELEMENT_MAX_SIZE);
            }
            element.set_element_to_buf(&this->data[idx]);
        }
        idx+=tmp.get_element_len();
    }
    return true;
}

void FelicaCMD::Search_Service_Code::setup(const uint8_t (&idm)[FELICA_IDM_SIZE],const uint16_t &service_index){
    this->cmd=FELICA_SEARCH_SERVICE_CODE_CMD_CODE;
    memcpy(this->idm,idm,FELICA_IDM_SIZE);
    this->service_index=service_index;
}
uint8_t FelicaCMD::Search_Service_Code::get_size()const{
    return FELICA_CMD_SIZE+FELICA_IDM_SIZE+2;
}

void FelicaCMD::Request_System_Code::setup(const uint8_t (&idm)[FELICA_IDM_SIZE]){
    this->cmd=FELICA_REQUEST_SYSTEM_CODE_CMD_CODE;
    memcpy(this->idm,idm,FELICA_IDM_SIZE);
}
uint8_t FelicaCMD::Request_System_Code::get_size()const{
    return FELICA_CMD_SIZE+FELICA_IDM_SIZE;
}
/**********************************************************************************************
 * Felica RES
 **********************************************************************************************/
FelicaRES::Polling::Polling(const uint8_t *buf,const uint16_t &bufsize):
    size(bufsize),
    res(buf[0]){
    //有効な形でなかったら終了
    if(!this->is_valid()){
        return;
    }
    memcpy(this->_idm,&buf[FELICA_CMD_SIZE],FELICA_IDM_SIZE);
    memcpy(this->_pmm,&buf[FELICA_CMD_SIZE+FELICA_IDM_SIZE],FELICA_PMM_SIZE);

    if(this->size==(FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE+FELICA_POLLING_REQUEST_DATA_SIZE)){
        memcpy(this->_req_data,&buf[FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE],FELICA_POLLING_REQUEST_DATA_SIZE);
    }
    else{
        memset(this->_req_data,0xFF,FELICA_POLLING_REQUEST_DATA_SIZE);
    }

}
bool FelicaRES::Polling::is_valid()const{
    //長さが正しいか
    if((this->size!=(FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE+FELICA_POLLING_REQUEST_DATA_SIZE))&&(this->size!=(FELICA_CMD_SIZE+FELICA_IDM_SIZE+FELICA_PMM_SIZE))){
        return false;
    }
    //レスポンスコードが正しいか
    if(this->res!=FELICA_POLLING_RES_CODE){
        return false;
    }

    return true;
}

/**********************************************************************************************
 * デバッグ
 **********************************************************************************************/
#ifdef _DEBUG_
    void Felica::show_block(){
        for(uint16_t i=0;i<FELICA_BLOCK_COUNT;i++){
            printf("addr:%p,%04X: ",this->block[i],i);
            for(uint8_t j=0;j<FELICA_BLOCK_SIZE;j++){
                printf("%02X ",this->block[i].data[j]);
            }
            printf("\n");
        }
    }
#endif//_DEBUG_