#include <DHT.h>
#include <Wire.h>
#include "RTClib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>
#include "EncoderStepCounter.h"
#include <avr/wdt.h>
#include <TimerThree.h>

#define RELAY_LUZUV 3
#define RELAY_CHUVA 4
#define RELAY_CONDENSACAO 5
bool isChuva, isLuzUV, isCondensacao; // Variáveis que contém o estado atual dos relays

#define DHTPIN 8
#define DHTTYPE DHT22 // Aponta qual é o modelo do sensor de umidade
DHT dht(DHTPIN, DHTTYPE); // Declara o sensor de umidade como um objeto "dht"

#define ML8511 A0
#define REF_3V3 A1

#define ONE_WIRE_BUS_1 6
#define ONE_WIRE_BUS_2 7

OneWire oneWire1(ONE_WIRE_BUS_1);
OneWire oneWire2(ONE_WIRE_BUS_2);

DallasTemperature sensorTemperatura1(&oneWire1);
DallasTemperature sensorTemperatura2(&oneWire2);

/*uint8_t sensorTemperatura1[8] = { 0x28, 0x27, 0xAE, 0x79, 0xA2, 0x00, 0x03, 0xDC }; // Serial number do sensor de temperatura I (do aquecedor)
uint8_t sensorTemperatura2[8] = { 0x28, 0x40, 0x8F, 0x16, 0xA8, 0x01, 0x3C, 0xF4 }; // Serial number do sensor de temperatura II (do ambiente)*/

// Configura a temperatura (em °C) mínima e máxima da água da condensação durante seu funcionamento
int tempMinAquecedor = 70;
int tempMaxAquecedor = 80;

RTC_DS3231 rtc; // Declara o RTC como um objeto "rtc"
DateTime now, dateAux;
char dateBuffer[10];
char timeBuffer[8];
char LCDTimeBuffer[5];

#define LCD_BACKLIGHT 28
const byte rs = 22, en = 23, d4 = 24, d5 = 25, d6 = 26, d7 = 27;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // Declara o display como um objeto chamado "lcd"
byte aoQuadrado[] = {B01100,B10010,B00100,B01000,B11110,B00000,B00000,B00000};
char LCDCiclosText[6];
int dadoLCD = 0;

unsigned long int lixo = 0;
int lixo2 = 0;

// Para o cartão SD, 3 pinos são fixos
// SCK = 52;
// MISO = 50;
// MOSI = 51;
const int CS = 53; // Determina o ultimo pino
File dataFile;

#define INTERRUPT_SW_PIN 2
#define INTERRUPT_DT_PIN 19
#define INTERRUPT_CLK_PIN 18
signed long int encoderValue = 0;
signed char encoderPosition;
unsigned long int lastButtonPress = 0;
int encoderSW = 0;
EncoderStepCounter encoder(INTERRUPT_CLK_PIN, INTERRUPT_DT_PIN, HALF_STEP);

// Para alterar o número máximo de ciclos, basta alterar a variável abaixo
int numeroMaxDeCiclos = 84;
// Além disso, é necessário preencher a arrays abaixo no seguinte formato: array[3][x], sendo x = (3 * numeroMaxDeCiclos) + 1
byte checkpoints[3][253] = {0};
long int checkpointsTimestamps[253] = {0};

int tempoEntreArquivamentos = 300; // Declara o tempo, em segundos, entre cada atualização nos dados salvos nos arquivos do cartão de memória

int umidade; // Variável que armazena a umidade
float temperaturaC1, temperaturaC2;
float Vout, irradiancia;
int instrucaoAtual, totalInstrucoes;
int cicloAtual, totalCiclos;
int faseAtual;
char dataCiclo[18];
char auxCharBuffer[10];
unsigned long int previousSensorLog = 0;

unsigned long int startTime;
long int aux;
float floatAux;
long int horas, minutos, segundos;
String stringAux;
int i, j;

byte numeroDeLeituras;
int valor;

void(* resetFunc) (void) = 0;

