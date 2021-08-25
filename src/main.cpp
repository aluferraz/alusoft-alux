#include <Arduino.h>
#include <fauxmoESP.h>
#include <EEPROM.h>
#include <algorithm>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "config_form.h" //Our HTML webpage contents
// #include <DNSServer.h>

// #define DEBUG_FAUXMO true
// #define DEBUG_FAUXMO_VERBOSE true

// Global declarations
/**
*  Para mudar o funcionamento para um rele 
* de contato, mude a a linha abaixo 
de false para true
*/
#define CONTACT_MODE false
#define LED_PIN 2
#define RELAY_PIN D6
#define SWITCH_PIN D7

#define CUSTOM_WIFI_SSID_POS 1
#define CUSTOM_WIFI_PASS_POS 52
#define CUSTOM_DEVICE_NAME 152 
#define SHORT_UPTIME_COUNTER_POS 204

// #define RELAY_PIN 8
// #define ID_LAMP "lucao cabuloso"
std::string default_ssid_alux = "Alux-";
std::string custom_ssid;
std::string custom_pass; 
AsyncWebServer server(80);
// DNSServer dnsServer;

fauxmoESP fauxmo;
TSetStateCallback callbackAlexa(unsigned char device_id, const char *device_name, bool state, unsigned char value);
void setupPins();
void powerOnLed();
void bootFaux();
void bootWifiClientOrServer();
void turnLight(bool onOff);
void flashLed(int interval);
void enterConfigMode();
bool tryToConnnect();
void switchLightState();
std::string getEeprom(int position);
boolean shouldFlash;
boolean deviceConfigured;
boolean triedNewSSID;
std::string ID_LAMP;
int currentSwitchState;
unsigned long previousMillis = 0;
unsigned long previousMillisUptime = 0;
unsigned int shortUptimes = 0;
bool firstSecs = true;

void setup()
{
  setupPins();
  //powerOnLed();
  //flashInterval = 300;
  flashLed(300);
  bootWifiClientOrServer();
}

