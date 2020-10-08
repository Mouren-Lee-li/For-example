#include  "extern.h"


//===============================================================================
//  You can get more infomation from
//    [Menu] -> [Help] -> [Application Note] -> [IC] -> [Register] -> [T16M]
//===============================================================================

////////////////////////////////////////////////////////////////////////////////////////////
//主控：PMS171B-S08  (根据具体IC以及封装修改)
//            _____________________________________
//          -|VDD                             GND |-
//          -|PA7/X1             INT0/CO/AD10/PA0 |-
//          -|PA6/X2      INT1A/CIN-/CIN+/AD9/PA4 |-
//          -|PA5/PRSTB      TM2PWM/CIN0-/AD8/PA3 |-
//           --------------------------------------
//  程序功能：
///////////////////////////////////////////////////////////////////////////////////////////// 
//电池灯对应
//电池电压>194     100%
//194>电池电压>173 75%
//173>电池电压>153 50%
//153>电池电压>143 25%
//电池电压<143     0%
/*延时单位1T(1个时钟周期),延时2000=1ms,一共延时1000ms*/
/****************.delay 2000*1000;******************/
BIT    AD_BAT    :  PA.0;    //电量检测
BIT    LIGHT_M   :  PA.3;    //主灯  
BIT    LIGHT_S   :  PA.4;    //侧灯
BIT    LED_CTL3  :  PA.5;    //指示灯控制
BIT    LED_CTL2  :  PA.6;    //指示灯控制
BIT    LED_CTL1  :  PA.7;    //指示灯控制

typedef byte  u8;
typedef word  u16;
typedef EWORD u24;
typedef DWORD u32;

/*****按键返回变量******/
typedef struct {
   u8   KEY;
   u8   KEY_Flag;//按键按下标志位
   u16  key_time; //按键单击计时
   u16  key_double_time;//按键双击计时
   u8   short_key_flag;//按键模式标志位
   u8   long_key_flag;
   u8   double_key_flag;
   u8   short_key_press;//按键键值标志位
   u8   long_key_press;//按键进入长按模式
   u8   double_key_press;
} KEY_PARA;
KEY_PARA key_data;
/*******END*********/

/*****工作模式标志位******/
typedef struct {  
    u8  Mod;
    u8  pwm;
    u8  control_time;
    u8  PWM_adjust;
    u8  Mod_LED;
    u16 V_Bat;
    u8  Key_long_Value;
	u32 Mod_time;
    u32 Timing;
} Mod_0_6;
Mod_0_6 Mod_data;
/*******END*********/

/******待机模式*******/
#define HIGH 1
#define LOW  0

#define DISABLE 0
#define ENABLE 1

#define EMPTY 0
#define FULL 1

#define ON 1
#define OFF 0

#define LOW_POW_TIMING_TMR 5000
/********END***********/



/******电量指示灯*******/
void LED_BAT1(void);
void LED_BAT2(void);
void LED_BAT3(void);
void LED_BAT4(void);
void LED_BATclose(void);
/********END***********/


/******功能函数*******/
void ChipInit(void); //初始化程序
void ADC_Init(void);
void Delay(void);
void GetBattery(void);
void MainLightCtrl(void); //主灯控制程序
void MainLightTurnOff(void); //关主灯
void MainLightBlink(void); //主灯闪烁,频率2Hz
/********END**********/



/************************************************************************************/
/*							  16位计数定时器T16 								    */
/************************************************************************************/
void T16_init(void) {
    $ T16M    IHRC, /1, BIT11;  //理论上应该用BIT11，但周期不对，用BIT12才符合要求    //用(IHRC/1)当 Clock Source，每 2^11 次使 INTRQ.T16=1。即周期为(2^11/16M)s = (2^11/16)us = 128us
  // 每次 T16 递增的时间      =   16MHz / 4 = 4 MHz = 0.25uS。
  // INTRQ.T16 的触发时间     =   2^16 * 0.25 uS = 16,384 uS。

//  INTEN.PA0 = 1; //开启PA0中断
 // INTEGS = 0; //跳变沿触发
  INTEN.T16 = 1; //使能16位定时器
  INTRQ = 0; //清除所有中断标志
}
/************************************************************************************/