void setup() {

  Timer3.initialize(6000000);
  Timer3.attachInterrupt(watchdog_reset);
  wdt_enable(WDTO_8S);
  
  Serial.begin(115200);

  Serial.println(F("Inicializando..."));

  pinMode(INTERRUPT_SW_PIN, INPUT_PULLUP);
  pinMode(INTERRUPT_DT_PIN, INPUT);
  pinMode(INTERRUPT_CLK_PIN, INPUT);
  
  encoder.begin();
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_SW_PIN), buttonInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_DT_PIN), encoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_CLK_PIN), encoderInterrupt, CHANGE);

  pinMode(LCD_BACKLIGHT, OUTPUT);
  lcd.begin(16, 2); // Inicia o display lcd(x, y), de x colunas e y linhas; Nesse caso, lcd(16,2);
  Serial.println(F("Display LCD ligado")); // Informa, na porta serial, que o display esta ligado
  lcdBacklight(); // Liga a luz de fundo do display
  lcd.createChar(0, aoQuadrado);

  Serial.print(F("Iniciando cartão SD... "));
  pinMode(CS, OUTPUT); // Configura o pino CS (Chip Select) como saída
    
  if(SD.begin(CS)){ // Caso a inicialização do SD no pino CS ocorra com sucesso
    Serial.println(F("Cartão SD inicializado corretamente")); // Imprime uma mensagem de sucesso
  }else{ // Caso a inicialização do SD no pino CS falhe
    Serial.println(F("Falha na inicialização do cartão SD")); // Imprime uma mensagem de erro

    Serial.println(F("Confira as conexões com o cartão SD, e depois aperte o botão para reiniciar a placa Arduino."));

    lcd.clear();

    encoderSW = 0;
    while(encoderSW == 0){ //Inicia um loop, pois o cartão não foi devidamente iniciado. Ao pressionar o botão, o Arduino reiniciará.
      lcd.setCursor(0, 0);
      lcd.print("Erro ao iniciar:");
      lcd.setCursor(0, 1);
      lcd.print("Cartao SD");
    }
    
    resetFunc();
  }
  
  // Inicialização do RTC DS3231
  if(!rtc.begin()){ // Se ocorrer um erro na inicialização do RTC DS3231
    Serial.println(F("Não foi possível reconhecer o RTC! Verifique as conexões"));

    crashLog(F("Erro ao inicializar o RTC (Real Time CLock)"));

    lcd.clear();

    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Erro ao iniciar:");
      lcd.setCursor(0, 1);
      lcd.print("RTC DS3231");
    }
    resetFunc();
    
  }else{
    Serial.println(F("RTC DS3231 inicializado corretamente"));
  }
  now = rtc.now();
  stringAux = parseDate(now);
  if(stringAux == "165/165/2165"){
    Serial.println(F("Data incorreta!"));
    Serial.println(parseDate(now)+" "+parseTime(now)); // Imprime o horário atual

    crashLog(F("RTC com data incorreta (165/165/2165) - erro nas conexões com o RTC"));

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Data incorreta!");
      lcd.setCursor(0, 1);
      lcd.print(parseDate(now)); // Imprime o horário atual
    }

    Serial.println(F("Confira as conexões do RTC DS3231"));
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Confira as");
      lcd.setCursor(0, 1);
      lcd.print("conexoes do RTC");
    }
    
    Serial.println(F("Pressione novamente para reiniciar o dispositivo"));
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Pressione novam.");
      lcd.setCursor(0, 1);
      lcd.print("para reiniciar");
    }
    
    resetFunc();
  //}else if(stringAux == "01/01/2000"){
  }else if(now.year() == 2000){
    Serial.println(F("Data incorreta!"));
    Serial.println(parseDate(now)+" "+parseTime(now)); // Imprime o horário atual

    crashLog(F("RTC com data incorreta - sem bateria"));

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Data incorreta!");
      lcd.setCursor(0, 1);
      lcd.print(parseDate(now)); // Imprime o horário atual
    }

    Serial.println(F("Confira a bateria do RTC DS3231"));
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Confira a");
      lcd.setCursor(0, 1);
      lcd.print("bateria do RTC");
    }
    
    Serial.println(F("Substituia a bateria do RTC e pressione novamente para ajustar o horário"));
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Pressione novam.");
      lcd.setCursor(0, 1);
      lcd.print("p/ ajustar o RTC");
    }
    
    dateAux = rtc.now();
    ajustarHorario(dateAux);
  }


  Serial.print(F("Hora atual: "));
  Serial.print(parseDate(now));
  Serial.print(F(" "));
  Serial.println(parseTime(now)); // Imprime o horário atual

  dataFile = SD.open("boot.csv", FILE_WRITE);

  if(dataFile){
    dataFile.println(parseDate(now)+" "+parseTime(now));
  }else{
    Serial.println(F("Erro ao abrir boot.csv"));

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Erro ao abrir:");
      lcd.setCursor(0, 1);
      lcd.print("boot.csv");
    }
    resetFunc();
  }

  dataFile.close();

  Serial.println();

  // Inicialização dos relays
  pinMode(RELAY_CHUVA, OUTPUT); // Declara o pino RELAY_CHUVA como OUTPUT
  pinMode(RELAY_LUZUV, OUTPUT); // Declara o pino RELAY_LUZUV como OUTPUT
  pinMode(RELAY_CONDENSACAO, OUTPUT); // Declara o pino RELAY_CONDENSACAO como OUTPUT
  chuva(false); // Desliga a bomba de água
  luzUV(false); // Desliga as lâmpadas UV
  condensacao(false); // Desliga o aquecedor de água

  // Inicialização do Sensor de Umidade
  dht.begin();
  Serial.println(F("Sensor de umidade DHT22 inicializado corretamente"));

  // Inicialização do Sensor de luz UV

  pinMode(ML8511, INPUT);
  pinMode(REF_3V3, INPUT);

  // Inicialização dos Sensores de Temperatura
  sensorTemperatura1.begin();
  sensorTemperatura2.begin();

  Serial.print(F("Localizando termômetros... "));
  Serial.print(F("Foram encontrados "));
  aux = sensorTemperatura1.getDeviceCount() + sensorTemperatura2.getDeviceCount();
  Serial.print(aux, DEC);
  Serial.println(F(" sensores DS18B20"));

  sensorTemperatura1.requestTemperatures();
  delay(50);
  sensorTemperatura2.requestTemperatures();
  delay(50);

  temperaturaC1 = sensorTemperatura1.getTempCByIndex(0);
  temperaturaC2 = sensorTemperatura2.getTempCByIndex(0);
  Serial.print(F("Temperatura 1: "));
  Serial.print((String)temperaturaC1);
  Serial.println(F(" °C"));
  Serial.print(F("Temperatura 2: "));
  Serial.print((String)temperaturaC2);
  Serial.println(F(" °C"));

  if(temperaturaC1 == -127.00 || temperaturaC2 == -127.00){
    Serial.println(F("Algum termômetro não está funcionando corretamente"));
    Serial.println(F("Confira as conexões e tente novamente"));

    crashLog(F("Erro ao inicializar o(s) termometro(s)"));
    
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Erro ao utilizar:");
      lcd.setCursor(0, 1);
      lcd.print("Termometros");
    }

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Temperatura 1:");
      lcd.setCursor(0, 1);
      lcd.print(temperaturaC1);
      if(temperaturaC1 == -127.00){
        lcd.print(" (ERRO)");
      }                                                 
    }

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Temperatura 2:");
      lcd.setCursor(0, 1);
      lcd.print(temperaturaC2);
      if(temperaturaC2 == -127.00){
        lcd.print(" (ERRO)");
      }                                                
    }
    
    Serial.println(F("Pressione novamente para reiniciar o dispositivo"));
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Pressione novam.");
      lcd.setCursor(0, 1);
      lcd.print("para reiniciar");
    }
    
    resetFunc();
  }

  Serial.println(F("Inicialização realizada com sucesso!"));
  Serial.println();
  delay(100);

  if(SD.exists("data.txt")){
  //if(1 == 0){ ///////////// TESTE
    Serial.println(F("Rotina em andamento encontrada. Iniciando retomada..."));
    continuarRotina();
  }else{
    Serial.println(F("Nenhuma rotina em andamento foi encontrada. Iniciando uma nova rotina..."));
    novaRotina();
  }
}

