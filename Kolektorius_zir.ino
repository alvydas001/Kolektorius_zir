// *****************************************************************************************************
//                                  W W W . S a u l e V i r e . l t
// ********************************************************************************************************                                  
// Projektas pavadinimu "pasidaryk pats mikrprocesorinį karšto vandens saulės kolektoriaus valdiklį".
// Visi failai yra ir čia:- http://fritzing.org/projects/hot-water-solar-controller/
// ********************************************************************************************************
//                              P  A  K  E  I  T  I  M  A  I
// ********************************************************************************************************
// 2013 05 12
// Pridėta galimybė pasirinkti montažinės plokštės versiją. Lietuvoje ir Kinijoje gamintose plokštelėse
// panaudoti skirtingi procesoriaus išvadai. 
// Todėl prieš įrašydami pasirinkite su kuria plokštele procesorius dirbs. Ieškokite eilutės "VERSIJA_1.0"
// 2013 05 05
// ekrane rodoma žvaigždutė, jei įjungta apsauga nuo užšalimo
// 2013 04 27
// Pridėtas siurblio įjungimo ir išjungimo reikšmių spausdinimas
// 2013 04 23 
// pridėtas kintamasis "freezing" apsaugai nuo užšalimo įjungti (freezing = 1) arba išjungti (freezing = 0)
// --------------------------------------------------------------------------------------------------------
// Reikalingos bibliotekos:
// "Dallas Temperature Control" biblioteką rasite čia:
// http://milesburton.com/index.php?title=Dallas_Temperature_Control_Library
// OneWire (Version 2.1) biblioteką  rasite čia:
// http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
// Bibliotekas reikia ispakuoti į c:\arduino-X.x.x\library katalogą. c:\arduino-X.x.x\library\dallas-temperature-control
// katalogą reikia pervardinti į c:\arduino-X.x.x\library\DallasTemperatureControl (be brūkšnelių)
// ****************************************************************************************************************
// 1. Kai temperatūros skirtumas tarp saulės kolektoriaus ir boilerio didesnis kaip 5 laipsniai, įjungiama relė. 
// 	Tempertūra matuojama ir rodoma ekrane nuolat, bet skirtumas tikrinamas kas 15 sekundžių.
// 2. Kai temperatūros skirtumas tarp saulės kolektoriaus ir boilerio mažesnis kaip 2 laipsniai, siurblį valdanti relė išjungiama.
// 	Skirtumas tikrinamas kas 15 seundžių.
// 3. Potenciometru galima užduoti bet kokį skaičių nuo 0 iki 100, jis rodomas ekrane. Kai 3-čio daviklio temperatūra
// 	pasieks nurodytą ribą įsijungs 2-oji relė. Temperatūrai nukritus žemiau nurodytos ribos relė išsijungs. 
// 4. Vietoj dviejų daviklių (išlitavus 2 varžas arba perpovus takelį plokštėje) galima jungti dar vieną 2 kanalų 
// 	relių modulį, o reikiamus daviklius jungti nuosekliai ant vieno laido. Kad valdiklis veiktų po tokių pakeitimų
// 	reikia minimalių programos patobulinimų. 
// 5. Daviklius DS18b20 galima jungti 2 arba 3 laidais. 
// 6. Ekrane rodoma boilerio temperatūra, kolektoriaus temperatūra, temperatūrų skirtumas,
// 	3-čio daviklio temperatūra, potenciometru nustatyta temperatūra, relių būsena.
// 7. Jei kolektoriaus temperatūra mažesnė arba lygi 0, įjungiamas siurblys ir kolektorius apsaugomas nuo užšalimo.	
// *******************************************************************************************************************
// 

#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// užkomentuokite  eilutes nuo "VERSIJA_1.0", jei ant Jūsų polokštelės yra
// www.SauleVire.lt reklama ir užrašas "Order #100010284"

//VERSIJA_1.0
// Čia pirmojoje plokštelės versijoje (gaminta Lietuvoje, geltona spalva) naudojami išvadai 
//#define Pump 11 //Siurblio valdymo relės išvadas
//#define Relay2 12 //Termostato valdymo relės išvadas
//int analogPin =  A0; //daugiasūkio rezistoriaus pajungimo išvadas
// VERSIJA_1.0 versijos pasirinkimo pabaiga

//VERSIJA_1.1
// Čia antrojoje plokštelės versijoje (gaminta Kinijoje, žalia spalva) naudojami išvadai 
#define Pump A0 //Siurblio valdymo relės išvadas
#define Relay2 A1 //Termostato valdymo relės išvadas
int analogPin = A2; ////daugiasūkio rezistoriaus pajungimo išvadas
// VERSIJA_1.1 versijos pasirinkimo pabaiga