void temp_init(void)
{
   /*******开关变量初始化*******/
   key_data.KEY_Flag = 0;//按键按下标志位
   key_data.key_time = 0; //按键单击计时
   key_data.key_double_time = 0;//按键双击计时
   key_data.short_key_flag = 0;//按键模式标志位
   key_data.long_key_flag = 0;
   key_data.double_key_flag = 0;
   key_data.short_key_press = 0;//按键键值标志位
   key_data.long_key_press = 0;
   key_data.double_key_press = 0;
   /*******工作模式变量初始化*******/
   Mod_data.Mod = 0;
   Mod_data.Mod_time = 0;
   Mod_data.Mod_LED = 0;//电池灯方式
   Mod_data.V_Bat = 0;
   Mod_data.pwm = 0;
   Mod_data.PWM_adjust = 0;
   Mod_data.Key_long_Value = 0;//模式4标志位
   Mod_data.Timing = 0;//模式计时
   Mod_data.control_time = 0;
}
/**********按键处理函数***********/
void keyScan(void) {
    if (key_data.KEY == 0) {
	.delay 2000*100;//100ms
       if (key_data.KEY == 0) {
           key_data.KEY_Flag = 1;
         }
    }else {
            key_data.KEY_Flag = 2;
         }
}

void KeyFuncAnls(void) 
{
        if (key_data.KEY_Flag == 1)  //当按键按下
        {
            if(key_data.short_key_flag == 0)//如果短按标志值为0
            {
                key_data.short_key_flag = 1;//开始第一次按键键值扫描
                key_data.key_time = 0;//按键按下计时变量清0，开始计时，1ms加1
            }
            else if(key_data.short_key_flag == 1)
            {
                    if (key_data.key_time >= 5500)
                    {
                        key_data.long_key_press = 1;
                        key_data.short_key_flag=0;
                    }
              }
        }
        if(key_data.KEY_Flag == 2)//当按键抬起
        {
			Mod_data.Key_long_Value = 0;
            if(key_data.short_key_flag == 1)//当按键抬起后，此标志如果为1，说明是短按不是长按
            {
                key_data.short_key_flag = 0;
                if(key_data.double_key_flag == 0)
                {
                    key_data.double_key_flag = 1;//按键双击标志置1，等待确认按键是否为双击
                    key_data.key_double_time = 0;//开始计时，同样1ms加1
                }
                else
                {
                    if(key_data.key_double_time < 2400)//第一次短按发生后，在300ms内，发生第二次短按，则完成
					//一次双击，刷新键值
                    {
                        key_data.double_key_flag = 0;
                        key_data.double_key_press = 1;
                    }
                }
            }
            else if((key_data.double_key_flag == 1))
            {
                if(key_data.key_double_time > 2400)
                {
                    key_data.double_key_flag = 0;
                    if (key_data.long_key_press == 2) {
                        key_data.long_key_press = 0;
                    } else {
                        key_data.short_key_press = 1;
                    }
                }
            }
           /* else if (key_data.Key_long_Value == 1) //长按松手检测
            {
                    key_data.Key_long_Value = 0;
                    key_data.long_key_press = 1;
            }*/
        }
}
/***************END****************/

/**********软件延迟模式***********/
void Delay (void) {   //10s  390000   1s  65000 
       	u16 tt = 3000;
        while(--tt)	{
                NULL;
        }
}
/*void Delay5Ms (void) {  //5ms
		u16 tt = 325;    
        while(--tt)	{
                NULL;
        }
}*/
void Delay2Ms (void) {
        u8 tt = 130;    
        while(--tt)	{
                NULL;
        }
}
/***************END**********/

/**********省电模式***********/
void Sleep_Mod(void) 
{
   //休眠模式
}
/***************END**********/

/**********电量指示灯函数***********/
void LED_BAT1(void) {
  LED_CTL1 = 1;
  LED_CTL2 = 0;
  LED_CTL3 = 1;
}

void LED_BAT2(void) {
  LED_CTL1 = 0;
  LED_CTL2 = 1;
  LED_CTL3 = 0;
}

void LED_BAT3(void) {
  LED_CTL1 = 1;
  LED_CTL2 = 1;
  LED_CTL3 = 0;
}

void LED_BAT4(void) {
  LED_CTL1 = 0;
  LED_CTL2 = 0;
  LED_CTL3 = 1;
}
void LED_BATclose(void) {  //灯全灭
  LED_CTL1 = 1;
  LED_CTL2 = 1;
  LED_CTL3 = 1;
}

