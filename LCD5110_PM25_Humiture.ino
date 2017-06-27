#include <dht11.h>       //引用温湿度传感器库文件
dht11 DHT11;
#define DHT11PIN 10      //定义温湿度传感器引脚为D10    
#include <nokia5110.h>   //引用5110LCD库文件
int flash=0;
int measurePin = 0;      // 空气传感器模拟输入，模拟口0,
int max_dust;
 
//以下变量为空气传感器使用
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;                
 
//5510LCD定义(宽84像素，高48像素)...
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;
static nokia5110 lcd(3,4,5,6,7);
 
// A custom "degrees" symbol...
static const byte DEGREES_CHAR = 1;
static const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };
 
// 定义温度计图标(10x2)
static const byte THERMO_WIDTH = 10;
static const byte THERMO_HEIGHT = 2;
static const byte thermometer[] = { 0x00, 0x00, 0x48, 0xfe, 0x01, 0xfe, 0x00, 0x02, 0x05, 0x02,
                                    0x00, 0x00, 0x62, 0xff, 0xfe, 0xff, 0x60, 0x00, 0x00, 0x00};
                                     
//定义中文字体的宽和高
static const byte chinese_WIDTH = 12;  //定义中文字体宽度（12像素）
static const byte chinese_HEIGHT = 2;  //定义中文字体高度（这个2不是像素，是2个英文字符高度，实际2*8=16像素）
 
 
//定义湿度图标      
static const byte shui[] = {0x00,0x00,0x00,0x0E,0x12,0x22,0xBC,0x80,
      0x8E,0x12,0x22,0x3C,0x00,0x00,0x00,0x00,
      0x00,0x03,0x06,0x08,0x08,0x09,0x0E,0x00}; 
 
//优
static const byte you[] = {0x20,0x10,0xFC,0x0B,0x08,0x08,0xFF,0x08,
      0xF9,0x0A,0x08,0x08,0x00,0x00,0x0F,0x08,
      0x04,0x03,0x00,0x00,0x07,0x08,0x08,0x0E}; 
//良      
static const byte liang[] = {0x00,0x00,0xFC,0x54,0xD5,0x56,0x54,0x54,
      0xFE,0x84,0x00,0x00,0x00,0x00,0x0F,0x04,
      0x04,0x03,0x02,0x05,0x04,0x08,0x08,0x08}; 
//好
static const byte hao[] = {0x08,0x02,0x08,0x44,0x0F,0xA8,0xF8,0x10,
	0x08,0x68,0x0F,0x86,0x00,0x00,0x01,0x00,
	0x41,0x02,0x41,0x01,0x47,0xFE,0x49,0x00};
//中	  
static const byte zhong[] = {0x00,0x00,0x00,0x00,0x0F,0xF0,0x08,0x20,
	0x08,0x20,0x08,0x20,0x08,0x20,0xFF,0xFF,
	0x08,0x20,0x08,0x20,0x08,0x20,0x08,0x20};
//差      
static const byte cha[] = {0x44,0x54,0x54,0xD5,0x76,0x5C,0x54,0x56,
      0x55,0x54,0x64,0x44,0x04,0x0A,0x09,0x09,
      0x09,0x09,0x0F,0x09,0x09,0x09,0x08,0x08};      
 
//电池电量不同图标     
static const byte vcc_vol_5[] =  {0x00,0x00,0xFE,0xFF,0xFF,0xFE,0x00,0x00}; //电池满
static const byte vcc_vol_4[] =  {0x00,0x00,0xFE,0xFD,0xFD,0xFE,0x00,0x00};
static const byte vcc_vol_3[] =  {0x00,0x00,0xFE,0xF9,0xF9,0xFE,0x00,0x00};
static const byte vcc_vol_2[] =  {0x00,0x00,0xFE,0xE1,0xE1,0xFE,0x00,0x00};
static const byte vcc_vol_1[] =  {0x00,0x00,0xFE,0x81,0x81,0xFE,0x00,0x00};  
static const byte vcc_vol_0[] =  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //电池空
 
//读取vcc电压
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}
 
void setup() {
  Serial.begin(9600,SERIAL_8N1);   //将串口波特率设为9600，数据位8，无校验，停止位1
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  delay(2000);     //延迟2秒，等待传感器初始化完成
}
 