#define SERIAL_BAUD_RATE 9600
// Temperatūros jutiklių duomenų laidai prijungti prie Arduino išvadų 8,9,10
#define Collector 8 //Kolektoriaus jutiklio išvadas
#define Boiler 9 //Boilerio jutiklio išvadas
#define Temp_3 10 //termostato jutiklio išvadas


int relayPin = Pump;
int relay2Pin = Relay2;
static long timer_pump=0;
static long timer_Relay2=0;

static long timer_Collector_Error=0;
static long timer_Boiler_Error=0;
//Laiko intervalai
// REQUEST_Pump_Control minimalus siurblio reles veikimo laiko intervalas, pasikeitus temperatūrų reikšmėms
// REQUEST_Relay2_Control minimalus termostato reles veikimo laiko intervalas, pasikeitus temperatūrų reikšmėms
// REQUEST_Collector_Error laiko intervalas veiksmui, jei kolektoriaus temperatūros jutiklis rodo klaidą
// REQUEST_Boiler_Error laiko intervalas veiksmui, jei boilerio temperatūros jutiklis rodo klaidą
#define REQUEST_Pump_Control 10000       // 10000 millis= 10 sekundziu
#define REQUEST_Relay2_Control 10000       // 15000 millis= 15 sekundziu
#define REQUEST_Collector_Error 180000       // 120000 millis= 3 minutės
#define REQUEST_Boiler_Error 10000       // 30000 millis= 30 sekundziu

int freezing = 1; //kintamasis apsaugai nuo užšalimo įjungti (freezing = 1) arba išjungti (freezing = 0)
int Differential_ON=5; //Čia užduodama prie kokio temperatūrų skirtumo įjungiama kolektoriaus siurblio relė
int Differential_OFF=3; //prie tokio skirtumo siurblio relė išjungiama
int Collector_Error, Boiler_Error; // jutiklio būsenos "klaida" žymė
int Collector_Good, Boiler_Good; // jutiklio būsenos "viskas gerai" žymė
float Collector_tempC, Boiler_tempC, Sensor3_tempC; //kintamieji temperatūros matavimų reikšmėms saugoti
float Saved_Collector_tempC, Saved_Boiler_tempC;

byte arrow_up[8]={ //Siurblio įjungimo simbolis (rodyklė į viršų)
B00100,
B01110,
B11111,
B00100,
B00100,
B00100,
B00100,
B00100
};
byte arrow_down[8]={ //Siurblio įšungimo simbolis (rodyklė į žemyn)
B00100,
B00100,
B00100,
B00100,
B00100,
B11111,
B01110,
B00100
};

      // nustatomos reikšmės valdiklio "susikalbėjimui" su jutikliais DS18B20
      OneWire Collector_T(Collector);
      OneWire Boiler_T(Boiler);
      OneWire Temp_T(Temp_3);
      DallasTemperature CollectorSensor(&Collector_T);
      DallasTemperature BoilerSensor(&Boiler_T);
      DallasTemperature TempSensor(&Temp_T);
      //masyvai jutiklių adresams saugoti
      DeviceAddress Collector_Sensor_Address;
      DeviceAddress Boiler_Sensor_Address;
      DeviceAddress Temp_3_Sensor_Address;

// LCD ekrano nustatymai
// prijungimas:
// rs (LCD išvadas 4) to Arduino išvadas 2
// enable (LCD išvadas 6) to Arduino išvadas 3
// LCD išvadas 15 - Arduino išvadas 13
// LCD išvadai d4, d5, d6, d7 - Arduino išvadai 4, 5, 6, 7

int backLight = 10;    // išvadas 13 valdo ekrano apšvietimą
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
// Kintamasis termostato užduotai reikšmei saugoti
int val = 0;
//Programos nustatymų pradžia
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(Pump, OUTPUT);
  pinMode(Relay2, OUTPUT);
  digitalWrite(Pump, HIGH);
  digitalWrite(Relay2, HIGH);
  // Arduino išvadų paruošimas darbui su LCD ekranu:
  pinMode(backLight, OUTPUT);
  pinMode(13, OUTPUT);
  analogWrite(backLight, 200); // Įjungiamas apšvietimas.
  lcd.createChar(1, arrow_down);
  lcd.createChar(2, arrow_up);

  lcd.begin(16, 2);
  // Reklaminio užrašo išvedimas į ekraną (TAI LABAI SVARBU :-).
          lcd.print("www.SauleVire.lt"); 
          Serial.println();
          Serial.println("www.SauleVire.lt");
		Serial.print("Siurblys isijungs, kai temperaturu skirtumas bus - ");  Serial.println(Differential_ON);
		Serial.print("Siurblys issijungs, kai temperaturu skirtumas bus - ");  Serial.println(Differential_OFF);
		if (freezing = 1) {Serial.println("Ijungta kolektoriaus apsauga nuo uzsalimo");}
		else {Serial.println("Kolektoriaus apsauga nuo uzsalimo NEIJUNGTA");}
          delay(5000); //Reklama trunka tik 5 sekundes.
          lcd.clear();