void LED_SHOW(void)
{
 
    if(Mod_data.Mod_LED >= 1)
    {
        LED_BAT1();//第1个灯亮
        Delay2Ms();
    }
     if(Mod_data.Mod_LED >= 2)
    {
         LED_BAT2();//第2个灯亮
         Delay2Ms();
    }
 
     if(Mod_data.Mod_LED >= 3)
    {
         LED_BAT3();//第3个灯亮
         Delay2Ms();
    }
    if(Mod_data.Mod_LED >= 4)
    {
        LED_BAT4();//第4个灯亮
        Delay2Ms();
    }  
}
/********END***********/

/**********低功耗***********/
/*void LowPow(void) {
//退出功能则进入低功耗
  if ((!ueLowPowAltTm)) {
    //关灯
    .delay 1000;
    //清看门狗
    wdreset;
    //IHRC ->ILRC,关看门狗
    CLKMD = 0xf4;
    //禁用IHRC
    CLKMD.4 = 0;
    while (1) {
      //低功耗
      STOPSYS;
      if (KEY_POW == 0) {
        break;
      }
    }
    //ILRC->IHRC ，
    //b7:5@001=IHRC/8,
    //b4@1=IHRC
    //b3@1=模式1
    //b2@ 1=ILRC启动
    //b1@ 1=看门狗开启
    //b0@ 0=Pa5;
    //模式口1；开看门狗
    //CLKMD = 0b001_1_1110;
    // ueLowPowAltTm = LOW_POW_TIMING_TMR;
    //$ padier 0b111_1_1001;
    }
}*/
/**********END***********/
void ADC_Init (void) { //PMS171B只有8位精度
        $ ADCM 8BIT, /16; //PMS171B只有8位精度,4分频(系统时钟为2M)
        $ ADCRGC VDD; //选择参考电压VDD
        $ ADCC  ENABLE  BANDGAP;   //  VDD/255 = 1200/AD_VALUE  (3680 /255 = 1200 /83)

        Delay(); //选择内部参考源后需要延时1ms
        Delay(); //选择内部参考源后需要延时1ms
        Delay(); //选择内部参考源后需要延时1ms
        Delay(); //选择内部参考源后需要延时1ms
}


void GetBattery (void) {
        u16 val;

        $ ADCC	Enable, PA0; //PA0作AD口
        AD_Start = 1;
        while (! AD_Done) NULL;
        val = ADCR; //PMS171B只有8位精度
        key_data.KEY = val;
}

void MathBattery(void) { //AD采集八次取平均值
        u8 i = 8;
        while (i --)
	 {
        GetBattery();
		Mod_data.V_Bat += key_data.KEY;
	}
	//取平均值输出
	Mod_data.V_Bat = Mod_data.V_Bat>>3;	//temp除以8

}
/************************主灯控制************************/
void MainLightCtrl (void) { //主灯控制程序
        TM2CT = 0; //清零定时器
        TM2B = Mod_data.pwm;
        TM2S = 0b0_01_01001; //预分频为4,分频系数9+1=10,因此pwm频率为2M/(256*(4*(9+1)))=195Hz
        TM2C = 0b0001_10_1_0; //使用系统时钟(2M),PA3输出,PWM模式,使能(BIT0为0时)
}
void MainLightTurnOff (void) { //关主灯
        TM2C = 0b0001_00_1_1; //使用系统时钟(2M),PA3输出,PWM模式,禁能(BIT0为1)
}

void MainLightBlink (void) { //主灯闪烁,频率2Hz
        TM2CT = 0; //清零定时器
        TM2B = 0b0111_1111;//Mod_data.PWM_MAX >> 1; //127
        TM2S = 0b0_11_11111; //预分频为64,分频系数60+1=61,因此pwm频率为2M/(256*(64*(30+1)))=3.938Hz
        TM2C = 0b0001_10_1_0; //使用系统时钟(2M),PA3输出,PWM模式,使能
}
/************************END************************/
/**************侧灯控制************************/
/*void S_LightCtrl (void) { //侧灯控制程序
        TM2CT = 0; //清零定时器
        TM2B = Mod_data.pwm;
        TM2S = 0b0_01_01001; //预分频为4,分频系数9+1=10,因此pwm频率为2M/(256*(4*(9+1)))=195Hz
        TM2C = 0b0001_10_1_0; //使用系统时钟(2M),PA4输出,PWM模式,使能(BIT0为0时)
}
void S_LightTurnOff (void) { //关侧灯
        TM2C = 0b0001_00_1_1; //使用系统时钟(2M),PA3输出,PWM模式,禁能(BIT0为1)
}*/
/***************END**********************/
/**********初始化程序***********/
void ChipInit(void) {
  PA = 0B0000_0000;           //PA初始值是0
  PAC = 0B1111_1110;       //输出为1，输入为0此代表PA7,PA6,PA4为输出
  PAPH = 0B1111_1110;         //不使用上拉电阻 PA3输入使用上拉
  $  PADIER = 0B1111_1000;       //PA0作AD使用时禁用数字功能

  ADC_Init();//AD初始化
  T16_init();//定时器初始化
  temp_init();//变量初始化

  //MISC = 0x00; //正常唤醒,启用LVR功能,看门狗为8K ILRC时钟周期
}

