#include <KSeries.h>



#include "TimerThree.h"

#include "SerialTransfer.h"



#include <RTClib.h>                 // for RTC

#include <LiquidCrystal.h>         // LCD模組的驅動函式庫

//定義LCD物件對應Arduino的腳位

LiquidCrystal lcd(A7,A6,4,5,6,7);  // LiquidCrystal(R/W, Enable, d4, d5, d6, d7)    

RTC_DS3231 rtc;  

#include "kSeries.h"      //include kSeries Library for CO2 detection

//kSeries K_30( 11, 10); //Initialize a kSeries Sensor with pin 12 as Rx and 13 as Tx

kSeries K_40(11, 10); //Initialize a kSeries Sensor with pin 2 as Rx and 3 as Tx (15, 14)



#include <stdlib.h>               //將字串轉換成數值用

#include <SPI.h>

#include <SdFat.h>  // for SD card

#include <time.h> // for time

              

// SD card attached to SPI bus ==> Mega 2560

// MISO     D12     50

// SCK      D13     52

// MOSI     D11     51

// SD_CS    D10     53

const int SD_CS = 53;  

SdFat SD;  

File dataFile;  



const int ON_SW = 41;                              // Switch input pin

int SW[8] = {33, 35, 37, 39, 32, 34, 36, 38};      // button pin

int out_pin[8] = {22, 24, 26, 28, 9, 3, 2, 8};   // output pin

int sensor_pin[4]= {2, 3, 12, 13}; // Sensor pin

unsigned char D_out[8] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW}; // output value

unsigned char bs[8] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW}; // status of D_out 

unsigned char bsON = LOW;





float CO2max = 2.0;             // switch group control (2%)

unsigned int dly_max = 300;      // 延遲時間 30 sec (0.1 sec * 300 = 30 sec)





unsigned long ms_cnt = 0;       // 計算時間

unsigned int dly_t = 0;         // 計算延遲時間

unsigned int tm = 0;            // 計算時間



String Fname = "Test_";         //檔案的開頭名稱

String FileName = "data_01.csv";  //檔案名稱;



int num_file = 0;               //開啟檔案數

int data_indx = 0;              //資料儲存筆數

unsigned int i_data = 0;        //資料筆數

unsigned int i_File = 0;        //檔案編號

const int max_data = 1800;      //每個檔案儲存資料數是1800筆 (save one hour's data)

unsigned char istep = 0;        //step number of sequence control

unsigned char L_step = 0;       //single step index for LCD display



float CO2 = 0.10, CO2_pre = 0.10; //simulation value for CO2(%)  

float Temp = 0.10, Temp_pre = 0.10; //simulation value for CO2(%) 

unsigned char ictrl = 0;        //step of CO2 control

boolean CO2_flag = true;

boolean Temp_flag = true;

String timeStampData;

String cTime;

SerialTransfer serialTransfer;



void setup() {

    Serial.begin(115200);

    serialTransfer.begin(Serial); // Begin serialTransfer

    float val= 0.0;

    transferData(val); // Send data for testing

//    Serial.println("   AN-216 uses the kSeries.h library");

//---------------------------------          

    pinMode(ON_SW, INPUT);           // D41

    int sensor_pin_len = sizeof(sensor_pin)/sizeof(sensor_pin[0]);

    int sw_len = sizeof(SW)/sizeof(SW[0]);

    int out_pin_len = sizeof(out_pin)/sizeof(out_pin[0]);


    for (int j=0; j< sensor_pin_len; j++) pinMode(sensor_pin[j], INPUT);  //設定D32到D39為輸入

    for (int j=0; j< sw_len; j++) pinMode(SW[j], INPUT);  //設定D32到D39為輸入   

    for (int j=0; j< out_pin_len; j++) pinMode(out_pin[j], OUTPUT); //設定D_out[]為輸出 



    lcd.begin(16,2);                 // 初始化LCD物件的格式為16字、2行

    lcd.clear();                     // 清除LCD螢幕       

    lcd.setCursor(1,0);              // 游標設到LCD第3字、第1行      

    lcd.print("LCD initialize!");    // LCD顯示   

//--------------------------------- 

    if (!SD.begin(SD_CS)) {   

       Serial.println("SD Card failed, or not present. Wait a moment !");

   //    while (!SD.begin(53));  

    ///    return; 

  

    } else {

        Serial.println("SD card is initialized.");  

    };



    if (!rtc.begin()) {

      Serial.println("Couldn't find RTC");

     

    } else {

      Serial.println("RTC is initialized");

    };


    Open_file();                     //開啟一新檔案

    delay(2000); 

    Set_all_states_LOW();            // 全部bs[]設為LOW  

    istep = 0;                       // stop sequence control

//---------------------------------           

    Timer3.initialize(100000);         // 設定中斷時間 100000us = 100ms

    Timer3.attachInterrupt(timerIsr);  // 設定中斷副程式  

}