void loop() {

  now = rtc.now();

  stringAux = parseDate(now);
  if(stringAux == "165/165/2165" || stringAux == "01/01/2000"){
    Serial.println(F("Data incorreta!"));
    Serial.println(parseDate(now)+" "+parseTime(now)); // Imprime o horário atual

    resetFunc();
  }

  Serial.println(parseDate(now)+" "+parseTime(now));

  // Verificar se é necessário atualizar o ciclo e o estado atual dos relays
  if(now.unixtime() >= checkpointsTimestamps[instrucaoAtual]){
  //if(1 == 1){
    Serial.println(F("Atualizando ciclo..."));
    atualizarCiclo(now, totalInstrucoes);
  }else{ // Caso contrário, é executada uma rotina de obtenção e log de dados dos sensores
    
    ///////////////////////////////////////////////////////////////////////////////////////// DEBUG
    Serial.print(F("Ciclo e fase atual: "));
    Serial.println((String)cicloAtual+" - "+(String)faseAtual);
    
    /*Serial.print(F("Instrução atual: "));
    Serial.println((String)instrucaoAtual);
    
    Serial.print(F("checkpoints[2][instrucaoAtual] = "));
    Serial.println((String)checkpoints[2][instrucaoAtual]);*/

    // Exibe o horário atual no Display LCD
    lcd.setCursor(0, 0); // Posiciona o cursor na primeira coluna (0) e na primeira linha (0)
    lcd.print(parseLCDTime(now)); // Imprime, na primeira linha, o horário (HH:MM)

    // Exibe o ciclo atual, a fase atual e o total de ciclos no Display LCD
    sprintf(LCDCiclosText, "C%03d-%01d/%03d", cicloAtual, faseAtual, totalCiclos);
    lcd.setCursor(6, 0);
    lcd.print(LCDCiclosText);

    // get humidity
    umidade = dht.readHumidity();
    Serial.print(F("Umidade: "));
    Serial.print((String)umidade);
    Serial.println(F(" %HR"));

    // get temperature(s)
    sensorTemperatura1.requestTemperatures();
    delay(50);
    sensorTemperatura2.requestTemperatures(); 
    delay(50);

    temperaturaC1 = sensorTemperatura1.getTempCByIndex(0);
    temperaturaC2 = sensorTemperatura2.getTempCByIndex(0);
    Serial.print(F("Temperatura 1: "));
    Serial.print((String)temperaturaC1);
    Serial.println(F(" °C"));
    Serial.print(F("Temperatura 2: "));
    Serial.print((String)temperaturaC2);
    Serial.println(F(" °C"));

    if(temperaturaC1 == -127.00 || temperaturaC2 == -127.00){
      Serial.println(F("Algum termômetro não está funcionando corretamente"));
      Serial.println(F("Reiniciando para tentar resolver o problema..."));
      resetFunc();
    }

    if(faseAtual == 3){
      if(temperaturaC1 >= tempMaxAquecedor){
        if(isCondensacao){
          Serial.print(F("Temperatura acima de 80 °C - Desligando o Aquecedor..."));
          condensacao(false);
        }
      }else if(temperaturaC1 <= tempMinAquecedor){
        if(!isCondensacao){
          Serial.print(F("Temperatura abaixo de 70 °C - Ligando o Aquecedor..."));
          condensacao(true);
        }
      }
    }

    // get UV
    Vout = 3.3 / mediaAnalogica(REF_3V3) * mediaAnalogica(ML8511);
    irradiancia = mapFloat(Vout, 0.99, 2.8, 0.0, 150.0);
    Serial.print(F("Irradiância: "));
    Serial.print((String)irradiancia);
    Serial.println(F(" W/m²"));

    // Exiibe dados dos sensores no Display LCD
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);

    if(dadoLCD == 4){
      dadoLCD = 0;
    }

    /*Serial.print(F("dadoLCD (1): "));
    Serial.println(dadoLCD);*/

    if(dadoLCD == 0){
      lcd.print("Umidade: "+(String)umidade+" %HR");
    }else if(dadoLCD == 1){
      lcd.print("Temp1: "+(String)temperaturaC1+(String)char(223)+"C");
    }else if(dadoLCD == 2){
      lcd.print("Temp2: "+(String)temperaturaC2+(String)char(223)+"C");
    }else if(dadoLCD == 3){
      lcd.print("Irr.: ");
      lcd.print((String)irradiancia);
      lcd.print(" W/m");
      lcd.write(byte(0));
    }
    dadoLCD++;

    /*Serial.print(F("dadoLCD (2): "));
    Serial.println(dadoLCD);*/

    //if(1){
    if(now.unixtime() - previousSensorLog >= tempoEntreArquivamentos){

      Serial.println(F("Arquivando dados..."));
      
      //Serial.print(F("previousSensorLog: "));
      //Serial.print(previousSensorLog);
      //Serial.print(F(" -> "));
      previousSensorLog = now.unixtime();
      //Serial.println(previousSensorLog);
      
      // Armazena os dados dos sensores no cartão (sensores.csv)
      dataFile = SD.open("sensores.csv", FILE_WRITE);
      
      if(dataFile){
        dataFile.print(parseDate(now));
        dataFile.print(F(" "));
        dataFile.print(parseTime(now));
        dataFile.print(F(";"));
        dataFile.print(cicloAtual);
        dataFile.print(F(";"));
        dataFile.print(faseAtual);
        dataFile.print(F(";"));
        dataFile.print(umidade);
        dataFile.print(F(";"));
        dataFile.print(temperaturaC1);
        dataFile.print(F(";"));
        dataFile.print(temperaturaC2);
        dataFile.print(F(";"));
        dataFile.println((String)irradiancia);

      }else{
        Serial.println(F("Erro ao abrir sensores.csv"));

        crashLog(F("Erro ao abrir sensores.csv"));

        lcd.clear();
        encoderSW = 0;
        while(encoderSW == 0){
          lcd.setCursor(0, 0);
          lcd.print("Erro ao abrir:");
          lcd.setCursor(0, 1);
          lcd.print("sensores.csv");
        }
        resetFunc();
      }

      dataFile.close();

      dataFile = SD.open("freeram.csv", FILE_WRITE);

      if(dataFile){
        dataFile.print(parseDate(now));
        dataFile.print(F(" "));
        dataFile.print(parseTime(now));
        dataFile.print(F(";"));
        dataFile.println(freeMemory());
      }else{
        Serial.println(F("Erro ao abrir freeram.csv"));
  
        crashLog(F("Erro ao abrir freeram.csv"));
  
        lcd.clear();
        encoderSW = 0;
        while(encoderSW == 0){
          lcd.setCursor(0, 0);
          lcd.print("Erro ao abrir:");
          lcd.setCursor(0, 1);
          lcd.print("freeram.csv");
        }
        resetFunc();
      }
  
      dataFile.close();
    }

    //Serial.print(F("lixo (1): "));
    //Serial.println(lixo);
    lixo++;
    //Serial.print(F("lixo (2): "));
    //Serial.println(lixo);

    //Serial.print(F("lixo2 (1): "));
    //Serial.println(lixo2);
    lixo2++;
    //Serial.print(F("lixo2 (2): "));
    //Serial.println(lixo2);

    Serial.print(F("RAM disponível: "));
    Serial.println(freeMemory());
    Serial.println();

    // Delay de 10 seg
    aux = millis();
    while(millis() - aux < 10000){
      if(encoderSW == 1){
        // Abre o Menu
        encoderSW = 0;
        menu(totalInstrucoes);
      }
    }
  }
}

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else // __ARM__
extern char *__brkval;
#endif // __arm__
int freeMemory() {
  char top;
  #ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
  #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
  #else // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
  #endif // __arm__
}

void watchdog_reset(){
  wdt_reset();
}

void buttonInterrupt(){
  if(millis() - lastButtonPress >= 200){
    lastButtonPress = millis();
    encoderSW = 1;
  }
}

void encoderInterrupt(){
  encoder.tick();

  encoderPosition = encoder.getPosition();
    if (encoderPosition != 0) {
      encoderValue += encoderPosition;
      encoder.reset();
    }
}

void crashLog(String errorMessage){
  dateAux = rtc.now();

  dataFile = SD.open("crashlog.csv", FILE_WRITE);

  if(dataFile){
    dataFile.print(parseDate(dateAux)+" "+parseTime(dateAux));
    dataFile.print(F(";"));
    dataFile.println(errorMessage);

    dataFile.close();
  }else{
    Serial.println(F("Erro ao abrir crashlog.csv"));

    /*lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print(F("Erro ao abrir:"));
      lcd.setCursor(0, 1);
      lcd.print("crashlog.csv");
    }
    resetFunc();*/
  }
}

void lcdBacklight(){
  digitalWrite(LCD_BACKLIGHT, HIGH);
}

void lcdNoBacklight(){
  digitalWrite(LCD_BACKLIGHT, LOW);
}

