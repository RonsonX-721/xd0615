#define IN1 3 
#define IN2 4
#define IN3 5
#define IN4 6
#define S1 7
#define LT 9
#define BT 8
volatile int pinInterrupt=2;
int count=0;
int num=0;

void myfunc()
{
  Serial.println("OK");
	count=0;
}

void setup()
{
  pinMode(IN1, OUTPUT);//输入1
  pinMode(IN2, OUTPUT);//输入2
  pinMode(IN3, OUTPUT);//输入3
  pinMode(IN4, OUTPUT);//输入4
  pinMode(S1, OUTPUT);//片选1
  pinMode(LT,OUTPUT);//测试
  pinMode(BT,OUTPUT);//消隐
  digitalWrite(LT,HIGH);
  digitalWrite(BT,HIGH);
  digitalWrite(S1,HIGH);
  Serial.begin(9600) ;
  digitalWrite(S1,LOW);
  attachInterrupt( digitalPinToInterrupt(pinInterrupt), myfunc, CHANGE);
}

void loop()
{
    if (count>9)
    {
      count=0;
    }
	num=count-'0';
	digitalWrite(IN1,num&0x1);
	digitalWrite(IN2,(num>>1)&0x1);
	digitalWrite(IN3,(num>>2)&0x1);
	digitalWrite(IN4,(num>>3)&0x1);
  	count++;
	delay(1000);
}