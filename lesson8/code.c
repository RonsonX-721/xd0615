/*-----------------------------------------------
  ˵����
        ���ܣ���������࣬DS18b20�������������ʾ
        ����12MHz
        ������Χ��1CM - 500CM
        �¶���ʾ��0.0 C - 99.9 C
        ������Χ���ã�5CM - 400CM���������ô洢�趨ֵ��
------------------------------------------------*/
#include "reg52.h" //����ͷ�ļ���һ���������Ҫ�Ķ���ͷ�ļ��������⹦�ܼĴ����Ķ���
#include "18b20.h"
#include "eeprom.h"
#include "delay.h"

#define DataPort P0 //�������ݶ˿� ����������DataPort ����P1 �滻
sbit DUAN=P2^6;//��������ʹ�ܶ˿� ������
sbit WEI=P2^7;//                 λ����

/***************���������Ŷ���******************/
sbit Echo = P3 ^ 3;         //�ز�����
sbit Trig = P1 ^ 5;         //��������

sbit buzzer = P1 ^ 0;    //����������
/*****************��������**********************/
uchar bdata key,key1,lastkey;
bit key1Mark;//���ü���־

/*****************������**********************/ 
uchar EchoTimeH,EchoTimeL;  //�Զ���Ĵ����洢8λ�ز�ʱ��
uint Distance,EchoTime;            //�������룬�ز���ʱ��
uint Distance_Alarm;                                //��������ͨ���Լ��趨����ʼΪ0��
bit succeed_flag;                              //�����ɹ���־

uchar code Table[18] = //�����
{0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};// ��ʾ����ֵ0~9
uchar code WeiMa[4]={0xfe,0xfd,0xfb,0xf7};         //�ֱ��Ӧ��Ӧ������ܵ���,��λ��

uint Temperature,temp;//��ȡ�¶�
bit flag200ms,flag300ms;//��ȡʱ���־

/*****************��ʾ����**********************/
uint DispData;
uchar TempData[4]; //�洢��ʾֵ��ȫ�ֱ���
uchar mod;//��ʾģʽ�л�

/*****************��������**********************/
void delay_20us();                                                                //20us�ӳ�
void CountDat(uint ShowData);                        //��ʾ���ݴ���
void readkey();                                                                                //������ȡ
void CountKey();                                                                        //��ʾɢת
void CSBFunction();                                                                //���������
void Display();                                                                                //�������ʾ����
void Init_Timer0(void);                                                //��ʱ��0��ʼ��

