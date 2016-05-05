int ledPin=13;//定义数字接口13为LED输出接口
int pin=10;//定义数字检测接口pin0
int val;//定义变量
void setup()
{
  pinMode(ledPin,OUTPUT);//设定数字接口13为输出接口
  Serial.begin(9600);//设置串口波特率为9600kbps
}
void loop()
{
  val=digitalRead(pin);//读取模拟接口的值
  Serial.println(val);//输出模拟接口的值
  if(val==1)//如果Pin10检测的值为高电平，点亮LED
  {
    digitalWrite(ledPin,HIGH);
  }
  else          //如果为低电平熄灭LED
  {
    digitalWrite(ledPin,LOW);
  }
}