void loop() {
    float vcc_vol=readVcc()/1000;                //将读取的vcc电压mv转换为v
   // Serial.println(vcc_vol,2);
    voMeasured = analogRead(measurePin);         // 读取空气传感器模拟值
    calcVoltage = voMeasured * (5.0 / 1024.0);   //将模拟值转换为电压值
    dustDensity = calcVoltage*0.2*1000;          //将电压值转换为粉尘密度输出单位ug/m3
 
  float temp,humi;
  static byte xChart = LCD_WIDTH;
  int chk = DHT11.read(DHT11PIN);  //检查DHT传感器的状态  
  switch (chk) {     
  case 0:       
   humi = DHT11.humidity;       
   temp = DHT11.temperature;       
    break;     
  case -1:       
    lcd.print("Checksum error"); 
    break;     
  case -2:       
    lcd.print("Time out error"); 
    break;     
  default:       
    lcd.print("Unknown error"); 
    break;   
  }  
   
  //显示温度
  lcd.setCursor(0, 0);     //定位到LCD左上角
  lcd.drawBitmap(thermometer, THERMO_WIDTH, THERMO_HEIGHT);  //画温度计图标
  lcd.print(temp, 1);      //显示温度
  lcd.print("C");          //显示温度单位C
  //显示湿度
  lcd.setCursor(50, 0);    //定位到LCD第50列，0行
  lcd.drawBitmap(shui, chinese_WIDTH, chinese_HEIGHT);   //画湿度图标
  lcd.print(" ");          
  lcd.print(humi, 0);      //显示湿度
  //显示PM值
  lcd.setCursor(10, 2);    //定位到LCD第10列,16行(这里的2指的是2个英文字符高，2*8=16)
  lcd.print("PM:");
  lcd.print(dustDensity, 1); //显示PM值
  
  lcd.setCursor(72, 2);                //定位到LCD第72列,16行(这里的2指的是2个英文字符高，2*8=16)
  if (xChart >= LCD_WIDTH-13) {        //图形X坐标超过(屏幕宽度-13)的话，重置为1
    xChart = 1;
    if (dustDensity < 75 ){            //根据PM浓度显示不同的汉字
      lcd.drawBitmap(you, chinese_WIDTH, chinese_HEIGHT);     //优
      max_dust=75;                      //根据PM浓度调整图形的Y轴最大值
    }
    if (dustDensity >= 70 && dustDensity <150){
       lcd.drawBitmap(liang, chinese_WIDTH, chinese_HEIGHT);//良
       max_dust=150;
    }
    if (dustDensity >= 150 && dustDensity < 300  ){
       lcd.drawBitmap(hao, chinese_WIDTH, chinese_HEIGHT);//好
       max_dust=300;
    }
	if (dustDensity >= 300 && dustDensity < 1000  ){
       lcd.drawBitmap(hao, chinese_WIDTH, chinese_HEIGHT);//中
       max_dust=1000;
    }
    if (dustDensity >= 1000 ){
      lcd.drawBitmap(cha, chinese_WIDTH, chinese_HEIGHT);//差
      max_dust=3000;
    }
  }
 
  //画PM实时柱状图  
  lcd.setCursor(xChart, 9);       //定位柱状图的起始位置
  lcd.drawColumn(3, map(dustDensity,0,max_dust, 0, 24));  // 将PM数值映射为数值0~24（24是图形的高度，3行3*8=24）
  lcd.drawColumn(3, 0);           // 将柱状图下一列先清空，避免与新数据混淆
   
  xChart++;
 
//根据VCC电压判断电池电量，并显示不同图标
lcd.setCursor(74, 5);       //定位电池电量图标的起始位置
if (vcc_vol >=4.5 ){
   lcd.drawBitmap(vcc_vol_5, 8, 8);
}
 
if (vcc_vol <4.5 && vcc_vol>=4 ){
   lcd.drawBitmap(vcc_vol_4, 8, 8);
}
 
if (vcc_vol <4 && vcc_vol>=3.5 ){
   lcd.drawBitmap(vcc_vol_3, 8, 8);
}
 
if (vcc_vol <3.5 && vcc_vol>=3 ){
   lcd.drawBitmap(vcc_vol_2, 8, 8);
}
 
if (vcc_vol <3 ){                  //当VCC电压小于3v时图标闪烁，电池电量不足警告
  switch (flash){                
    case 0:  
         lcd.drawBitmap(vcc_vol_0, 8, 8);
         flash=1;
         break;
    case 1:
         lcd.drawBitmap(vcc_vol_1, 8, 8);
         flash=0;
         break;
       }     
}
 
delay(500);
}
