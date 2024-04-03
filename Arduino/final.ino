#include<stdio.h>
#include<string.h>
#define PIN_trig  A1
#define PIN_echo  A2
#include <PS2X_lib.h>

volatile int ai_mode;
volatile int dianji_mode;
volatile int ai_mode_bak;
volatile int dianji_mode_bak;
volatile uint8_t ps2_lx;
volatile uint8_t ps2_ly;
volatile uint8_t ps2_rx;
volatile uint8_t ps2_ry;
volatile int car_speed;
volatile int car_left;
volatile int car_left_bak;
volatile int car_right;
volatile int car_right_bak;
volatile long systick_ms;
volatile long nextTime;
volatile long distance;
volatile int dis_hold;
volatile int count_r;
volatile int count_d;
volatile int count_m;
volatile boolean debug_mode;
volatile long systick_ms_bizhang;


#define  PIN_nled   13              //宏定义工作指示灯引脚
#define  PIN_beep   4               //蜂鸣器引脚定义
#define PIN_dianji 7                //滚筒电机定义引脚

#define nled_on() {digitalWrite(PIN_nled, LOW);}
#define nled_off() {digitalWrite(PIN_nled, HIGH);}

#define beep_on() {digitalWrite(PIN_beep, HIGH);}
#define beep_off() {digitalWrite(PIN_beep, LOW);}

#define dianji_on() {digitalWrite(PIN_dianji, HIGH);}
#define dianji_off() {digitalWrite(PIN_dianji, LOW);}


char s1[100];
int i=0;
char leng[2];
int j = 0;
char information[20];
double direc[2] = {0, 0};
int flag;

//led灯初始化
void setup_nled() {
    pinMode(PIN_nled,OUTPUT); //设置引脚为输出模式
    nled_off();
}

//蜂鸣器初始化
void setup_beep() {
    pinMode(PIN_beep,OUTPUT);
    beep_off();
}


//功能介绍：LED灯闪烁，每秒闪烁一次
void loop_nled() {
    static u8 val = 0;
    static unsigned long systick_ms_bak = 0;
    if(millis() - systick_ms_bak > 500) {
      systick_ms_bak = millis();
      if(val) {
        nled_on();
      } else {
        nled_off();
      }
      val = !val;
    }
}


#define PS2_DAT 12
#define PS2_CMD A0
#define PS2_CS A3
#define PS2_CLK 11
PS2X ps2x;
void setup_PS2() {
  ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_CS, PS2_DAT, true, true);
  Serial.println("setup_PS2_success");
}

void loop_PS2() {
  if (millis() - systick_ms > 50) {
    systick_ms = millis();
    ps2x.read_gamepad();
    ps2_lx = ps2x.Analog(PSS_LX);
    ps2_ly = ps2x.Analog(PSS_LY);
    ps2_rx = ps2x.Analog(PSS_RX);
    ps2_ry = ps2x.Analog(PSS_RY);
    if (millis() < 3000) {
      ps2_lx = 128;
      ps2_ly = 128;
      ps2_rx = 128;
      ps2_ry = 128;
      return;
    }
    handle_button_press();
    handle_button_release();
    if (ps2_lx != 255 || (ps2_ly != 255 || (ps2_rx != 255 || ps2_ry != 255))) {
      handle_rock();
    }
  }
}

char cmd_return_tmp[64];

void loop_AI() {
  if (ai_mode == 1) {
    AI_auto_mode();
  }
}

void handle_rock() {
  car_left = (128 - ps2_ly) * 8;
  car_right = (128 - ps2_ry) * 8;
  if (abs(car_left) < 100) {
    car_left = 0;

  } else if (car_left > 1000) {
    car_left = 1000;
  } else if (car_left < -1000) {
    car_left = -1000;
  }
  if (abs(car_right) < 100) {
    car_right = 0;

  } else if (car_right > 1000) {
    car_right = 1000;
  } else if (car_right < -1000) {
    car_right = -1000;
  }
  if (car_left_bak != car_left || car_right_bak != car_right) {
    car_left_bak = car_left;
    car_right_bak = car_right;
    sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}",6,1500+car_left,7,1500-car_right,8,1500+car_left,9,1500-car_right); //组合指令
    Serial.println(cmd_return_tmp); //解析ZMotor4指令
    sprintf(cmd_return_tmp, "#%03dP%04dT%04d!",10,(int)(((135 + (car_left - car_right) / 30)/270.0)*2000+500),100); //组合指令
    Serial.println(cmd_return_tmp); //解析ZServo指令

  }
}

