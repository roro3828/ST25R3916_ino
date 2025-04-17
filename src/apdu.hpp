/**
 * @attention
 * MIT License
 * 
 * Copyright (c) 2025 ろろ (https://roro.ro)
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

//参照 https://www.nmda.or.jp/nmda/ic-card/iso10536/sec4.html
//https://cardwerk.com/smart-card-standard-iso7816-4-section-6-basic-interindustry-commands/

#ifndef __APDU_HPP__
#define __APDU_HPP__

#include<stdio.h>
#include<cstring>
#include<cstdint>

#define APDU_SEND_DATA_MAX_LEN 0xFF
#define APDU_CLA_LEN            1U
#define APDU_INS_LEN            1U
#define APDU_P1_LEN             1U
#define APDU_P2_LEN             1U

//ISO/IEC 7816-4に準拠したコマンド 0b000000xx
#define APDU_CLA_COMPLIANT_ISO 0x00
//ISO/IEC 7816-4準拠でないコマンド 0b100000xx
#define APDU_CLA_NOT_COMPLIANT 0x80

#define APDU_CMD_READ_BINARY            0xB0

#define APDU_CMD_WRITE_BINARY           0xD0

#define APDU_CMD_UPDATE_BINARY          0xD6

#define APDU_CMD_READ_RECORD            0xB2

#define APDU_CMD_WRITE_RECORD           0xD2

#define APDU_CMD_APPEND_RECORD          0xE2

#define APDU_CMD_UPDATE_RECORD          0xDC

#define APDU_CMD_SELECT_FILE            0xA4

#define APDU_CMD_VERIFY                 0x20

#define APDU_CMD_INTERNAL_AUTHENTICATE  0x88
#define APDU_CMD_EXTERNAL_AUTHENTICATE  0x82
#define APDU_CMD_GET_CHALLENGE          0x84

class APDU{
    uint8_t cla;
    //コマンド
    uint8_t ins;
    //パラメータ
    uint8_t p1;
    uint8_t p2;

    //データ長
    uint16_t lc;
    //送信データ
    uint8_t data[APDU_SEND_DATA_MAX_LEN];
    //レスポンスのデータフィールド最大サイズ
    uint16_t le;
};

#endif //__APDU_HPP__