void loop() { 

    if (CO2_flag == true ) {   // measure CO2 concentration in every 2 sec

    //  CO2 = 10.0 * K_30.getCO2('%') + 0.08;  //CO2 concentration(%)  
      CO2_flag = false;
      Temp_flag= true;

      }

      

    if (Temp_flag == true ) { 
      Temp = 10.0 * K_40.getCO2('%') + 0.08;  //CO2 concentration(%)                                     
      Temp_flag = false;  
      CO2_flag= true;

    };

    DateTime rtcTime = rtc.now(); // Get time from RTC module

    timeStampData= String(rtcTime.unixtime());

    char timeInfo[20];

    sprintf(timeInfo, "%02d:%02d:%02d %02d/%02d/%02d",  rtc.now().hour(), rtc.now().minute(), rtc.now().second(), rtc.now().day(), rtc.now().month(), rtc.now().year()); 

    cTime= String(timeInfo);
     
}



//========= 計時器3中斷副程式 ==========

void timerIsr()

{

    ms_cnt ++;                                // calculation of time (ms)

    dly_t ++;                                 // 計算延遲時間



    if ( (ms_cnt%20) == 0 ) {   // measure CO2 concentration in every 2 sec

      CO2_flag = true;

    } 



    if ((ms_cnt%2) == 0 ) LCD_disp();         // 0.2 sec LCD顯示一次

    Detect_button();                          // Detect push button     

    Sequence_control();                       // Perform sequence control     

    

    if ( bsON == LOW ) {

      all_OFF();                              // 關閉所有閥門  

      Set_all_states_LOW();                   // 全部bs設為LOW

      L_step = 0;                             // step index for LCD display

    }

    tm = 2*(ms_cnt/20);                       // time(sec) for saving data

    if ( (ms_cnt%20) == 0 ) save_data();    // 2 sec 儲存一次資料 

    if ( digitalRead(SW[6])==true & digitalRead(SW[7])==true & bsON==LOW) {

      while ( digitalRead(SW[6]) == HIGH );

      dataFile.close();                            //關閉檔案

      i_data = 0;

      i_File ++;                                   //檔案編號  

      dataFile = SD.open(FileName, FILE_WRITE);

//      while ( !dataFile );            

    }          

}

//------------------------------------

void Detect_button() {

    bsON = digitalRead(ON_SW);   

    if ( digitalRead(SW[0]) == HIGH & bsON == HIGH) {

      while ( digitalRead(SW[0]) == HIGH );

      Set_all_states_LOW();      // 全部bs[]設為LOW 

      istep = 1;                 // start sequence control

      dly_t = 0;                 // start calculating time delay

      L_step = istep;            // step index for LCD  display

    } else if ( digitalRead(SW[1]) == HIGH & bsON == HIGH) {

      while ( digitalRead(SW[1]) == HIGH );

      Set_all_states_LOW();      // 全部bs[]設為LOW 

      istep = 0;                 // stop sequence control

      Step_1_control();          // valves 3 & 4 fully close ---> ON       

      dly_t = 0;                 // start calculating time delay

      L_step = 1;                // step index for LCD display

    } else if ( digitalRead(SW[2]) == HIGH & bsON == HIGH) {

      while ( digitalRead(SW[2]) == HIGH );

      Set_all_states_LOW();      // 全部bs[]設為LOW  

      istep = 0;                 // stop sequence control

      Step_2_control();          // perform step 2 control

      dly_t = 0;                 // start calculating time delay 

      L_step = 2;                // step index for LCD display

    } else if ( digitalRead(SW[3]) == HIGH & bsON == HIGH) {

      while ( digitalRead(SW[3]) == HIGH );

      Set_all_states_LOW();      // 全部bs[]設為LOW 

      istep = 0;                 // stop sequence control

      Step_3_control();          // perform step 3 control      

      dly_t = 0;                 // start calculating time delay  

      L_step = 3;                // step index for LCD display

    } else if ( digitalRead(SW[4]) == HIGH & bsON == HIGH) {

      while ( digitalRead(SW[4]) == HIGH );

      Set_all_states_LOW();      // 全部bs[]設為LOW 

      istep = 0;                 // stop sequence control

      Step_4_control();          // perform step 4 control        

      dly_t = 0;                 // start calculating time delay 

      L_step = 4;                // step index for LCD display

    } else if ( digitalRead(SW[5]) == HIGH & bsON == HIGH) {

      while ( digitalRead(SW[5]) == HIGH );

      Set_all_states_LOW();      // 全部bs[]設為LOW  

      istep = 0;                 // stop sequence control

      Step_5_control();          // perform step 5 control        

      dly_t = 0;                 // start calculating time delay  

      L_step = 5;                // step index for LCD display

    } else if ( digitalRead(SW[6]) == HIGH & bsON == HIGH) {

      while ( digitalRead(SW[6]) == HIGH );

      Set_all_states_LOW();      // 全部bs[]設為LOW  

      istep = 0;                 // stop sequence control 

      all_OFF();                 // power off for all valves     

      dly_t = 0;                 // start calculating time delay

      L_step = 0;                // step index for LCD display

      ms_cnt = 0;                // reset time

      //CO2 = 0.10;                // reset CO2 simulation value

      CO2_pre = 0.10;

      //Temp = 0.10 ;

      Temp_pre = 0.10;

    } else if ( digitalRead(SW[7]) == HIGH & bsON == HIGH) {

      while ( digitalRead(SW[7]) == HIGH );

      Set_all_states_LOW();      // 全部bs[]設為LOW  

      istep = 8;                 // index for CO2 control button

      L_step = 8;                // step index for LCD display  

      ictrl = 2;                 // index for CO2 control loop 

    }

}

