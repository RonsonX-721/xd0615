/*-----------------------------------------------
  说明：
        功能：超声波测距，DS18b20矫正；数码管显示
        晶振：12MHz
        测量范围：1CM - 500CM
        温度显示：0.0 C - 99.9 C
        警报范围设置：5CM - 400CM（可以永久存储设定值）
------------------------------------------------*/
#include "reg52.h" //包含头文件，一般情况不需要改动，头文件包含特殊功能寄存器的定义
#include "18b20.h"
#include "eeprom.h"
#include "delay.h"

#define DataPort P0 //定义数据端口 程序中遇到DataPort 则用P1 替换
sbit DUAN=P2^6;//定义锁存使能端口 段锁存
sbit WEI=P2^7;//                 位锁存

/***************超声波引脚定义******************/
sbit Echo = P3 ^ 3;         //回波引脚
sbit Trig = P1 ^ 5;         //触发引脚

sbit buzzer = P1 ^ 0;    //蜂鸣器引脚
/*****************按键定义**********************/
uchar bdata key,key1,lastkey;
bit key1Mark;//设置键标志

/*****************测距变量**********************/ 
uchar EchoTimeH,EchoTimeL;  //自定义寄存器存储8位回波时间
uint Distance,EchoTime;            //测量距离，回波总时间
uint Distance_Alarm;                                //报警距离通过自己设定（初始为0）
bit succeed_flag;                              //测量成功标志

uchar code Table[18] = //段码表
{0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};// 显示段码值0~9
uchar code WeiMa[4]={0xfe,0xfd,0xfb,0xf7};         //分别对应相应的数码管点亮,即位码

uint Temperature,temp;//读取温度
bit flag200ms,flag300ms;//获取时间标志

/*****************显示变量**********************/
uint DispData;
uchar TempData[4]; //存储显示值的全局变量
uchar mod;//显示模式切换

/*****************函数声明**********************/
void delay_20us();                                                                //20us延迟
void CountDat(uint ShowData);                        //显示数据处理
void readkey();                                                                                //按键读取
void CountKey();                                                                        //显示散转
void CSBFunction();                                                                //超声波测距
void Display();                                                                                //数码管显示函数
void Init_Timer0(void);                                                //定时器0初始化

/*------------------------------------------------
                    主函数
------------------------------------------------*/
void main (void)
{                          
        float speed = 0;

        buzzer=1;     //初始化蜂鸣器引脚
        Trig=0;       //首先拉低脉冲输入引脚
        TMOD|=0x11;    //定时器0，定时器1，16位工作方式
  IT1=0;        //低电平触发外部中断
        EX1=0;        //关闭外部中断
  Init_Timer0();
        ISP_IAP_readData(0x2ff0,ReadBuf,2);
        Distance_Alarm = ReadBuf[0] | (ReadBuf[1] << 8);//读取警报距离值 
        while (1)         //主循环
  {
                CSBFunction();        //开始测距
                if(succeed_flag)
    {
                        succeed_flag=0; //清测量成功标志         
                   EchoTime = (EchoTimeH << 8) | EchoTimeL;                //与低8位合并成为16位结果数据
                        //公式说明： 1、V=(331.4+0.61*T)*D/2000000 CM；2、回波时间D是us单位，最终
                        //距离以CM单位要除以20000，这里为了减小计算量转成uchar，先/200，再/100；
                        speed = (331.4 + 0.61 * (TempData[0]*10+TempData[1])) / 200;
                         Distance = (EchoTime * speed) / 100;     //算出来是CM
                        CountDat(DispData);//计算需要显示数据的各个位
    }
                if(mod != 2)
                        readkey();//按键读取,警报时显示温度不能设定
                CountKey();//报警距离设置
                if(flag200ms)
                {
                        flag200ms = 0;
                        temp = ReadTemperature();//
                        Temperature = temp * 6;//小数近似处理，采用中间变量temp是防止中断导致最终温度值异常
                        if(Distance < Distance_Alarm)
                        {
                                buzzer = 0;mod = 2;
                        }
                        else
                        {
                                buzzer = 1;
                                if(!key1Mark)
                                        mod = 0;
                        }
                }                                                 //蜂鸣器产生300Hz频率
                if(flag300ms)
                {
                        flag300ms = 0;
                        if(Distance < Distance_Alarm)
                        {
                                buzzer = 1;
                        }
                }
        }
}