/*------------------------------------------------
                    ������
------------------------------------------------*/
void main (void)
{                          
        float speed = 0;

        buzzer=1;     //��ʼ������������
        Trig=0;       //��������������������
        TMOD|=0x11;    //��ʱ��0����ʱ��1��16λ������ʽ
  IT1=0;        //�͵�ƽ�����ⲿ�ж�
        EX1=0;        //�ر��ⲿ�ж�
  Init_Timer0();
        ISP_IAP_readData(0x2ff0,ReadBuf,2);
        Distance_Alarm = ReadBuf[0] | (ReadBuf[1] << 8);//��ȡ��������ֵ 
        while (1)         //��ѭ��
  {
                CSBFunction();        //��ʼ���
                if(succeed_flag)
    {
                        succeed_flag=0; //������ɹ���־         
                   EchoTime = (EchoTimeH << 8) | EchoTimeL;                //���8λ�ϲ���Ϊ16λ�������
                        //��ʽ˵���� 1��V=(331.4+0.61*T)*D/2000000 CM��2���ز�ʱ��D��us��λ������
                        //������CM��λҪ����20000������Ϊ�˼�С������ת��uchar����/200����/100��
                        speed = (331.4 + 0.61 * (TempData[0]*10+TempData[1])) / 200;
                         Distance = (EchoTime * speed) / 100;     //�������CM
                        CountDat(DispData);//������Ҫ��ʾ���ݵĸ���λ
    }
                if(mod != 2)
                        readkey();//������ȡ,����ʱ��ʾ�¶Ȳ����趨
                CountKey();//������������
                if(flag200ms)
                {
                        flag200ms = 0;
                        temp = ReadTemperature();//
                        Temperature = temp * 6;//С�����ƴ��������м����temp�Ƿ�ֹ�жϵ��������¶�ֵ�쳣
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
                }                                                 //����������300HzƵ��
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
 
 ��ʾ���������ڶ�̬ɨ�������

------------------------------------------------*/
void Display()
{
        static unsigned char i=0;

        DataPort=0;   //������ݣ���ֹ�н�����Ӱ
        DUAN=1;     //������
        DUAN=0;
        
        DataPort=WeiMa[i]; //ȡλ�� 
        WEI=1;     //λ����
        WEI=0;

        if((i == 1) && (mod == 2))        
        {
                DataPort = Table[TempData[i]] & 0xef;//�¶���ʾ��
        }
        else
        {
                DataPort = Table[TempData[i]] ; //ȡ��ʾ���ݣ�����
        }
        DUAN=1;     //������
        DUAN=0;
        
        i ++;
        if(i == 4)
                i = 0;
}
//******************************************************************
//20us����ʱ
void delay_20us()
{   
        uchar bt;
        for(bt=0;bt<20;bt++);
}
/*
������Ҫ��ʾ������
��ڲ�����ShowData
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
/*ɨ��K1 - K3*/
void readkey()                 
{
        static uchar keycnt = 0;
        uchar R0,R1;

        P3 |= 0x07;                                                         //51��Ƭ����Ϊ����ʱ������Ӧλλ1
        R0 = (P3 ^ 0x07) & 0x07; //����ֵת�������߼�
        keycnt ++;           
        if(R0)
        {
                if(keycnt > 1) //�����ӳ٣�����MCU�ٶȽ��е���
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
                key1 = 0;                //�ް������¶�Ϊ0
                keycnt = 0;
        }  
        R1 = key1;
        key1 = key1 & (key ^ key1);//key^key1�жϼ�ֵ��0��1����1��0����&key1��key1��Ϊ0���а������£���0��1��
        key = R1;                
}
/*��ȡ�����󣬸��ݼ�ֵ��������*/
void CountKey()
{
        switch(key1)
        {
                case 0x01:if(!key1Mark){mod = 1;key1Mark = 1;}
                                                        else
                                                        {
                                                                mod = 0;key1Mark = 0;
                                                                WriteBuf[0] = Distance_Alarm & 0xff; WriteBuf[1] = (Distance_Alarm >> 8) & 0xff;
                                                                ISP_IAP_sectorErase(0x2e00);//��������,һ��512�ֽ�         
                                                    ISP_IAP_writeData(0x2ff0,WriteBuf,2);  //д����ֵ�� EEPROM
                                                        }
                        break;
                case 0x02:if(mod == 1){Distance_Alarm += 5;if(Distance_Alarm > 400)Distance_Alarm=400;}                                
                        break;
                case 0x04:if(mod == 1){Distance_Alarm -= 5;if(Distance_Alarm < 5)Distance_Alarm=5;}
                        break;
                default : break;
        }
        switch(0)                 //������ʾģʽɢת
        {
                case 0:DispData = Distance;        //����Ĭ������ʾ��õľ���
                        break;
                case 1:DispData =  Distance_Alarm;        //����ģʽ����ʾ��������
                        break;
                case 2:DispData = Temperature;        //��������ʾ��ǰ�¶�ֵ
                        break;
                default : break;
        }
}
/*���������*/
void CSBFunction()
{
        EA=0;
        Trig=1;
        delay_20us();
        Trig=0;         //����һ��20us�����壬��Trig����
        while(Echo==0); //�ȴ�Echo�ز����ű�ߵ�ƽ,�ߵ�ƽ������ʱ����ǳ������ӷ��䵽���ص�ʱ��
        {
          EX1=1;          //���ⲿ�ж�
                TH1=0;          //��ʱ��1����
          TL1=0;          //��ʱ��1����
          TF1=0;          
          TR1=1;          //������ʱ��1
                EA=1;
        }
        while(TH1 < 255);//�ȴ������Ľ��������65.535���루�����ж�ʵ�֣��������ʱ���ڵȴ��ⲿ�ж�
        {
                TR1=0;          //�رն�ʱ��1
          EX1=0;          //�ر��ⲿ�ж�
        }
}
/*------------------------------------------------
            ��ʱ����ʼ���ӳ���
------------------------------------------------*/
void Init_Timer0(void)
{
        TMOD |= 0x01;          //ʹ��ģʽ1��16λ��ʱ����ʹ��"|"���ſ�����ʹ�ö����ʱ��ʱ����Ӱ��                     
        TH0=(65536-2000)/256;                  //������ֵ 2ms
        TL0=(65536-2000)%256;
        EA=1;            //���жϴ�
        ET0=1;           //��ʱ���жϴ�
        TR0=1;           //��ʱ�����ش�
}
/*------------------------------------------------
                 ��ʱ���ж��ӳ���
------------------------------------------------*/
void Timer0_isr(void) interrupt 1 
{
        static unsigned char Cnt200ms,Cnt300ms;
        TH0=(65536-2000)/256;                  //���¸�ֵ 2ms
        TL0=(65536-2000)%256; 

        Display();       // ���������ɨ��
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
//�ⲿ�ж�1�������жϻز���ƽ���͵�ƽ�����жϣ��õ��ߵ�ƽ�ĳ���ʱ��
INT1_()  interrupt 2   // �ⲿ�ж�
{    
        EchoTimeH = TH1;    //ȡ����ʱ����ֵ
        EchoTimeL = TL1;    //ȡ����ʱ����ֵ
        succeed_flag = 1;  //�óɹ������ı�־
        EX1=0;             //�ر��ⲿ�ж�
}