//------------------------------------

void Sequence_control() {  

    if ( istep == 1 & bsON == HIGH ) {         

        Step_1_control();          // initialization (Valves 3 & 4 are fully closed.)

        if (dly_t > dly_max ) {

            istep = 2;

            dly_t = 0;

            L_step = istep;            

        }

    } else if ( istep == 2 & bsON == HIGH ) {         

        Step_2_control();          // step 2 control

        if (dly_t > dly_max ) {

            istep = 3;

            dly_t = 0;

            L_step = istep;                         

        }

    } else if ( istep == 3 & bsON == HIGH ) {         

        Step_3_control();          // step 3 control

        if (dly_t > dly_max ) {

            istep = 4;  

            dly_t = 0;   

            L_step = istep;                        

        }

    } else if ( istep == 4 & bsON == HIGH ) {         

        Step_4_control();          // step 4 control

        if (dly_t > dly_max ) {

            istep = 5;    

            dly_t = 0;  

            L_step = istep;                       

        }

    } else if ( istep == 5 & bsON == HIGH ) {         

        Step_5_control();          // step 5 control

        if (dly_t > dly_max ) {

            istep = 2;             // return to step 2 control

            dly_t = 0; 

            L_step = istep;                       

        }

    } else if ( istep == 8 & bsON == HIGH ) {     

        L_step = 8;    

        CO2_control_loop();        // CO2 control loop           

    }

}

//------------------------------------

void CO2_control_loop() {        

    if ( ictrl == 2 ) {

      Set_all_states_LOW();      // 全部bs[]設為LOW  

      Step_2_control();

      if ( CO2 > CO2max ) {      // index for CO2 control loop 

        ictrl = 3;               // CO2 control step

        dly_t = 0;         

      } 

    } else if ( ictrl == 3 ) {   // index for group_1 transisiton control

      Set_all_states_LOW();      // 全部bs[]設為LOW       

      Step_3_control(); 

      if (dly_t > dly_max ) {

        ictrl = 4;        

//        CO2 = 0.10;              // reset CO2 simulation value

//        CO2_pre = 0.10;                              

      }

    } else if ( ictrl == 4 ) {

      Set_all_states_LOW();      // 全部bs[]設為LOW      

      Step_4_control(); 

      if ( CO2 > CO2max ) {      // index for CO2 control loop 

        ictrl = 5;               // CO2 control step

        dly_t = 0;         

      }       

    } else if ( ictrl == 5 ) {  // index for group_2 transisiton control

      Set_all_states_LOW();      // 全部bs[]設為LOW        

      Step_5_control(); 

      if (dly_t > dly_max ) {

        ictrl = 2;   

//        CO2 = 0.10;               // reset CO2 simulation value

//        CO2_pre = 0.10;          

      }

    }

}

//==================================================================

//Group_1: valves 1, 3(open, close), 6&7 ---> out_pin[0] ~ out_pin[3]  

//Group_2: valves 2, 4(open, close), 5&8 ---> out_pin[4] ~ out_pin[7]  

void Step_1_control() {     

      all_OFF();                // power off for all valves 

      valve_ON(1);              // valve 3 fully close ---> ON

      valve_ON(5);              // valve 4 fully close ---> ON   

}

//------------------------------------

void Step_2_control() {      

      all_OFF();                // power off for all valves 

      valve_ON(0);              // valve 3 open ---> ON

      valve_ON(2);              // valve 1 ---> ON

      valve_ON(3);              // valves 6 & 7 ---> ON  

      valve_ON(5);              // valve 4 close ---> ON  

}