/////////////////////按键功能函数///////////////////////////////
void KeyFuncHandle(void)
{
/////////////////////KEY键相应的操作///////////////////////////////
	if(key_data.short_key_press == 1)
	{
        key_data.short_key_press = 0;
        /***************************处理程序******************************/
        Mod_data.control_time = 0;
        if (Mod_data.Mod == 5 || Mod_data.Mod_time >= 43600) {   //大于10s
            if (Mod_data.Mod != 3 && Mod_data.Key_long_Value != 1) {
                Mod_data.Mod = 0;
                Mod_data.Mod_time = 0;
                //进入休眠模式
                LED_BATclose();
               	LIGHT_S = 0 ;
                MainLightTurnOff();
                //S_LightTurnOff();
            }
        } 
        else if (Mod_data.Mod != 1 && Mod_data.Mod != 2) {   //模式1  MOD = 1
            Mod_data.Key_long_Value = 0;
            Mod_data.Mod = 1;
            Mod_data.Mod_time = 0;
            Mod_data.pwm = 0b1111_1111;
            //模式1
			//MainLightTurnOff();
            MainLightCtrl();
        }
        else if (Mod_data.Mod == 1) {                        //模式2  MOD = 2
            Mod_data.Mod_time = 0;
            Mod_data.Mod = 2;

                //模式2
				Mod_data.pwm = 0b0010_1101;
            	MainLightCtrl();

        }
        else if (Mod_data.Mod == 2) {                        //模式3  MOD = 3
            Mod_data.Mod_time = 0;
            Mod_data.Mod = 3;
            //模式3
            MainLightBlink();
        }
        /**************************END******************************/
        key_data.KEY_Flag = 0;
    }
    else if (key_data.double_key_press == 1) {		
		    key_data.double_key_press = 0;
            /************处理程序**************/
                Mod_data.Mod = 5;
                //模式5
               //侧灯开，主灯关
                MainLightTurnOff();
                LIGHT_S = 1;
                //Mod_data.pwm = 0b1111_1111;
                //S_LightCtrl();
                //侧灯
             /***********END**************/
            key_data.KEY_Flag = 0;
	}
    else if(key_data.long_key_press == 1) {
		    key_data.long_key_press = 2;  //按住不用松手模式
             /********处理程序*********/
            if (Mod_data.Mod == 1 || Mod_data.Mod == 2) {
                Mod_data.Mod = 4;
                Mod_data.Key_long_Value = 1;
                Mod_data.Mod_time = 0;
                //模式4
             }
            /***********END*********/
            key_data.KEY_Flag = 0;
    }
}
/****************Mod********************/
void Mod_config(void) 
{
    u8 Mod_Temp = 0;
    /************************模式一***************************/
    if (Mod_data.Mod == 1) {
        if (Mod_data.control_time == 0) {
            Mod_data.Timing = 0;
            if (Mod_data.Timing == 261600) { //一分钟计时
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b1111_1111;
                MainLightCtrl();
                Mod_Temp = 1;
            }
                if (Mod_Temp == 1 && Mod_data.Timing == 3924000) { //15分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0101_0101;
                    MainLightCtrl();
                } else if (Mod_data.Timing == 7848000) { //30分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0100_1011;
                    MainLightCtrl();
                } else if (Mod_data.Timing == 11772000) { //45分钟
                    Mod_data.Timing = 0;
                    Mod_data.control_time = 1;
                    Mod_data.pwm = 0b0100_0001;
                    MainLightCtrl();
                }
        } 
        else if (Mod_data.control_time == 1){
            if (Mod_data.Timing == 31392000) { //120分钟
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0011_1100;
                MainLightCtrl();
                Mod_Temp = 2;
            }
            else if (Mod_Temp == 1 && Mod_data.Timing == 2616000) { //10分钟
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0011_0111;
                MainLightCtrl();
                Mod_data.control_time = 2;
            }
        }
        else if (Mod_data.control_time == 2){
            if (Mod_data.Timing == 47088000) { //180分钟
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0011_0010;
                MainLightCtrl();
                Mod_Temp = 3;
            }
            else if (Mod_Temp == 3 && Mod_data.Timing == 1308000) { //5分钟
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0010_1101;
                MainLightCtrl();
            }
            else if (Mod_Temp == 3 && Mod_data.Timing == 2616000) { //10分钟
                Mod_data.control_time = 0;
                Mod_Temp = 0;
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0010_1000;
                MainLightCtrl();
            }
        }
    } 
     /************************模式二***************************/
    else if (Mod_data.Mod == 2) {
            if (Mod_data.control_time == 0) {
                Mod_data.Timing = 0;
                if (Mod_data.Timing == 15696000) { //一小时
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0010_1011;
                    MainLightCtrl();
                    Mod_Temp = 1;
                }
                if (Mod_Temp == 1 && Mod_data.Timing == 1308000) { //5分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0010_1010;
                    MainLightCtrl();
                } else if (Mod_Temp == 1 && Mod_data.Timing == 1831200) { //7分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0100_1001;
                    MainLightCtrl();
                } else if (Mod_Temp == 1 && Mod_data.Timing == 11772000) { //10分钟
                    Mod_data.Timing = 0;
                    Mod_data.control_time = 1;
                    Mod_data.pwm = 0b0100_1000;
                    MainLightCtrl();
                }
            }
            else if (Mod_data.control_time == 1) {
                if (Mod_data.Timing == 31392000) { //三小时
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0010_0101;
                    MainLightCtrl();
                    Mod_Temp = 2;
                }
                if (Mod_Temp == 2 && Mod_data.Timing == 1308000) { //5分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0010_1010;
                    MainLightCtrl();
                } else if (Mod_Temp == 2 && Mod_data.Timing == 1831200) { //7分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0010_0100;
                    MainLightCtrl();
                } else if (Mod_Temp == 2 && Mod_data.Timing == 11772000) { //10分钟
                    Mod_data.Timing = 0;
                    Mod_data.control_time = 2;
                    Mod_data.pwm = 0b0010_0011;
                    MainLightCtrl();
                }
        }
        else if (Mod_data.control_time == 2) {
                Mod_data.pwm = 0b0011_0111;
                MainLightCtrl();
                if (Mod_data.Timing == 31392000) { //三小时
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0010_0101;
                    MainLightCtrl();
                    Mod_Temp = 3;
                }
                if (Mod_Temp == 3 && Mod_data.Timing == 1308000) { //5分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0010_1010;
                    MainLightCtrl();
                } else if (Mod_Temp == 3 && Mod_data.Timing == 1831200) { //7分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0010_0100;
                    MainLightCtrl();
                } else if (Mod_Temp == 3 && Mod_data.Timing == 11772000) { //10分钟
                    Mod_data.Timing = 0;
                    Mod_data.control_time = 2;
                    Mod_data.pwm = 0b0010_0011;
                    MainLightCtrl();
                }
            }
    }
    /************************模式四***************************/
    else if (Mod_data.Key_long_Value == 1) {   //调光模式
            if (key_data.long_key_press != 0) {
                Mod_data.PWM_adjust ++;
                if (Mod_data.PWM_adjust == 1) {
                    Mod_data.pwm = 0b1111_1111;
                    MainLightCtrl();
                } else if (Mod_data.PWM_adjust == 2) {
                    Mod_data.pwm = 0b1010_0101;
                    MainLightCtrl();
                } else if (Mod_data.PWM_adjust == 3) {
                    Mod_data.pwm = 0b1000_1100;
                    MainLightCtrl();
                } else if (Mod_data.PWM_adjust == 4) {
                    Mod_data.pwm = 0b0111_1111;
                    MainLightCtrl();
                } else if (Mod_data.PWM_adjust == 5) {
                    Mod_data.pwm = 0b0110_0110;
                    MainLightCtrl();
                } else if (Mod_data.PWM_adjust == 6) {
                    Mod_data.PWM_adjust = 0;
                    Mod_data.pwm = 0b0000_1101;
                    MainLightCtrl();
                }.delay 2000 * 500;
            } else {
                //暂时保留
            }
    }
    /************************模式五***************************/
    else if (Mod_data.Mod == 5) {
        /*if (Mod_data.control_time == 0) {
            if (Mod_data.Timing == 261600) { //一分钟计时
                Mod_data.pwm = 0b1111_1111;
                S_LightCtrl();
                if (Mod_data.Timing == 3924000) { //15分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0101_0101;
                    S_LightCtrl();
                } else if (Mod_data.Timing == 7848000) { //30分钟
                    Mod_data.Timing = 0;
                    Mod_data.pwm = 0b0100_1011;
                    S_LightCtrl();
                } else if (Mod_data.Timing == 11772000) { //45分钟
                    Mod_data.Timing = 0;
                    Mod_data.control_time = 1;
                    Mod_data.pwm = 0b0100_0001;
                    S_LightCtrl();
                }
            }
        } 
        else if (Mod_data.control_time == 1){
            if (Mod_data.Timing == 31392000) { //120分钟
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0011_1100;
                S_LightCtrl();
            }
            else if (Mod_data.Timing == 2616000) { //10分钟
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0011_0111;
                S_LightCtrl();
                Mod_data.control_time = 2;
            }
        }
        else if (Mod_data.control_time == 2){
            if (Mod_data.Timing == 47088000) { //180分钟
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0011_0010;
                S_LightCtrl();
            }
            else if (Mod_data.Timing == 1308000) { //5分钟
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0010_1101;
                S_LightCtrl();
            }
            else if (Mod_data.Timing == 2616000) { //10分钟
                Mod_data.control_time = 0;
                Mod_data.Timing = 0;
                Mod_data.pwm = 0b0010_1000;
                S_LightCtrl();
            }
        }*/
    }
    
}

