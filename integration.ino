#include <avr/pgmspace.h>
#include <LedControl.h>
#include "font.h"

const int LedPin[3]    = {2,3,4};    // Цветной светодиод
const int ButtonPin[3] = {5,6,7};    // Кнопки
const int ScreenPin[3] = {11,10,12}; // Экран (DIN CS CLK)
const long ButtonInterval = 500;    // Минимальный интервал между нажатиями
const int numDevices = 4;            //Количество соединённых матриц 8х8
const int delta = 150;                // Изменение скорости при нажатии на кнопки

int ButtonStatus = 0;
bool NewString = false;
unsigned long previousMillis[2] = {0,0};
unsigned long currentMillis = 0;
long scrollDelay = 250;               //Задержка сдвига текста в мс

unsigned long bufferLong [16] = {0};

LedControl lc=LedControl(ScreenPin[0],ScreenPin[2],ScreenPin[1],numDevices);                                       

const unsigned char scrollText1[] PROGMEM = {" Лаборатория CISCO"};
const unsigned char scrollText2[] PROGMEM = {" С днем космонавтики!"};
const unsigned char scrollText3[] PROGMEM = {" Электротехника"};

const char* names[]= {
  scrollText1, scrollText2, scrollText3,
};
int NumberOfString = 0;


// Функция возвращает пин нажатой кнопки либо 0.
int Button (){
    for (int i=0; i<3; i++){
    ButtonStatus = digitalRead(ButtonPin[i]);
    if (ButtonStatus == LOW){
      digitalWrite(LedPin[i], LOW);  
      } else {
      digitalWrite(LedPin[i], HIGH);
      return i+ButtonPin[0];
      }
    }
  return 0;  
}

void setup() {
  Serial.begin(9600); // Включаем монитор порта
  
  for (int i=0; i<3; i++){
    pinMode(LedPin[i],    OUTPUT);
    pinMode(ButtonPin[i], INPUT );
    }
  
  for (int x=0; x<numDevices; x++){
    lc.shutdown(x,false);       
    lc.setIntensity(x,3);       
    lc.clearDisplay(x);         
    }
}

void loop() {

  scrollMessage(names[NumberOfString]);

  NewString = false;          
}

