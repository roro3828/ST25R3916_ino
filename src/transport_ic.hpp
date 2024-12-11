#ifndef TRANSPORT_IC_HPP
#define TRANSPORT_IC_HPP

#include"felica.hpp"

#define TRANSPORT_IC_SYSTEM_CODE (systemcode_t)0x0003
#define TRANSPORT_IC_USAGE_HISTORY_SERVICE_CODE (servicecode_t)0x090F
#define TRANSPORT_IC_MONTH_MIN      1
#define TRANSPORT_IC_MONTH_MAX      12
#define TRANSPORT_IC_DAY_MIN        1
#define TRANSPORT_IC_DAY_MAX        31
#define to_valid_month(m)   (((m+TRANSPORT_IC_MONTH_MAX-TRANSPORT_IC_MONTH_MIN)%(TRANSPORT_IC_MONTH_MAX-TRANSPORT_IC_MONTH_MIN+1))+TRANSPORT_IC_MONTH_MIN)
#define to_valid_day(d)     (((d+TRANSPORT_IC_DAY_MAX-TRANSPORT_IC_DAY_MIN)%(TRANSPORT_IC_DAY_MAX-TRANSPORT_IC_DAY_MIN+1))+TRANSPORT_IC_DAY_MIN)

class TransportIC{
    /**
     * @brief 8bit BCD 上位4bitを10の位、下位4bitを1の位として扱う型
     */
    struct BCD{
        private:
        uint8_t data;
        public:
        BCD& operator=(const uint8_t& x){
            this->data=(x%10);
            this->data|=(((x/10)%10)<<4);
            return *this;
        }
        operator uint8_t()const{
            return (data&0b1111)+10*(data>>4);
        }
    };
    //日付データ
    union Date{
        //年
        struct{
            private:
            uint8_t data[2];
            public:
            void operator=(const uint8_t& year){
                this->data[0]=((year<<1)+(this->data[0]&0b00000001));
            }
            operator uint8_t()const{
                return (this->data[0]>>1);
            }
        }year;
        //月 1~12のみ有効
        struct{
            private:
            uint8_t data[2];
            public:
            void operator=(const uint8_t& month){
                this->data[0]=((to_valid_month(month)>>3)&0b1)+(this->data[0]&0b11111110);
                this->data[1]=((to_valid_month(month)<<5)&0b11100000)+(this->data[1]&0b00011111);
            }
            operator uint8_t()const{
                return ((this->data[0]<<3)&0b1000)+((this->data[1]>>5)&0b111);
            }
        }month;
        //日 1~31のみ有効
        struct{
            private:
            uint8_t data[2];
            public:
            void operator=(const uint8_t& day){
                this->data[1]=(to_valid_day(day)&0b11111)+(this->data[1]&0b11100000);
            }
            operator uint8_t()const{
                return (this->data[1]&0b00011111);
            }
        }day;
    };
    //時刻データ時間&分
    struct Time_HM{
        TransportIC::BCD hour;
        TransportIC::BCD munit;
    };
    //時刻データ時間&分&秒(2秒単位)
    union Time_HMS{
        struct{
            private:
            uint8_t data[2];
            public:
            void operator=(const uint8_t& hour){
                this->data[0]=((hour<<3)+(this->data[0]&0b00000111));
            }
            operator uint8_t()const{
                return (this->data[0]>>3);
            }
        }hour;
        struct{
            private:
            uint8_t data[2];
            public:
            void operator=(const uint8_t& munit){
                this->data[0]=((munit>>3)&0b111)+(this->data[0]&0b11111000);
                this->data[1]=((munit<<5)&0b11100000)+(this->data[1]&0b00011111);
            }
            operator uint8_t()const{
                return ((this->data[0]<<3)&0b111000)+((this->data[1]>>5)&0b000111);
            }
        }munit;
        struct{
            private:
            uint8_t data[2];
            public:
            void operator=(const uint8_t& second){
                this->data[1]=((second/2)&0b11111)+(this->data[1]&0b11100000);
            }
            operator uint8_t()const{
                return (this->data[1]&0b00011111)*2;
            }
        }second;
    };
    //駅コード 参照:https://ja.ysrl.org/atc/station-code.html
    struct Station_code{
        //路線コード
        uint8_t line_code;
        //駅順コード
        uint8_t station_order_code;
    };


    public:
    struct usage_history_data{
        uint8_t device_type;
        uint8_t usage_type;
        uint8_t payment_type;
        uint8_t enter_type;
        TransportIC::Date date;
        TransportIC::Station_code enter_station;
        TransportIC::Station_code exit_station;
        _uint16_l balance;
        uint8_t unknown0;
        _uint16_l process_num;
        uint8_t unknown1;
    };

};

#endif //TRANSPORT_IC_HPP