/****************End********************/
/**********按键初始化程序***********/
void key_intersection(void)
{
    keyScan();
    KeyFuncAnls();
    KeyFuncHandle();
}
/////////////////////END///////////////////////////////
void FPPA0(void) {
  .ADJUST_IC  SYSCLK = IHRC/8,IHRC = 16MHz,VDD = 5V; // IHRC/8 = 2MIPS, WatchDog Disable, RAM 0,1 temporary be used
  /*配置IO口为输出模式*/
  ChipInit();
  ENGINT;
  Sleep_Mod();//上电进入休眠模式
  while (1) 
  {
      MathBattery();
      key_intersection();
      Mod_config();
      if (Mod_data.Mod == 1 || Mod_data.Mod == 2 || Mod_data.Mod == 3 || Mod_data.Key_long_Value == 1 || Mod_data.Mod == 5) {
          if (Mod_data.V_Bat >= 194) {
              Mod_data.Mod_LED = 4;
          }
          else if (Mod_data.V_Bat >= 173 || Mod_data.V_Bat < 194) {
              Mod_data.Mod_LED = 3;
          }
          else if (Mod_data.V_Bat >= 153 || Mod_data.V_Bat < 173) {
              Mod_data.Mod_LED = 2;
          }
          else if (Mod_data.V_Bat >= 143 || Mod_data.V_Bat < 153) {
              Mod_data.Mod_LED = 1;
          }
          else if (Mod_data.V_Bat < 143) {
              Mod_data.Mod_LED = 0;
              LED_BAT4();//电量灯闪烁
              .delay 2000 * 500;
              LED_BATclose();
              .delay 2000 * 500;
          }
          LED_SHOW();
      }
  }
}


void Interrupt (void) { //128uS 定时器T2产生一次中断  8为1ms
    pushaf;
 	if (Intrq.T16) { //T16 Trig

        key_data.key_double_time ++;
        key_data.key_time ++;
        if (Mod_data.Mod != 3 && Mod_data.Key_long_Value != 1) {
            Mod_data.Mod_time ++;
        }
        if (Mod_data.Mod == 1 || Mod_data.Mod == 2 || Mod_data.Mod == 5 || Mod_data.Key_long_Value == 1) {
            Mod_data.Timing ++;
        }

    Intrq.T16  =  0;
  }

//  if (INTRQ.PA0) { //PA0产生中断
//    batt_data.hold_time = 0; //适配器检测引脚发生波动即清零计时
//
//    INTRQ.PA0 = 0;   // 清除已经处理过的中断
//  }

  popaf;
}