//**********************************************************************
//**********************************************************************
void scrollMessage(const unsigned char * messageString) {
    int counter = 0;
    int myChar = 0;
    int CharRange = 0;
    do {
        // read back a char
        myChar =  pgm_read_byte_near(messageString + counter);
         if (myChar < 0x20 || myChar > 0xbf)     // байт не выводится, т.к. либо не включён в шрифт,
         {                                     
           if (myChar == 0xd0 || myChar == 0xd1) // либо это первый байт русских символов
            {
            CharRange = myChar - 0xd0 + 1 ;  // запоминается признак первого байта русских символов
            }
         }
         else if (myChar == 0x80)          //     второй байт русского символа
         {
            myChar = myChar + 49 ;         //     код 0xd180 - буква р в UTF-8
              if (myChar != 0 && CharRange ==2){loadBufferLong(myChar);
              }
              CharRange = 0 ;              // признак обнуляется после вывода второго байта символа             
         }
         else if (myChar == 0x81)
         {
              switch (CharRange) {
                  case 1:                  //     код 0xd081 - буква Ё в UTF-8
                     myChar = myChar - 1 ;
                     break;
                  case 2:                  //     код 0xd181 - буква с в UTF-8
                     myChar = myChar + 49 ;
                     break;
              }
              if (myChar != 0){loadBufferLong(myChar);
              CharRange = 0 ;
              }
         }
         else if (myChar > 0x81 && myChar < 0x90)  //   русские буквы т - я (0xd182 - 0xd18f в UTF-8)
         {
              myChar = myChar + 49 ;
              if (myChar != 0){loadBufferLong(myChar);
              CharRange = 0 ;
              }
         }
         else if (myChar > 0x8f && myChar < 0xc0) 
         {                                       
              switch (CharRange) {
                  case 1:                         //    русские буквы А - Я (0xd090 - 0xd0af в UTF-8)
                     myChar = myChar - 15 ;       //  и русские буквы а - п (0xd0b0 - 0xd0bf в UTF-8)
                     break;
                  case 2:                         //    код 0xd191 - буква ё в UTF-8
                     myChar = myChar + 48 ;
                     break;
              }           
              if (myChar != 0){loadBufferLong(myChar);
              CharRange = 0 ;
              }
         }
         else                             //  английские буквы и прочие символы (0x20 - 0x7f в UTF-8)
         {
              if (myChar != 0){loadBufferLong(myChar);
              CharRange = 0 ;
              }
         }
         counter++;
     }
     while (myChar != 0 && !NewString);
}
// Load character into scroll buffer
void loadBufferLong(int ascii){

    if (ascii >= 0x0020 && ascii <=0x00ff){
        for (int a=0;a<8;a++){                      // Loop 7 times for a 5x7 font
            // Index into character table to get row data
            unsigned long c = pgm_read_byte_near(font5x7 + ((ascii - 0x20) * 9) + a);
            unsigned long x = bufferLong [a*2];     // Load current scroll buffer
            x = x | c;                              // OR the new character onto end of current
            bufferLong [a*2] = x;                   // Store in buffer
        }
           // Index into character table for kerning data
        byte count = pgm_read_byte_near(font5x7 +((ascii - 0x20) * 9) + 8); 
        
        for (byte x=0; x<count;x++){  
            //***************************
            int k = false;
            while (!k){
              currentMillis = millis(); //Обновляем текущее значение времени
                if (currentMillis - previousMillis[1] >= scrollDelay) {
                  previousMillis[1] = currentMillis;     
                  rotateBufferLong();
                  printBufferLong();
                  k = true;
                  }
              int But = Button();
              //----
              if (But != 0) { //Если было нажатие
                if (currentMillis - previousMillis[0] >= ButtonInterval) {
                  previousMillis[0] = currentMillis;
                  Serial.println(But);
                switch (But) {
                  case 5:{
                    if (scrollDelay < 10*delta) scrollDelay += delta;
                    Serial.println(scrollDelay);
                    break;
                    }
                  case 6:{
                    if (scrollDelay >= delta) scrollDelay -= delta;
                    Serial.println(scrollDelay);
                    break;
                    }
                  case 7:{
                    NewString = true;
                    NumberOfString++;
                    if (names[NumberOfString] == '\0') NumberOfString = 0;
                    break;
                  }
                  }
                }
              }     
            }
            //**************************
        }
    }
 }
// Rotate the buffer
void rotateBufferLong(){
    for (int a=0;a<8;a++){                      // Loop 7 times for a 5x7 font
        unsigned long x = bufferLong [a*2];     // Get low buffer entry
        byte b = bitRead(x,31);                 // Copy high order bit that gets lost in rotation
        x = x<<1;                               // Rotate left one bit
        bufferLong [a*2] = x;                   // Store new low buffer
        x = bufferLong [a*2+1];                 // Get high buffer entry
        x = x<<1;                               // Rotate left one bit
        bitWrite(x,0,b);                        // Store saved bit
        bufferLong [a*2+1] = x;                 // Store new high buffer
    }
} 
// Display Buffer on LED matrix
void printBufferLong(){
  for (int a=0;a<8;a++){                    // Loop 7 times for a 5x7 font
    unsigned long x = bufferLong [a*2+1];   // Get high buffer entry
    byte y = x;                             // Mask off first character
    lc.setRow(3,a,y);                       // Send row to relevent MAX7219 chip
    x = bufferLong [a*2];                   // Get low buffer entry
    y = (x>>24);                            // Mask off second character
    lc.setRow(2,a,y);                       // Send row to relevent MAX7219 chip
    y = (x>>16);                            // Mask off third character
    lc.setRow(1,a,y);                       // Send row to relevent MAX7219 chip
    y = (x>>8);                             // Mask off forth character
    lc.setRow(0,a,y);                       // Send row to relevent MAX7219 chip
  }
}