void novaRotina(){

  Serial.println(F("Configurando uma nova rotina..."));
  
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Nova rotina:");
  lcd.setCursor(4, 1);
  lcd.print("ciclos");

  encoderValue = 10;
  totalCiclos = encoderValue;
  encoderSW = 0;
  while(encoderSW == 0){
    aux = millis();
    while(millis() - aux <= 800){
     
      if(encoderValue < 1){
        encoderValue = 1;
      }else if(encoderValue > 84){
        encoderValue = 84;
      }
  
      totalCiclos = encoderValue;
      lcd.setCursor(0, 1);
      lcd.print(totalCiclos);
    }

    lcd.setCursor(0, 1);
    lcd.print("   ");
    //delay(150);
    aux = millis();
    while(millis() - aux <= 100){
      
      if(encoderValue < 1){
        encoderValue = 1;
      }else if(encoderValue > 84){
        encoderValue = 84;
      }
      
      totalCiclos = encoderValue;      
    }
  }
  encoderSW = 0;
  
  Serial.print(F("Nova rotina de: "));
  Serial.print((String)totalCiclos);
  Serial.println(F(" ciclos"));

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Configurando:");
  lcd.setCursor(0, 1);
  lcd.print(totalCiclos);
  lcd.print(" Ciclos");

  now = rtc.now();
  startTime = now.unixtime();
  aux = startTime;
  totalInstrucoes = totalCiclos * 3;

  char dataCiclo[18];

  // Abrir um arquivo "data.txt" no cartão SD para guardar a programação dos ciclos
  dataFile = SD.open("data.txt", FILE_WRITE);
  if(dataFile){
    j = 0;
    for(i=1; i<=totalCiclos; i++){
      sprintf(dataCiclo, "0:%03d:%10ld:1", i, aux);
      Serial.println(dataCiclo);
      dataFile.println(dataCiclo);
      checkpoints[1][j] = i;
      checkpointsTimestamps[j] = aux;
      checkpoints[2][j] = 1;
      aux += 28800; // Duração de cada ciclo UV em segundos
      j++;
      sprintf(dataCiclo, "0:%03d:%10ld:2", i, aux);
      Serial.println(dataCiclo);
      dataFile.println(dataCiclo);
      checkpoints[1][j] = i;
      checkpointsTimestamps[j] = aux;
      checkpoints[2][j] = 2;
      aux += 900; // Duração de cada ciclo UV em segundos
      j++;
      sprintf(dataCiclo, "0:%03d:%10ld:3", i, aux);
      Serial.println(dataCiclo);
      dataFile.println(dataCiclo);
      checkpoints[1][j] = i;
      checkpointsTimestamps[j] = aux;
      checkpoints[2][j] = 3;
      aux += 13500; // Duração de cada ciclo UV em segundos
      j++;
    }
    sprintf(dataCiclo, "0:%03d:%10ld:0", totalCiclos, aux);
    Serial.println(dataCiclo);
    dataFile.println(dataCiclo);
    checkpoints[1][j] = i;
    checkpointsTimestamps[j] = aux;
    checkpoints[2][j] = 0;

    dataFile.close();

    Serial.println(F("Arquivo de dados 'data.txt' criado com sucesso!"));
    Serial.println(F("Iniciando rotina de trabalho..."));
    Serial.println();

    dateAux = startTime;
    Serial.print(F("Início: "));
    Serial.print(parseDate(dateAux));
    Serial.print(F(" "));
    Serial.println(parseTime(dateAux));

    dateAux = aux;
    Serial.print(F("Previsão de término: "));
    Serial.print(parseDate(dateAux));
    Serial.print(F(" "));
    Serial.println(parseTime(dateAux));
    Serial.println();
  }
  else{
    Serial.println(F("Erro ao abrir data.txt"));
    
    crashLog(F("Erro ao abrir data.txt"));

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Erro ao abrir:");
      lcd.setCursor(0, 1);
      lcd.print("data.txt");
    }
    resetFunc();
  }

  instrucaoAtual = 0;
  cicloAtual = 1;
  faseAtual = 1;

  lcd.clear();
  previousSensorLog = now.unixtime() - tempoEntreArquivamentos;

  atualizarCiclo(now, totalInstrucoes);
}