CollectorSensor.begin();
          if (!CollectorSensor.getAddress(Collector_Sensor_Address, 0))
          Serial.println("defective Collector temperaturos Sensor");
          // nustatomas didžiausias jutiklio tikslumas- 12 bitų.
          CollectorSensor.setResolution(Collector_Sensor_Address, 12);
BoilerSensor.begin();
          if (!BoilerSensor.getAddress(Boiler_Sensor_Address, 0))
          Serial.println("defective Boiler temperaturos Sensor");
          // nustatomas didžiausias jutiklio tikslumas- 12 bitų.
          BoilerSensor.setResolution(Boiler_Sensor_Address, 12);
TempSensor.begin();
          if (!TempSensor.getAddress(Temp_3_Sensor_Address, 0))
          Serial.println("defective Temp_3 temperaturos Sensor");
          // nustatomas didžiausias jutiklio tikslumas- 12 bitų.
          TempSensor.setResolution(Temp_3_Sensor_Address, 12);          
} 
//____________________________
//Programos nustatymų pabaiga
//____________________________
void loop() { //Prasideda pagrindinė programa
 Serial.println();
 Serial.print("Siurblys isijungs, kai temperaturu skirtumas bus - ");  Serial.println(Differential_ON);
 Serial.print("Siurblys issijungs, kai temperaturu skirtumas bus - ");  Serial.println(Differential_OFF);
 
 	if (freezing == 1) {lcd.setCursor(14,0);
 	lcd.print("*");}
 //	lcd.write((uint8_t)3);}
		else {Serial.println("Kolektoriaus apsauga nuo uzsalimo NEIJUNGTA");
		lcd.setCursor(14,0);
		lcd.print(" ");
		}
 
 val = (analogRead(analogPin) * 100.0) / 1024; //nuskaitoma kintamu rezistoriumi nustatyta termostato reikšmė ir
lcd.setCursor(7,1);
   lcd.print("N");
   lcd.setCursor(8,1);
   lcd.print(val); //parodoma ekrane
   
  TempSensor.requestTemperatures();
  Sensor3_tempC=TempSensor.getTempC(Temp_3_Sensor_Address);
  Serial.print("Nustatyta termostato reiksme "); Serial.println(val); //spausdinama į terminalą, jei prijungtas kompiuteris
  Sensor3_tempC=TempSensor.getTempC(Temp_3_Sensor_Address);
  if (Sensor3_tempC==-127){lcd.setCursor(10,1);lcd.print("KLAIDA");}
  else {lcd.setCursor(11,1);lcd.print(Sensor3_tempC);}
  Serial.println(Sensor3_tempC);
  if (millis() > timer_Relay2 + REQUEST_Relay2_Control) {
  timer_Relay2 = millis();
   
   
   if (Sensor3_tempC >= val) { Rele_2_ijungta();}
   if (Sensor3_tempC <= val) { Rele_2_isjungta();}
   
 }
 
 
 
 
Serial.print((!CollectorSensor.getAddress(Collector_Sensor_Address, 0)));
if (!CollectorSensor.getAddress(Collector_Sensor_Address, 0)) {
   lcd.setCursor(0,0);
   lcd.noDisplay();
   lcd.print("KLAIDA!");
   Serial.println("Kolektoriaus Temperatura: K_L_A_I_D_A");
   Collector_Error=1;
   delay(333);
   lcd.display();
}
else
{
  CollectorSensor.requestTemperatures(); // Komanda jutikliui parodyti išmatuotą temperatūrą
Serial.print("Kolektoriaus Temperatura: ");
Collector_tempC=CollectorSensor.getTempC(Collector_Sensor_Address);
if (Collector_tempC==-127.00){
  lcd.setCursor(0,0);
  lcd.noDisplay();
  lcd.print("KLAIDA!");
  Serial.println(" K L A I D A !");
  Collector_Error=1;
  delay(333);
  lcd.display();
}
else
{
Serial.println(Collector_tempC);
  lcd.setCursor(0,0);
  lcd.print("K");
  if (Collector_tempC >= 0) lcd.print(" "); 
  else lcd.setCursor(1,0);
  lcd.print(Collector_tempC);
  lcd.setCursor(9,0);
  if (Collector_tempC-Boiler_tempC >=0) lcd.print(" ");
  lcd.print(Collector_tempC-Boiler_tempC);
  Collector_Good=1;
  }}

  if (!BoilerSensor.getAddress(Boiler_Sensor_Address, 0)) {
  lcd.setCursor(0,1);
  lcd.noDisplay();
  lcd.print("KLAIDA!"); 
  Serial.println("Boilerio Temperatura: K_L_A_I_D_A");
  Boiler_Error=1;
  delay(333);
  lcd.display();
}
else{
BoilerSensor.requestTemperatures();
Serial.print("Boilerio temperatura: ");
Boiler_tempC=BoilerSensor.getTempC(Boiler_Sensor_Address);
if (Boiler_tempC==-127.00){
  lcd.setCursor(0,1);
  lcd.noDisplay();
  lcd.print("KLAIDA!");
  Serial.println(" K L A I D A !");
  delay(333);
  lcd.display();
  Boiler_Error=1;
}
else{
Serial.println(Boiler_tempC);
  lcd.setCursor(0,1);
  lcd.print("B");
  lcd.setCursor(1,1);
  lcd.print(Boiler_tempC);
  Boiler_Good=1;
}}
//__________________________________________________
//Tikrinama kokia turį būti siurblio būsena. Pradžia
//--------------------------------------------------
//kintamasis REQUEST_Pump_Control nurodo kas kiek laiko tai daryti
if (millis() > timer_pump + REQUEST_Pump_Control) {
    timer_pump = millis();
if (Boiler_Good==1){
if (Collector_Good==1){ 
//tikrinama at kolektorius nėra šiltesnis už boilerį	
if (Collector_tempC - Boiler_tempC >= Differential_ON) {
   Siurblys_ijungtas();
   Serial.println("Siurblys- _I_J_U_N_G_T_A_S_");
}
//tikrinama ar kolektoriaus ir boilerio temperatūrų skirtumes nėra mažesnis už nustatytą
if (Collector_tempC - Boiler_tempC <= Differential_OFF) { 
   Siurblys_isjungtas();
   Serial.println("Siurblys- _N_E_D_I_R_B_A_");
}
//apsauga nuo užšalimo 
if (freezing == 1) { //jei apsauga įjungta,
	//tikrinama ar kolektoriaus temperatūra teigiama
if (Collector_tempC  <= 0 ) { //jei temperatūra neigiama, 
   Siurblys_ijungtas();       //įjungiamas siurblys
   Serial.println("Ijungtas siurblys apsaugai nuo uzsalimo");
}}
//______________________________________________
//Kolektoriaus jutiklių klaidų tikrinimo pražia
//----------------------------------------------
}else{//Jei kolektoriaus temperatūros jutiklis neveikia, įjungiamas siurblys 3 minutėms (laiką nurodo kintamasis REQUEST_Collector_Error) 

if (millis() > timer_Collector_Error + REQUEST_Collector_Error) {
   timer_Collector_Error = millis();
   Saved_Boiler_tempC=Boiler_tempC; //Atsimenama boilerio temperatūra
   Siurblys_ijungtas();
   Serial.println("kolektoriaus temperatūros matavimo klaida, siurblys ijungtas");
   
}else{ if (Saved_Boiler_tempC-Boiler_tempC <=2){
  Siurblys_isjungtas(); //Jei kolektoriaus temperatūros jutiklis neveikia ir boilerio temperatūra krinta, siurblys išjungiamas
}}//______________________________________________
  //Kolektoriaus jutiklių klaidų tikrinimo pražia
  //----------------------------------------------
}}
else{ if (Collector_tempC >=81){Siurblys_ijungtas();} 

}} //__________________________________________________
   //Pabaiga tikrinamo kokia turį būti siurblio būsena.
   //--------------------------------------------------

}//Pagrindinės programos pabaiga
//_______________________________________
//      P A P R O G R A M Ė S 
//siurblio ir termostato relių junginėjimui
//---------------------------------------
//siurblio įjungimas ir atitinkamo ženklo ekrane rodymas
void Siurblys_ijungtas()  {
 digitalWrite(Pump, LOW);
 digitalWrite(13, HIGH);
   lcd.setCursor(15,0);
   lcd.write((uint8_t)2);//char up to LCD
//   lcd.setCursor(15,1);
//   lcd.write((uint8_t)2);//char up to LCD
}
//siurblio įšjungimas ir atitinkamo ženklo ekrane rodymas
void Siurblys_isjungtas(){
digitalWrite(Pump, HIGH);
   lcd.setCursor(15,0);
   lcd.write((uint8_t)1);//char down to LCD
//   lcd.setCursor(15,1);
//   lcd.write((uint8_t)1); //char down to LCD
}
//termostato relės įjungimas ir atitinkamo ženklo ekrane rodymas
void Rele_2_ijungta()  {
digitalWrite(Relay2, LOW);
digitalWrite(13, HIGH);
   lcd.setCursor(10,1);
   lcd.write((uint8_t)2);//char up to LCD
}
//termostato reles įsjungimas ir atitinkamo ženklo ekrane rodymas
void Rele_2_isjungta(){
digitalWrite(Relay2, HIGH);
   lcd.setCursor(10,1);
   lcd.write((uint8_t)1); //char down to LCD
}