//------------------------------------

void Step_3_control() {      

      all_OFF();                // power off for all valves 

      valve_ON(1);              // valve 3 close ---> ON

      valve_ON(4);              // valve 4 open ---> ON

      valve_ON(6);              // valve 2 ---> ON  

}

//------------------------------------

void Step_4_control() {      

      all_OFF();                // power off for all valves 

      valve_ON(1);              // valve 3 close ---> ON

      valve_ON(4);              // valve 4 open ---> ON

      valve_ON(6);              // valve 2 ---> ON  

      valve_ON(7);              // valves 5 & 8 ---> ON  

}

//------------------------------------

void Step_5_control() {        

      all_OFF();                // power off for all valves 

      valve_ON(0);              // valve 3 open ---> ON

      valve_ON(2);              // valve 1 ---> ON  

      valve_ON(5);              // valve 4 close ---> ON  

}

//------------------------------------

void Set_all_states_LOW() {  //全部bs設為false

    for (int j=0; j<=7; j++)  bs[j] = LOW;    

}

//------------------------------------

void valve_ON(int j) {

    digitalWrite(out_pin[j],HIGH);   

    bs[j] = true;

}

//------------------------------------

void valve_OFF(int j) {

    digitalWrite(out_pin[j],LOW);    

    bs[j] = false;

}

//------------------------------------

void all_OFF() {               // power off for all valves 

    for (int j=0; j<=7; j++)  {

        digitalWrite(out_pin[j], LOW);  

        bs[j] = false;   

    }

}

//------------------------------------

void LCD_disp() {

    lcd.clear();                // 清除LCD螢幕

    lcd.setCursor(0,0);         // 游標設到LCD第0字、第1列           

    lcd.print(bs[0]);           // LCD接著顯示bs1數值

    lcd.setCursor(3,0);         // 游標設到LCD第5字、第1列          

    lcd.print(bs[1]);           // LCD接著顯示bs2數值  

    lcd.setCursor(6,0);         // 游標設到LCD第8字、第1列           

    lcd.print(bs[2]);           // LCD接著顯示bs3數值

    lcd.setCursor(9,0);         // 游標設到LCD第11字、第1列          

    lcd.print(bs[3]);           // LCD接著顯示bs4數值 

    lcd.setCursor(12,0);        // 游標設到LCD第13字、第0列          

    lcd.print(CO2);             // LCD接著顯示CO2數值     

    lcd.setCursor(0,1);         // 游標設到LCD第0字、第2列           

    lcd.print(bs[4]);           // LCD接著顯示bs5數值  

    lcd.setCursor(3,1);         // 游標設到LCD第0字、第2列           

    lcd.print(bs[5]);           // LCD接著顯示bs6數值

    lcd.setCursor(6,1);         // 游標設到LCD第5字、第2列            

    lcd.print(bs[6]);           // LCD接著顯示bs7數值    

    lcd.setCursor(9,1);         // 游標設到LCD第8字、第2列            

    lcd.print(bs[7]);           // LCD接著顯示bs8數值              

    lcd.setCursor(12,1);        // 游標設到LCD第11字、第2列

    if (L_step == 8 )  lcd.print("RUN");  // LCD接著顯示ictrl 

    else {

      if (bsON==1)lcd.print("ON");            

      else if (bsON==0)lcd.print("OFF");  

    }

    lcd.setCursor(15,1);        // 游標設到LCD第15字、第2列

    if (L_step == 8 )  lcd.print(ictrl);  // LCD接著顯示ictrl 

    else lcd.print(L_step);          // LCD接著顯示istep數值 

}

//------------------------------------

void Open_file() {              //開啟一新檔案 

    if (SD.exists(FileName)) { 

      SD.remove(FileName);                       //刪除檔案     

    };

    dataFile = SD.open(FileName, FILE_WRITE);    //開新檔案來儲存資料

    if ( !dataFile )  Serial.println("Open a new file!");

}

//------------------------------------



void transferData(float data){

 


    serialTransfer.sendDatum(data);

  

}





void save_data() {                         //儲存數據資料                

    String data; 

    i_data ++;


 //   data += cTime+ ", "+ timeStampData+", " +String(tm) + ",  " + String(CO2); 

    data += cTime+ ", "+ timeStampData+", " +String(tm) + ",  " + String(Temp); 


    transferData(Temp);

    Serial.println(data);

    

  //  dataFile = SD.open(FileName, FILE_WRITE); 

  //  if (dataFile) {             // if the file is available, write to it:

  //    dataFile.println(data);

  //    dataFile.close();

  //    Serial.println(data);

  //  } else  Serial.println("Error opening file.");  

  

}