void menu(int totalInstrucoes){
  Serial.println(F("Abrindo o Menu:"));
  
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Menu:");
  
  encoderValue = 0;
  encoderSW = 0;
  while(encoderSW == 0){
    
    if(encoderValue < 0){
      encoderValue = 0;
    }else if(encoderValue > 4){
      encoderValue = 4;
    }

    if(encoderValue == 0){
      lcd.setCursor(0, 1);
      lcd.print("                ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(1, 1);
        lcd.print("Horario Termino");
        
        if(encoderValue != 0 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 1);
        lcd.print("                ");
        
        if(encoderValue != 0 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 1){
      lcd.setCursor(0, 1);
      lcd.print("                ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(1, 1);
        lcd.print("Excluir Rotina");
        
        if(encoderValue != 1 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 1);
        lcd.print("                ");
        
        if(encoderValue != 1 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 2){
      lcd.setCursor(0, 1);
      lcd.print("                ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(0, 1);
        lcd.print("Ajustar horario");
        
        if(encoderValue != 2 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 1);
        lcd.print("                ");
        
        if(encoderValue != 2 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 3){
      lcd.setCursor(0, 1);
      lcd.print("                ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(5, 1);
        lcd.print("Teste Equipame.");
        
        if(encoderValue != 3 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 1);
        lcd.print("                ");
        
        if(encoderValue != 3 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 4){
      lcd.setCursor(0, 1);
      lcd.print("                ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(5, 1);
        lcd.print("Voltar");
        
        if(encoderValue != 4 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 1);
        lcd.print("                ");
        
        if(encoderValue != 4 || encoderSW != 0){
          break;
        }
      }
    }
  }
  encoderSW = 0;

  if(encoderValue == 0){
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      dateAux = checkpointsTimestamps[totalInstrucoes];

      lcd.setCursor(3, 0);
      lcd.print(parseDate(dateAux));
      lcd.setCursor(4, 1);
      lcd.print(parseTime(dateAux));
    }
    menu(totalInstrucoes);
  }else if(encoderValue == 1){
    excluirRotina();
  }else if(encoderValue == 2){
    lcd.clear();
    ajustarHorario(dateAux);
  }else if(encoderValue == 3){
    lcd.clear();
    dateAux = rtc.now();
    testarEquipamentos();
  }else{
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Retornando...");

    Serial.println(F("Retornando à rotina..."));
    Serial.println();
  }
}

void excluirRotina(){
  lcd.clear();
  encoderSW = 0;
  while(encoderSW == 0){
    lcd.setCursor(0, 0);
    lcd.print("Deseja iniciar");
    lcd.setCursor(0, 1);
    lcd.print("uma nova rotina?");
  }

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Sim");
  lcd.setCursor(6, 1);
  lcd.print("Nao");
  
  encoderValue = 0;
  encoderSW = 0;
  while(encoderSW == 0){

    if(encoderValue < 0){
      encoderValue = 0;
    }else if(encoderValue > 1){
      encoderValue = 1;
    }

    lcd.setCursor(6, 0);
    lcd.print("Sim");
    lcd.setCursor(6, 1);
    lcd.print("Nao");

    if(encoderValue == 0){
      lcd.setCursor(0, 0);
      lcd.print("                ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(6, 0);
        lcd.print("Sim");
        
        if(encoderValue != 0 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 0);
        lcd.print("                ");
        
        if(encoderValue != 0 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 1){
      lcd.setCursor(0, 1);
      lcd.print("                ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(6, 1);
        lcd.print("Nao");
        
        if(encoderValue != 1 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 1);
        lcd.print("                ");
        
        if(encoderValue != 1 || encoderSW != 0){
          break;
        }
      }
    }
  }

  encoderSW = 0;
  lcd.clear();
  
  if(encoderValue == 0){
    Serial.println(F("Excluindo a rotina existente..."));
    lcd.setCursor(0, 0);
    lcd.print("Excluindo...");
  
    // Apagar os arquivos do cartão de memória para permitir a criação de uma nova rotina
    SD.remove("boot.csv");
    SD.remove("chuva.csv");
    SD.remove("ciclos.csv");
    SD.remove("condens.csv");
    SD.remove("crashlog.csv");
    SD.remove("data.txt");
    SD.remove("finallog.txt");
    SD.remove("freeram.csv");
    SD.remove("sensores.csv");
    SD.remove("luzuv.csv");
    
    resetFunc();
  }else{
    menu(totalInstrucoes);
  }
}

void ajustarHorario(DateTime& dateAux){
  int auxArray[6];

  if(parseDate(dateAux) == "165/165/2165" && parseTime(dateAux) == "165:165:85"){
    DateTime dateAux(2021, 01, 01, 00, 00, 00);
  }

  auxArray[0] = dateAux.year();
  auxArray[1] = dateAux.month();
  auxArray[2] = dateAux.day();
  auxArray[3] = dateAux.hour();
  auxArray[4] = dateAux.minute();
  auxArray[5] = dateAux.second();

  lcd.clear();
  encoderValue = 0;
  encoderSW = 0;
  while(encoderSW == 0){

    if(encoderValue < 0){
      encoderValue = 0;
    }else if(encoderValue > 7){
      encoderValue = 7;
    }

    lcd.setCursor(0, 0);
    lcd.print(parseDate(dateAux));
    lcd.setCursor(12, 0);
    lcd.print("Sair");
    lcd.setCursor(0, 1);
    lcd.print(parseTime(dateAux));
    lcd.setCursor(10, 1);
    lcd.print("Salvar");

    if(encoderValue == 0){ // Dia
      lcd.setCursor(0, 0);
      lcd.print("  ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(0, 0);
        lcd.print(parseDate(dateAux));
        
        if(encoderValue != 0 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 0);
        lcd.print("  ");
        
        if(encoderValue != 0 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 1){ // Mês
      lcd.setCursor(3, 0);
      lcd.print("  ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(0, 0);
        lcd.print(parseDate(dateAux));
        
        if(encoderValue != 1 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(3, 0);
        lcd.print("  ");
        
        if(encoderValue != 1 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 2){ // Ano
      lcd.setCursor(6, 0);
      lcd.print("    ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(0, 0);
        lcd.print(parseDate(dateAux));
        
        if(encoderValue != 2 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(6, 0);
        lcd.print("    ");
        
        if(encoderValue != 2 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 3){ // Horas
      lcd.setCursor(0, 1);
      lcd.print("  ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(0, 1);
        lcd.print(parseTime(dateAux));
        
        if(encoderValue != 3 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(0, 1);
        lcd.print("  ");
        
        if(encoderValue != 3 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 4){ // Minutos
      lcd.setCursor(3, 1);
      lcd.print("  ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(0, 1);
        lcd.print(parseTime(dateAux));
        
        if(encoderValue != 4 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(3, 1);
        lcd.print("  ");
        
        if(encoderValue != 4 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 5){ // Segundos
      lcd.setCursor(6, 1);
      lcd.print("  ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(0, 1);
        lcd.print(parseTime(dateAux));
        
        if(encoderValue != 5 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(6, 1);
        lcd.print("  ");
        
        if(encoderValue != 5 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 6){ // Voltar
      lcd.setCursor(12, 0);
      lcd.print("    ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(12, 0);
        lcd.print("Sair");
        
        if(encoderValue != 6 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(12, 0);
        lcd.print("    ");
        
        if(encoderValue != 6 || encoderSW != 0){
          break;
        }
      }
    }else if(encoderValue == 7){ // Salvar

    

      lcd.setCursor(10, 1);
      lcd.print("      ");

      aux = millis();
      while(millis() - aux <= 800){
        lcd.setCursor(10, 1);
        lcd.print("Salvar");
        
        if(encoderValue != 7 || encoderSW != 0){
          break;
        }
      }
      aux = millis();
      while(millis() - aux <= 100){
        lcd.setCursor(10, 1);
        lcd.print("      ");
        
        if(encoderValue != 7 || encoderSW != 0){
          break;
        }
      }
    }
  }
  encoderSW = 0;
  
  if(encoderValue == 0){
    encoderValue = auxArray[2];
    encoderSW = 0;
    while(encoderSW == 0){
      aux = millis();
      while(millis() - aux <= 800){
      
        if(encoderValue < 1){
          encoderValue = 1;
        }else if(encoderValue > 31){
          encoderValue = 31;
        }
    
        auxArray[2] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        lcd.setCursor(0, 0);
        lcd.print(parseDate(dateAux));
      }

      lcd.setCursor(0, 0);
      lcd.print("  ");
      aux = millis();
      while(millis() - aux <= 100){
        if(encoderValue < 0){
          encoderValue = 0;
        }else if(encoderValue > 31){
          encoderValue = 31;
        }

        auxArray[2] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        
        if(encoderSW != 0){
          break;
        }
      }
    }
    encoderSW = 0;
    ajustarHorario(dateAux);
  }else if(encoderValue == 1){
    encoderValue = auxArray[1];
    encoderSW = 0;
    while(encoderSW == 0){
      aux = millis();
      while(millis() - aux <= 800){
      
        if(encoderValue < 1){
          encoderValue = 1;
        }else if(encoderValue > 12){
          encoderValue = 12;
        }
    
        auxArray[1] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        lcd.setCursor(0, 0);
        lcd.print(parseDate(dateAux));
      }

      lcd.setCursor(3, 0);
      lcd.print("  ");
      aux = millis();
      while(millis() - aux <= 100){
        if(encoderValue < 1){
          encoderValue = 1;
        }else if(encoderValue > 12){
          encoderValue = 12;
        }

        auxArray[1] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        
        if(encoderSW != 0){
          break;
        }
      }
    }
    encoderSW = 0;
    ajustarHorario(dateAux);
  }else if(encoderValue == 2){
    encoderValue = auxArray[0];
    encoderSW = 0;
    while(encoderSW == 0){
      aux = millis();
      while(millis() - aux <= 800){
      
        if(encoderValue < 2020){
          encoderValue = 2020;
        }
    
        auxArray[0] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        lcd.setCursor(0, 0);
        lcd.print(parseDate(dateAux));
      }

      lcd.setCursor(6, 0);
      lcd.print("    ");
      aux = millis();
      while(millis() - aux <= 100){
        if(encoderValue < 2020){
          encoderValue = 2020;
        }

        auxArray[0] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        
        if(encoderSW != 0){
          break;
        }
      }
    }
    encoderSW = 0;
    ajustarHorario(dateAux);
  }else if(encoderValue == 3){
    encoderValue = auxArray[3];
    encoderSW = 0;
    while(encoderSW == 0){
      aux = millis();
      while(millis() - aux <= 800){
      
        if(encoderValue < 0){
          encoderValue = 0;
        }else if(encoderValue > 23){
          encoderValue = 23;
        }
    
        auxArray[3] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        lcd.setCursor(0, 1);
        lcd.print(parseTime(dateAux));
      }

      lcd.setCursor(0, 1);
      lcd.print("  ");
      aux = millis();
      while(millis() - aux <= 100){
        if(encoderValue < 0){
          encoderValue = 0;
        }else if(encoderValue > 23){
          encoderValue = 23;
        }

        auxArray[3] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        
        if(encoderSW != 0){
          break;
        }
      }
    }
    encoderSW = 0;
    ajustarHorario(dateAux);
  }else if(encoderValue == 4){
    encoderValue = auxArray[4];
    encoderSW = 0;
    while(encoderSW == 0){
      aux = millis();
      while(millis() - aux <= 800){
      
        if(encoderValue < 0){
          encoderValue = 0;
        }else if(encoderValue > 59){
          encoderValue = 59;
        }
    
        auxArray[4] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        lcd.setCursor(0, 1);
        lcd.print(parseTime(dateAux));
      }

      lcd.setCursor(3, 1);
      lcd.print("  ");
      aux = millis();
      while(millis() - aux <= 100){
        if(encoderValue < 0){
          encoderValue = 0;
        }else if(encoderValue > 59){
          encoderValue = 59;
        }

        auxArray[4] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        
        if(encoderSW != 0){
          break;
        }
      }
    }
    encoderSW = 0;
    ajustarHorario(dateAux);
  }else if(encoderValue == 5){
    encoderValue = auxArray[5];
    encoderSW = 0;
    while(encoderSW == 0){
      aux = millis();
      while(millis() - aux <= 800){
      
        if(encoderValue < 0){
          encoderValue = 0;
        }else if(encoderValue > 59){
          encoderValue = 59;
        }
    
        auxArray[5] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        lcd.setCursor(0, 1);
        lcd.print(parseTime(dateAux));
      }

      lcd.setCursor(6, 1);
      lcd.print("  ");
      aux = millis();
      while(millis() - aux <= 100){
        if(encoderValue < 0){
          encoderValue = 0;
        }else if(encoderValue > 59){
          encoderValue = 59;
        }

        auxArray[5] = encoderValue;
        DateTime newDate(auxArray[0],auxArray[1],auxArray[2],auxArray[3],auxArray[4],auxArray[5]);
        dateAux = newDate;
        
        if(encoderSW != 0){
          break;
        }
      }
    }
    encoderSW = 0;
    ajustarHorario(dateAux);
  }else if(encoderValue == 6){
    menu(totalInstrucoes);
  }else if(encoderValue == 7){
    Serial.print(F("Atualizando data e hora: "));
    rtc.adjust(dateAux);
    Serial.println(parseDate(dateAux)+" "+parseTime(dateAux));
    delay(100); // Para conseguir imprimir todo o texto na porta serial antes de reiniciar
    resetFunc();
  }
}

void testarEquipamentos(){
  // Utilizado para testar, individualmente, o sistema de luz UV, chuva e condensação
  luzUV(false);
  chuva(false);
  condensacao(false);

  // Testando lâmpadas UV

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testando:");
  lcd.setCursor(0, 1);
  lcd.print("Lampadas UV");
  luzUV(true);

  encoderSW = 0;
  while(encoderSW == 0){
    continue;
  }
  
  luzUV(false);

  // Testando chuva

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testando:");
  lcd.setCursor(0, 1);
  lcd.print("Chuva");
  chuva(true);

  encoderSW = 0;
  while(encoderSW == 0){
    continue;
  }

  chuva(false);

  // Testando condensação

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testando:");
  lcd.setCursor(0, 1);
  lcd.print("Condensacao");
  condensacao(true);

  encoderSW = 0;
  while(encoderSW == 0){
    continue;
  }

  condensacao(false);

  // Retornando os relés à fase atual da rotina

  if(faseAtual == 1){
    luzUV(true);
  }else if(faseAtual == 2){
    chuva(true);
  }else if(faseAtual == 3){
    condensacao(true);
  }
}

void continuarRotina(){ /////////////////////////////////////////////////////////////////////////////////////// REGISTRAR O MOMENTO DA REINICIALIZAÇÃO NO SD
  // preencher as arrays 
  dataFile = SD.open("data.txt", FILE_READ);

  char dataCiclo[18];  ///// TESTE

  if(dataFile){
    aux = 0;
    Serial.println(F("Dados lidos do arquivo 'data.txt':"));
    while(dataFile.available()){
      stringAux = dataFile.readStringUntil('\r');
      dataFile.read(); // Lê o '/r', que não deve ser arquivado nas arrays

      checkpoints[0][aux] = stringAux.substring(0,1).toInt();
      checkpoints[1][aux] = stringAux.substring(2,5).toInt();
      checkpointsTimestamps[aux] = stringAux.substring(6,16).toInt();
      checkpoints[2][aux] = stringAux.substring(17,18).toInt();

      sprintf(dataCiclo, "%01d:%03d:%10ld:%01d", checkpoints[0][aux], checkpoints[1][aux], checkpointsTimestamps[aux], checkpoints[2][aux]);
      Serial.println(dataCiclo);

      aux++;
    }
  }else{
    Serial.println(F("Erro ao abrir data.txt"));

    crashLog(F("Erro ao abrir data.txt"));

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Erro ao abrir:");
      lcd.setCursor(0, 1);
      lcd.print("data.txt");
    }
    resetFunc();
  }
  dataFile.close();

  // Reconhecer a instrução, o ciclo e a fase atual
  totalInstrucoes = aux-1; // Desconsidera a última linha do arquivo 'data.txt', que é vazia
  totalCiclos = checkpoints[1][aux-1];
  aux = 0;
  for(i=0; i<=totalInstrucoes; i++){
    if(checkpoints[0][i] == 0){
      instrucaoAtual = i;
      cicloAtual = checkpoints[1][i-1];
      faseAtual = checkpoints[2][i-1];
      break;
    }else{
      aux++;
    }
  }

  Serial.println();
  Serial.println(F("Dados reconhecidos:"));
  Serial.print(F("Instrução atual: "));
  Serial.println((String)instrucaoAtual+"/"+(String)totalInstrucoes);
  Serial.print(F("Ciclo atual: "));
  Serial.println((String)cicloAtual+"/"+(String)totalCiclos);
  Serial.print(F("Fase atual: "));
  Serial.println((String)faseAtual);

  /*now = rtc.now();
  dataFile = SD.open("crashlog.csv", FILE_WRITE);

  if(dataFile){
    dataFile.print(F("O sistema foi reinicializado em: "));
    dataFile.println(parseDate(now)+" "+parseTime(now));
  }else{
    Serial.println(F("Erro ao abrir crashlog.csv"));

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print(F("Erro ao abrir:"));
      lcd.setCursor(0, 1);
      lcd.print("crashlog.csv");
    }
    resetFunc();
  }

  dataFile.close();*/

  crashLog(F("O sistema foi reinicializado"));

  Serial.println(F("Retomando instrução anterior..."));
  Serial.println();
  instrucaoAtual--; // Envio da instrução anterior para o void loop para que o estado dos relays seja atualizado, já que o void continuarRotina é executado logo após uma reinicialização do sistema
  // Dessa forma, enviando a instrução anterior, o loop irá reconhecer que possui um timeStamp obsoleto e irá encaminhar para o atualizarCiclo, que restitui os estados dos pinos de controle dos relays

  previousSensorLog = now.unixtime() - tempoEntreArquivamentos;
}

void atualizarCiclo(DateTime& rtcTime, int totalInstrucoes){
  // Ligar/Desligar módulos e relatar no cartão sd
  if(checkpoints[2][instrucaoAtual] == 1){
    Serial.print(F("Atualizando para Ciclo "));
    Serial.println((String)checkpoints[1][instrucaoAtual]+" - "+(String)checkpoints[2][instrucaoAtual]);
    // Desligar CON, ligar UV
    condensacao(false); // Desliga o aquecedor de água
    luzUV(true); // Liga as lâmpadas UV
    
  }else if(checkpoints[2][instrucaoAtual] == 2){
    Serial.print(F("Atualizando para Ciclo "));
    Serial.println((String)checkpoints[1][instrucaoAtual]+" - "+(String)checkpoints[2][instrucaoAtual]);
    // Desligar UV, ligar CHU
    luzUV(false); // Desliga as lâmpadas UV
    chuva(true); // Liga a bomba de água
    
  }else if(checkpoints[2][instrucaoAtual] == 3){
    Serial.print(F("Atualizando para Ciclo "));
    Serial.println((String)checkpoints[1][instrucaoAtual]+" - "+(String)checkpoints[2][instrucaoAtual]);
    // Desligar CHU, ligar CON
    chuva(false); // Desliga a bomba de água
    condensacao(true); // Liga o aquecedor de água
    
  }else if(checkpoints[2][instrucaoAtual] == 0){
    Serial.println();
    Serial.println(F("Encerrando rotina..."));
    // Desligar UV, CHU e CON, encerrar a rotina e gerar relatório
    luzUV(false); // Desliga as lâmpadas UV
    chuva(false); // Desliga a bomba de água
    condensacao(false); // Desliga o aquecedor de água

    dataFile = SD.open("ciclos.csv", FILE_WRITE);

    if(dataFile){
      dataFile.print(parseDate(rtcTime));
      dataFile.print(F(" "));
      dataFile.print(parseTime(rtcTime));
      dataFile.print(F(";"));
      dataFile.print(F("Rotina"));
      dataFile.print(F(";"));
      dataFile.print(F("Concluida"));
      dataFile.println();
    }else{
      Serial.println(F("Erro ao abrir ciclos.csv"));

      crashLog(F("Erro ao abrir ciclos.csv"));

      lcd.clear();
      encoderSW = 0;
      while(encoderSW == 0){
        lcd.setCursor(0, 0);
        lcd.print("Erro ao abrir:");
        lcd.setCursor(0, 1);
        lcd.print("ciclos.csv");
      }
      resetFunc();
    }

    dataFile.close();

    // gerar um relatorio final

    dataFile = SD.open("finallog.txt", FILE_WRITE);

    dateAux = checkpointsTimestamps[0];
    Serial.print(F("Rotina de "));
    Serial.print((String)totalCiclos);
    Serial.println(F(" ciclos concluída!"));
    dataFile.print(F("Rotina de "));
    dataFile.print((String)totalCiclos);
    dataFile.println(F(" ciclos concluída!"));

    Serial.println();
    Serial.print(F("Início: "));
    Serial.println((String)parseDate(dateAux)+" "+(String)parseTime(dateAux));
    dataFile.print(F("Início: "));
    dataFile.println((String)parseDate(dateAux)+" "+(String)parseTime(dateAux));

    Serial.print(F("Término: "));
    Serial.println((String)parseDate(rtcTime)+" "+(String)parseTime(rtcTime));
    dataFile.print(F("Término: "));
    dataFile.println((String)parseDate(rtcTime)+" "+(String)parseTime(rtcTime));

    aux = checkpointsTimestamps[instrucaoAtual] - checkpointsTimestamps[0];
    horas = aux/3600;
    minutos = aux/60-horas*60;
    segundos = aux - horas*3600 - minutos*60;

    Serial.println();
    Serial.print(F("Duração estimada: "));
    Serial.print(horas);
    Serial.print(F(":"));
    Serial.print(minutos);
    Serial.print(F(":"));
    Serial.print(segundos);
    Serial.println();

    dataFile.print(F("Duração estimada: "));
    dataFile.print(horas);
    dataFile.print(F(":"));
    dataFile.print(minutos);
    dataFile.print(F(":"));
    dataFile.print(segundos);
    dataFile.println();

    aux = rtcTime.unixtime() - checkpointsTimestamps[0];
    horas = aux/3600;
    minutos = aux/60-horas*60;
    segundos = aux - horas*3600 - minutos*60;

    Serial.print(F("Tempo decorrido: "));
    Serial.print(horas);
    Serial.print(F(":"));
    Serial.print(minutos);
    Serial.print(F(":"));
    Serial.print(segundos);
    Serial.println();

    dataFile.print(F("Tempo decorrido: "));
    dataFile.print(horas);
    dataFile.print(F(":"));
    dataFile.print(minutos);
    dataFile.print(F(":"));
    dataFile.print(segundos);
    dataFile.println();

    dataFile.close();

    // imprimir uma mensagem de conclusão da rotina
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("    Rotina");
      lcd.setCursor(0, 1);
      lcd.print("   encerrada");
    }  

    // aguarda o botão ser pressionado para excluir/mover os arquivos e reiniciar o arduino
    Serial.println(F("Salve os arquivos do cartão SD que desejar"));
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Salve os arquiv.");
      lcd.setCursor(0, 1);
      lcd.print("que desejar (SD)");
    }
    
    Serial.println(F("Pressione novamente para reiniciar o dispositivo"));
    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Pressione novam.");
      lcd.setCursor(0, 1);
      lcd.print("para reiniciar");
    }

    // Apagar os arquivos do cartão de memória para permitir a criação de uma nova rotina
    SD.remove("boot.csv");
    SD.remove("chuva.csv");
    SD.remove("ciclos.csv");
    SD.remove("condens.csv");
    SD.remove("crashlog.csv");
    SD.remove("data.txt");
    SD.remove("finallog.txt");
    SD.remove("freeram.csv");
    SD.remove("sensores.csv");
    SD.remove("luzuv.csv");
    
    resetFunc();
  }

  // Registrar a mudança de ciclo/fase
  dataFile = SD.open("ciclos.csv", FILE_WRITE);

  if(dataFile){
    dataFile.print(parseDate(rtcTime));
    dataFile.print(F(" "));
    dataFile.print(parseTime(rtcTime));
    dataFile.print(F(";"));
    dataFile.print(F("Ciclo "));
    dataFile.print(checkpoints[1][instrucaoAtual]);
    dataFile.print(F(";"));
    dataFile.print(F("Fase "));
    dataFile.print(checkpoints[2][instrucaoAtual]);
    dataFile.println();
  }else{
    Serial.println(F("Erro ao abrir ciclos.csv"));

    crashLog(F("Erro ao abrir ciclos.csv"));

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Erro ao abrir:");
      lcd.setCursor(0, 1);
      lcd.print("ciclos.csv");
    }
    resetFunc();
  }

  dataFile.close();

  // marcar o checkpoint no 'data.txt'
  dataFile = SD.open("data.txt", O_RDWR); // Abre o arquivo 'data.txt' em um modo que permite sobrescrições
  if(dataFile){
    aux = 20 * instrucaoAtual; // Calcula a posição do caractere a ser editado
    dataFile.seek(aux); // Aponta para o caractere desejado
    dataFile.write('1'); // Sobrescreve o caractere por '1', a fim de registrar que a ação descrita naquela instrução foi executada
  }else{
    Serial.println(F("Erro ao abrir data.txt"));

    crashLog(F("Erro ao abrir data.txt"));

    lcd.clear();
    encoderSW = 0;
    while(encoderSW == 0){
      lcd.setCursor(0, 0);
      lcd.print("Erro ao abrir:");
      lcd.setCursor(0, 1);
      lcd.print("data.txt");
    }
    resetFunc();
  }
  dataFile.close();
  
  //atualizar as variáveis de ciclo

  cicloAtual = checkpoints[1][instrucaoAtual];
  faseAtual = checkpoints[2][instrucaoAtual];
  instrucaoAtual++;

  previousSensorLog = rtcTime.unixtime() - tempoEntreArquivamentos;

  // Delay de 1 seg (para evitar sobrecarga nos relés por revisão de rotina, após uma queda de energia)
  aux = millis();
  encoderSW = 0;
  while(millis() - aux < 1000){
    if(encoderSW == 1){
      // Abre o Menu
      encoderSW = 0;
      menu(totalInstrucoes);
    }
  }
}

int mediaAnalogica(int pinoAnalogico){
  numeroDeLeituras = 8;
  valor = 0; 

  for(int x = 0 ; x < numeroDeLeituras ; x++)
    valor += analogRead(pinoAnalogico);
    valor /= numeroDeLeituras;

  return valor; 
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max){
  floatAux = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  return floatAux;
}

void luzUV(bool estadoLuzUV){
  dataFile = SD.open("luzuv.csv", FILE_WRITE);

  if(estadoLuzUV == true){
    digitalWrite(RELAY_LUZUV, HIGH);
    isLuzUV = true;
    Serial.println(F("Lâmpadas UV ligadas"));
    if(dataFile){
      dataFile.println(parseDate(now)+" "+parseTime(now)+";1");
    }else{
      Serial.println(F("Erro ao abrir luzuv.csv"));

      crashLog(F("Erro ao abrir luzuv.csv"));

      lcd.clear();
      encoderSW = 0;
      while(encoderSW == 0){
        lcd.setCursor(0, 0);
        lcd.print("Erro ao abrir:");
        lcd.setCursor(0, 1);
        lcd.print("luzuv.csv");
      }
      resetFunc();
    }
  }else{
    digitalWrite(RELAY_LUZUV, LOW);
    isLuzUV = false;
    Serial.println(F("Lâmpadas UV desligadas"));
    if(dataFile){
      dataFile.println(parseDate(now)+" "+parseTime(now)+";0");
    }else{
      Serial.println(F("Erro ao abrir luzuv.csv"));

      crashLog(F("Erro ao abrir luzuv.csv"));

      lcd.clear();
      encoderSW = 0;
      while(encoderSW == 0){
        lcd.setCursor(0, 0);
        lcd.print("Erro ao abrir:");
        lcd.setCursor(0, 1);
        lcd.print("luzuv.csv");
      }
      resetFunc();
    }
  }
  dataFile.close();
}

void chuva(bool estadoChuva){
  dataFile = SD.open("chuva.csv", FILE_WRITE);

  if(estadoChuva == true){
    digitalWrite(RELAY_CHUVA, HIGH);
    isChuva = true;
    Serial.println(F("Chuva ligada"));
    if(dataFile){
      dataFile.println(parseDate(now)+" "+parseTime(now)+";1");
    }else{
      Serial.println(F("Erro ao abrir chuva.csv"));

      crashLog(F("Erro ao abrir chuva.csv"));

      lcd.clear();
      encoderSW = 0;
      while(encoderSW == 0){
        lcd.setCursor(0, 0);
        lcd.print("Erro ao abrir:");
        lcd.setCursor(0, 1);
        lcd.print("chuva.csv");
      }
      resetFunc();
    }
  }else{
    digitalWrite(RELAY_CHUVA, LOW);
    isChuva = false;
    Serial.println(F("Chuva desligada"));
    if(dataFile){
      dataFile.println(parseDate(now)+" "+parseTime(now)+";0");
    }else{
      Serial.println(F("Erro ao abrir chuva.csv"));

      crashLog(F("Erro ao abrir chuva.csv"));

      lcd.clear();
      encoderSW = 0;
      while(encoderSW == 0){
        lcd.setCursor(0, 0);
        lcd.print("Erro ao abrir:");
        lcd.setCursor(0, 1);
        lcd.print("chuva.csv");
      }
      resetFunc();
    }
  }
  dataFile.close();
}

void condensacao(bool estadoCondensacao){
  dataFile = SD.open("condens.csv", FILE_WRITE);

  if(estadoCondensacao == true){
    digitalWrite(RELAY_CONDENSACAO, HIGH);
    isCondensacao = true;
    Serial.println(F("Condensação ligada"));
    if(dataFile){
      dataFile.println(parseDate(now)+" "+parseTime(now)+";1");
    }else{
      Serial.println(F("Erro ao abrir condens.csv"));

      crashLog(F("Erro ao abrir condens.csv"));

      lcd.clear();
      encoderSW = 0;
      while(encoderSW == 0){
        lcd.setCursor(0, 0);
        lcd.print("Erro ao abrir:");
        lcd.setCursor(0, 1);
        lcd.print("condens.csv");
      }
      resetFunc();
    }
  }else{
    digitalWrite(RELAY_CONDENSACAO, LOW);
    isCondensacao = false;
    Serial.println(F("Condensação desligada"));
    if(dataFile){
      dataFile.println(parseDate(now)+" "+parseTime(now)+";0");
    }else{
      Serial.println(F("Erro ao abrir condens.csv"));

      crashLog(F("Erro ao abrir condens.csv"));

      lcd.clear();
      encoderSW = 0;
      while(encoderSW == 0){
        lcd.setCursor(0, 0);
        lcd.print("Erro ao abrir:");
        lcd.setCursor(0, 1);
        lcd.print("condens.csv");
      }
      resetFunc();
    }
  }
  dataFile.close();
}

String parseDate(DateTime& rtcTime){
  sprintf(dateBuffer, "%02d/%02d/%04d", rtcTime.day(), rtcTime.month(), rtcTime.year());
  return dateBuffer;
}

String parseTime(DateTime& rtcTime){
  sprintf(timeBuffer, "%02d:%02d:%02d", rtcTime.hour(), rtcTime.minute(), rtcTime.second());
  return timeBuffer;
}

String parseLCDTime(DateTime& rtcTime){
  sprintf(LCDTimeBuffer, "%02d:%02d", rtcTime.hour(), rtcTime.minute());
  return LCDTimeBuffer;
}

// CORRIGIR O HORARIO TERMINO