/*------------------------------------------------
 
 显示函数，用于动态扫描数码管

------------------------------------------------*/
void Display()
{
        static unsigned char i=0;

        DataPort=0;   //清空数据，防止有交替重影
        DUAN=1;     //段锁存
        DUAN=0;
        
        DataPort=WeiMa[i]; //取位码 
        WEI=1;     //位锁存
        WEI=0;

        if((i == 1) && (mod == 2))        
        {
                DataPort = Table[TempData[i]] & 0xef;//温度显示点
        }
        else
        {
                DataPort = Table[TempData[i]] ; //取显示数据，段码
        }
        DUAN=1;     //段锁存
        DUAN=0;
        
        i ++;
        if(i == 4)
                i = 0;
}
//******************************************************************
//20us短延时
void delay_20us()
{   
        uchar bt;
        for(bt=0;bt<20;bt++);
}
/*
处理需要显示的数据
入口参数：ShowData
*/
void CountDat(uint ShowData)
{
        EA=0;
        TempData[0] =  ShowData / 1000 % 10;
        TempData[1] =  ShowData / 100 % 10;
        TempData[2] =  ShowData / 10 % 10;
        TempData[3] =  ShowData % 10;
        EA=1;
}
/*扫描K1 - K3*/
void readkey()                 
{
        static uchar keycnt = 0;
        uchar R0,R1;

        P3 |= 0x07;                                                         //51单片机作为输入时先置相应位位1
        R0 = (P3 ^ 0x07) & 0x07; //将键值转换成正逻辑
        keycnt ++;           
        if(R0)
        {
                if(keycnt > 1) //用做延迟，根据MCU速度进行调整
                {
                        keycnt = 0;
                         key1 = R0;
                }
                else
                {
                        key1 = key;
                }
        }
        else
        {
                key1 = 0;                //无按键按下都为0
                keycnt = 0;
        }  
        R1 = key1;
        key1 = key1 & (key ^ key1);//key^key1判断键值有0变1，或1变0；再&key1若key1不为0则有按键按下（由0变1）
        key = R1;                
}
/*读取按键后，根据键值处理数据*/
void CountKey()
{
        switch(key1)
        {
                case 0x01:if(!key1Mark){mod = 1;key1Mark = 1;}
                                                        else
                                                        {
                                                                mod = 0;key1Mark = 0;
                                                                WriteBuf[0] = Distance_Alarm & 0xff; WriteBuf[1] = (Distance_Alarm >> 8) & 0xff;
                                                                ISP_IAP_sectorErase(0x2e00);//扇区擦除,一块512字节         
                                                    ISP_IAP_writeData(0x2ff0,WriteBuf,2);  //写警报值到 EEPROM
                                                        }
                        break;
                case 0x02:if(mod == 1){Distance_Alarm += 5;if(Distance_Alarm > 400)Distance_Alarm=400;}                                
                        break;
                case 0x04:if(mod == 1){Distance_Alarm -= 5;if(Distance_Alarm < 5)Distance_Alarm=5;}
                        break;
                default : break;
        }
        switch(0)                 //数据显示模式散转
        {
                case 0:DispData = Distance;        //正常默认下显示测得的距离
                        break;
                case 1:DispData =  Distance_Alarm;        //设置模式下显示警报距离
                        break;
                case 2:DispData = Temperature;        //警报下显示当前温度值
                        break;
                default : break;
        }
}
/*超声波测距*/
void CSBFunction()
{
        EA=0;
        Trig=1;
        delay_20us();
        Trig=0;         //产生一个20us的脉冲，在Trig引脚
        while(Echo==0); //等待Echo回波引脚变高电平,高电平持续的时间就是超声波从发射到返回的时间
        {
          EX1=1;          //打开外部中断
                TH1=0;          //定时器1清零
          TL1=0;          //定时器1清零
          TF1=0;          
          TR1=1;          //启动定时器1
                EA=1;
        }
        while(TH1 < 255);//等待测量的结果，周期65.535毫秒（可用中断实现），在这段时间内等待外部中断
        {
                TR1=0;          //关闭定时器1
          EX1=0;          //关闭外部中断
        }
}
/*------------------------------------------------
            定时器初始化子程序
------------------------------------------------*/
void Init_Timer0(void)
{
        TMOD |= 0x01;          //使用模式1，16位定时器，使用"|"符号可以在使用多个定时器时不受影响                     
        TH0=(65536-2000)/256;                  //给定初值 2ms
        TL0=(65536-2000)%256;
        EA=1;            //总中断打开
        ET0=1;           //定时器中断打开
        TR0=1;           //定时器开关打开
}
/*------------------------------------------------
                 定时器中断子程序
------------------------------------------------*/
void Timer0_isr(void) interrupt 1 
{
        static unsigned char Cnt200ms,Cnt300ms;
        TH0=(65536-2000)/256;                  //重新赋值 2ms
        TL0=(65536-2000)%256; 

        Display();       // 调用数码管扫描
        Cnt200ms ++;
        if(Cnt200ms == 100)
        {
                Cnt200ms = 0;
                flag200ms = 1;
                Cnt300ms ++;
                if(Cnt300ms == 3)
                {
                        Cnt300ms = 0;
                        flag300ms = 1;        
                }
        }
}
//***************************************************************
//外部中断1，用做判断回波电平，低电平引发中断，得到高电平的持续时间
INT1_()  interrupt 2   // 外部中断
{    
        EchoTimeH = TH1;    //取出定时器的值
        EchoTimeL = TL1;    //取出定时器的值
        succeed_flag = 1;  //置成功测量的标志
        EX1=0;             //关闭外部中断
}