void loop()
{

  unsigned long currentMillis = millis();
  

  if (currentSwitchState != digitalRead(SWITCH_PIN))
  {
    Serial.println("Interruptor acionado");
    currentSwitchState = !currentSwitchState;
    switchLightState();
  }

  if (deviceConfigured)
  {
    fauxmo.handle();
    if (shouldFlash)
    {
      flashLed(300); //Fast
    }
  }
  else
  {
    //Disable AP and Try to connect
    if ((!custom_ssid.empty()) && (!custom_ssid.empty()) && (!ID_LAMP.empty()) && triedNewSSID == false)
    {
      Serial.println("Tentando nova configuração");
      triedNewSSID = true;
      WiFi.softAPdisconnect(true);
      delay(500);
      bootWifiClientOrServer();
    }

    if ((currentMillis - previousMillis) > 2000)
    {
      previousMillis = currentMillis;
      // Serial.println("Dispositivo nao configurado");

      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    // dnsServer.processNextRequest();
  }
  if ((currentMillis - previousMillisUptime) > 5000)
  {
    //Se passar ao menos uma vez de 5 segundos, vamos zerar o contador
    if (firstSecs == true)
    {
      firstSecs = false;
      EEPROM.begin(4096);
      Serial.printf("Contador de inicios curtos [%d]\n", 0);
      EEPROM.write(SHORT_UPTIME_COUNTER_POS, 0);
      EEPROM.commit();
    }
    previousMillisUptime = currentMillis;
  }
}

std::string getEeprom(int position)
{
  EEPROM.begin(4096); //Initialize EEPROM

  std::string memValue;
  int valueLen = EEPROM.read((position - 1));
  Serial.println("Lendo string de tamanho:");
  Serial.println(valueLen);
  Serial.printf("A partir da posicao: [%d]\n", position);

  if (valueLen != 0)
  {
    for (int i = 0; i < valueLen; i++)
    {
      memValue = memValue + (char)EEPROM.read((position + i));
    }
  }
  Serial.println("Valor lido:");
  Serial.println(memValue.c_str());
  return memValue;
}

int strSize(const char *str)
{
  int Size = 0;
  while (str[Size] != '\0')
  {
    Size++;
  }
  return Size;
}

void setEeprom(int position, const char *value)
{
  Serial.printf("SALVANDO NA EEPROM:[%s], na posicao: [%d] \n", value, position);
  int length = strSize(value);
  EEPROM.begin(4096);
  Serial.printf("Gravando Tamanho:[%d] \n", length);
  Serial.printf("Na posicao[%d] \n", (position - 1));
  EEPROM.write((position - 1), length);
  for (int n = 0; n < length; n++) // automatically adjust for number of digits
  {
    // Serial.println("Gravando Valor:");
    // Serial.println( value[n] );
    // Serial.println("Na posicao");
    // Serial.println( ( n + position) );
    EEPROM.write((n + position), value[n]);
  }
  EEPROM.commit(); //Store data to EEPROM
}

void setupPins()
{
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLDOWN_16);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Inicia ligado
  
  deviceConfigured = false;
  std::string wifiMacAdr;
  wifiMacAdr.append(WiFi.macAddress().c_str());
  wifiMacAdr.erase(std::remove(wifiMacAdr.begin(), wifiMacAdr.end(), ':'), wifiMacAdr.end());
  default_ssid_alux = default_ssid_alux + wifiMacAdr;
  Serial.println("Default WIFI SSID:");
  Serial.println(default_ssid_alux.c_str());
  EEPROM.begin(4096);
  int previousShortUptimeCount = EEPROM.read(SHORT_UPTIME_COUNTER_POS);
  previousShortUptimeCount++;
  Serial.printf("Contador de inicios curtos [%d]\n", previousShortUptimeCount);
  EEPROM.write(SHORT_UPTIME_COUNTER_POS, previousShortUptimeCount);
  if(previousShortUptimeCount == 5){
    //Limpando config wifi
    EEPROM.write((CUSTOM_WIFI_PASS_POS - 1), 0);
    EEPROM.write((CUSTOM_WIFI_SSID_POS - 1), 0);
    //Piscando Lampada
    for(int i = 0; i < 4 ; i++){
      turnLight( (i%2)== 0 );
    }
  }
  EEPROM.commit();
  
  turnLight(true);

  //O interruptor não tem lado certo, ele troca o estado atual da lampada
  currentSwitchState = digitalRead(SWITCH_PIN);
}
void powerOnLed()
{
  Serial.println("Iniciando led inidicativo");
  // digitalWrite(LED_PIN, HIGH);
  // delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
}
void flashLed(int interval)
{
  Serial.println("Flashing led inidicativo");
  for (int i = 0; i < 5; i++)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(interval);
    digitalWrite(LED_PIN, LOW);
    delay(interval);
  }
  digitalWrite(LED_PIN, HIGH); // APAGA NO FINAL
  shouldFlash = false;
}
void bootFaux()
{
  // Criando servidor para receber os comandos da Alexa
  fauxmo.createServer(true); 
  fauxmo.setPort(80);  
  fauxmo.enable(true);
  Serial.printf("Bootando lampada de ID [%s] \n", ID_LAMP.c_str());
  fauxmo.addDevice(ID_LAMP.c_str());

  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value) {
    shouldFlash = true;
    Serial.printf("[INFO] Dipositivo #%d (%s) estado: %s valor: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    //Se o dispositivo for nossa lampada
    if (strcmp(device_name, ID_LAMP.c_str()) == 0)
    {
      Serial.println("Alterando estado da luz");
      turnLight(state);
    }
  });
}
void bootWifiClientOrServer()
{
  custom_ssid = getEeprom(CUSTOM_WIFI_SSID_POS);
  custom_pass = getEeprom(CUSTOM_WIFI_PASS_POS);
  ID_LAMP = getEeprom(CUSTOM_DEVICE_NAME);
  Serial.println("Custom SSID:");
  Serial.println(custom_ssid.c_str());

  Serial.println("Custom PASS:");
  Serial.println(custom_pass.c_str());

  if ((!custom_ssid.empty()) && (!custom_pass.empty()) && (!ID_LAMP.empty()))
  {
    Serial.println("Tentando conectar");
    WiFi.mode(WIFI_STA);
    WiFi.softAPdisconnect(true);

    if (tryToConnnect())
    {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      deviceConfigured = true;
      if (deviceConfigured == true)
      {
        bootFaux();
      }
    }
    else
    {
      //Disable wifi and reenter config mode
      WiFi.mode(WIFI_OFF);
      Serial.println("Cannot connect!");
      Serial.flush();
      // ESP.deepSleep(10e6, RF_DISABLED);
      custom_ssid = "\0";
      custom_pass = "\0";
      triedNewSSID = false;
      enterConfigMode();
    }
  }
  else
  {
    enterConfigMode();
  }
}