float checkdistance_A1_A2() {
  digitalWrite(A1, LOW);
  delayMicroseconds(2);
  digitalWrite(A1, HIGH);
  delayMicroseconds(10);
  digitalWrite(A1, LOW);
  float distance = pulseIn(A2, HIGH) / 58.00;
  //delay(10);
  return distance;
}


void handle_button_press() {
  // 麦轮左平移
  if (ps2x.ButtonPressed(PSB_L3)) {
    sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}",6,1500+(-1000),7,1500-1000,8,1500+1000,9,1500-(-1000)); //组合指令
    Serial.println(cmd_return_tmp); //解析ZMotor4指令
    return;
  }
  // 麦轮右平移
  if (ps2x.ButtonPressed(PSB_R3)) {
    sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}",6,1500+1000,7,1500-(-1000),8,1500+(-1000),9,1500-1000); //组合指令
    Serial.println(cmd_return_tmp); //解析ZMotor4指令
    return;
  }
  if (ps2x.ButtonPressed(PSB_START)) {
    dianji_mode = dianji_mode + 1;
    if (dianji_mode > 1) {
    dianji_mode = 0;
    }
    if(dianji_mode == 1)
    {
        dianji_on();
    }
    if(dianji_mode == 0)
    {
      dianji_off();
    }
    for(int i=0;i<=dianji_mode;i++) {
        beep_on();
        delay(100);
        beep_off();
        delay(100);
    }return;
  }
  if (ps2x.ButtonPressed(PSB_SELECT)) {
    ai_mode = ai_mode + 1;
    if (ai_mode > 1) {
      ai_mode = 0;
    }
    if(ai_mode == 0)
    {
     sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}",6,1500+0,7,1500-0,8,1500+0,9,1500-0); //组合指令
     Serial.println(cmd_return_tmp); //解析ZMotor4指令
    }
    for(int i=0;i<=ai_mode;i++) {
        beep_on();
        delay(100);
        beep_off();
        delay(100);
    }return;
  }
}

void handle_button_release() {
  if (ps2x.ButtonReleased(PSB_L3)) {
    sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}",6,1500+0,7,1500-0,8,1500+0,9,1500-0); //组合指令
    Serial.println(cmd_return_tmp); //解析ZMotor4指令
    return;
  }
  if (ps2x.ButtonReleased(PSB_R3)) {
    sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}",6,1500+0,7,1500-0,8,1500+0,9,1500-0); //组合指令
    Serial.println(cmd_return_tmp); //解析ZMotor4指令
    return;
  }
}

void AI_auto_mode() {
  dis_hold = 30;
  int speed_q_l = 450;
  int speed_q_r = 800;
  int speed_r_l = 450;
  int speed_r_r = 550;
  int speed_c = 150;
  if (millis() - systick_ms_bizhang > 50) {
  systick_ms_bizhang = millis();
  distance = checkdistance_A1_A2();
  if (distance < dis_hold)
  {
    sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}",6,1500+ speed_r_l+speed_c,7,1500+speed_r_r+speed_c,8,1500+speed_r_l+speed_c,9,1500+speed_r_r+speed_c); //组合指令
    Serial.println(cmd_return_tmp); //解析ZMotor4指令
    delay(200);
  }
  else 
  {
    if(direc[1]!=0)
    {
      count_r = 0;
      if (direc[0]<-0.3)
      {
        sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}", 6, 1500 - speed_r_l, 7, 1500 - speed_r_r, 8, 1500 - speed_r_l, 9, 1500 - speed_r_r); //组合指令
        Serial.println(cmd_return_tmp); //解析ZMotor4指令
      }
      else if (direc[0]>0.3)
      {
        sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}", 6, 1500 + speed_r_l, 7, 1500 + 150+speed_r_r, 8, 1500 + speed_r_l, 9, 1500 + 150+speed_r_r); //组合指令
        Serial.println(cmd_return_tmp); //解析ZMotor4指令
      }
      else
      {
        sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}", 6, 1500 + speed_q_l, 7, 1500 - speed_q_r, 8, 1500 + speed_q_l, 9, 1500 - speed_q_r); //组合指令
        Serial.println(cmd_return_tmp); //解析ZMotor4指令
        dianji_on();
        flag = 1;
      }    
    }
    else
    {
        count_d = 0;
        if(flag == 1)
        {
        sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}", 6, 1500 + speed_q_l, 7, 1500 - speed_q_r, 8, 1500 + speed_q_l, 9, 1500 - speed_q_r); //组合指令组合指令
          Serial.println(cmd_return_tmp); //解析ZMotor4指令
          flag = 0;
          while(distance > dis_hold&&count_d<=10)
          {
            delay(200);
            count_d++;
            if (millis() - systick_ms_bizhang > 50)
            {
            systick_ms_bizhang = millis();
            distance = checkdistance_A1_A2();
            }
          }
          dianji_off();
        }
          sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}", 6, 1500, 7, 1500, 8, 1500, 9, 1500); //组合指令
          Serial.println(cmd_return_tmp); //解析ZMotor4指令
          delay(300);
          sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}", 6, 1500 + speed_r_l+speed_c, 7, 1500 + speed_r_r+speed_c, 8, 1500 + speed_r_l+speed_c, 9, 1500 + speed_r_r+speed_c); //组合指令组合指令
          Serial.println(cmd_return_tmp); //解析ZMotor4指令
          delay(200);
          count_r++;
          if(count_r == 33)
          {
            count_r = 0;
            count_m = 0;
            sprintf(cmd_return_tmp, "{#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!#%03dP%04dT0000!}", 6, 1500 + speed_q_l, 7, 1500 - speed_q_r, 8, 1500 + speed_q_l, 9, 1500 - speed_q_r); //组合指令组合指令
            Serial.println(cmd_return_tmp); //解析ZMotor4指令，
           while(distance > dis_hold&&count_m<=5)
          {
            delay(200);
            count_m++;
            if (millis() - systick_ms_bizhang > 50)
            {
            systick_ms_bizhang = millis();
            distance = checkdistance_A1_A2();
            }
          }
          }
    }
  }
  }
}

void camera()
{
  delay(100);
  for (int i=0;i<100;i++)
  {
    s1[i] = '\0';
  }
  for(int i=0;i<20;i++)
  {
    information[i] = '\0';
  }
  for(int i=0;i<2;i++)
  {
    leng[i] = '\0';
  }
  if(Serial.available()==0)
  {
    direc[0] = 0;
    direc[1] = 0;
  }
  else
  {
    while(Serial.available()>0)
    {
        s1[i] = Serial.read();
        i++;
    }
  
    for (j=0;1;j++)
    {
        if (s1[j] == 'l')
        {
            break;
        }
    }
    leng[0] = s1[j+1];
    leng[1] = s1[j+2];
    j = j+3;
    int length1 = (leng[0]-'0')*10 + leng[1]-'0';
//    Serial.println(length1);
    for (i=0;i<length1;i++)
    {
        information[i] = s1[j];
        j++;
    }
    information[i] = '\0';
    
    i = 0;
    char *temp = strtok(information,",");
    while(temp)
    {
      direc[i] = atof(temp);
      temp = strtok(NULL,",");
      i++;
    }
 }
}

void setup(){
  Serial.begin(115200);      //串口初始化
  setup_nled();         //led灯闪烁初始化
  setup_beep();         //蜂鸣器初始化

  ai_mode = 0;
  dianji_mode = 0;
  ai_mode_bak = 0;
  dianji_mode_bak = 0;
  flag = 0;
  ps2_lx = 128;
  ps2_ly = 128;
  ps2_rx = 128;
  ps2_ry = 128;
  car_speed = 0;
  car_left = 0;
  car_left_bak = 0;
  car_right = 0;
  car_right_bak = 0;
  systick_ms = 0;
  nextTime = 0;
  distance = 0;
  dis_hold = 0;
  count_r = 0;
  count_d = 0;
  count_m = 0;
  debug_mode = false;
  setup_PS2();
  for(int i=0;i<3;i++) {
      beep_on();
      delay(100);
      beep_off();
      delay(100);
  }
  systick_ms_bizhang = 0;
  pinMode(A1, OUTPUT);
  pinMode(A2, INPUT_PULLUP);
}
void loop(){
  // 循迹 S1
  // 超声波 S3

  //     管脚定义：
  //     循迹（S1）：A1 A0
  //     超声波（S3）：trig-A3 echo-A2
  //     声音（S4） 2
  //     颜色识别（S6） A5 A4
  //
  //     板载按键 D4
  //
  //     手柄模式：
  //     手柄控制车+6个舵机
  //
  //     智能模式
  //     循迹模式-跟随模式
  //     自由避障-循迹避障
  //     声控循迹-声控夹取
  //     定距夹取-颜色识别
  //     循迹定距-循迹识别
   camera();
   loop_nled();    //切换LED灯状态实现LED灯闪烁
   loop_PS2();
   loop_AI();
}