bool tryToConnnect()
{
  if (!WiFi.mode(WIFI_STA))
  {
    Serial.println("Wifi no modo incorreto");
    return false;
  }
  if (!WiFi.begin(custom_ssid.c_str(), custom_pass.c_str()))
  {
    Serial.println("Usuario / Senha invalidos");
    return false;
  }
  // if ((WiFi.waitForConnectResult(60000) != WL_CONNECTED) )
  int tried = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    tried++;
    if (tried > 120)
    {
      Serial.println("Timeout");
      return false;
    } // 60s
  }
  return true;
}

void turnLight(boolean onOff)
{
  if (onOff == true)
  {
    digitalWrite(RELAY_PIN, HIGH);
  }
  else
  {
    
    digitalWrite(RELAY_PIN, LOW);
  }
}

void switchLightState()
{
  int curState = digitalRead(RELAY_PIN);
  turnLight(!curState);
  if(CONTACT_MODE){
    Serial.println("Transistor em : ");
    Serial.println(digitalRead(RELAY_PIN));
    delay(150);
    curState = digitalRead(RELAY_PIN);
    turnLight(!curState);
    delay(500);
    Serial.println("Transistor em : ");
    Serial.println(digitalRead(RELAY_PIN));
  }else{
    delay(500);
  }
  Serial.println("Transistor em : ");
  Serial.println(digitalRead(RELAY_PIN));
}

void enterConfigMode()
{
  deviceConfigured = false;
  Serial.println("WiFi not configured, booting up AP");
  IPAddress local_IP(192, 168, 4, 22);
  IPAddress gateway(192, 168, 4, 9);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.mode(WIFI_STA);

  //dnsServer.start(53, "*", WiFi.softAPIP());

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  WiFi.softAP(default_ssid_alux.c_str());
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.print("Local IP address: ");
  Serial.println(WiFi.softAPIP());
  // Route for root / web page
  // server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("Form de configuracao");
    request->send_P(200, "text/html", MAIN_page); //config_form.h
  });

  server.on("/config_alux", HTTP_POST, [](AsyncWebServerRequest *request) {
    int args = request->args();
    for (int i = 0; i < args; i++)
    {
      Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
      if (request->argName(i).compareTo("custom_wifi_ssid") == 0)
      {
        Serial.println("Salvar SSID");
        custom_ssid = request->arg(i).c_str();
        setEeprom(CUSTOM_WIFI_SSID_POS, request->arg(i).c_str());
      }
      if (request->argName(i).compareTo("custom_wifi_pass") == 0)
      {
        Serial.println("Salvar PASS");
        custom_pass = request->arg(i).c_str();
        setEeprom(CUSTOM_WIFI_PASS_POS, request->arg(i).c_str());
      }
      if (request->argName(i).compareTo("custom_alux_name") == 0)
      {
        Serial.println("Salvar DEVICE NAME");
        ID_LAMP = request->arg(i).c_str();
        setEeprom(CUSTOM_DEVICE_NAME, ID_LAMP.c_str());
      }
      triedNewSSID = false;
      request->send_P(200, "text/plain", "Dispositivo Configurado !"); //config_form.h


      
    }
  });
  server.begin();
  shouldFlash = true;
}