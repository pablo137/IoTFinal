#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "HX711.h"
#include <ESP32Servo.h>
#include <ArduinoJson.h>

// Definimos el pin al que está conectado el servo
#define SERVO_PIN 13
//Pines ultrasonico
#define trigger 15
#define echo 2
//
#define LOADCELL_DOUT_PIN 5  //pin DT del modulo HX711 ira conectado al pin A0 de arduino.
#define LOADCELL_SCK_PIN 18  //pin SCK del modulo HX711 ira conectado al pin A1 de arduino.

//Sensor de peso
HX711 scale;
float calibration_factor = -632;  //funciona para mi celda de carga de 5 kgs

float cm = 0;
float weight = 0;

bool modoAutomatico = false;  // Variable para controlar el estado del modo automático
unsigned long previousMillis = 0;  // Variable para almacenar el tiempo en el que se ejecutó la acción por última vez
const long interval = 8000;  // Intervalo de espera en milisegundos (por ejemplo, 8 segundos)

String receivedMessage = "";
String pubMessage = "";
float tmpMessage = 0;
const char* MQTT_BROKER_HOST = "aqpw3xfp04sdt-ats.iot.us-east-1.amazonaws.com";
const int MQTT_BROKER_PORT = 8883;

const char* MQTT_CLIENT_ID = "ESP-32";                                          // Unique CLIENT_ID
const char* PUBLISH_TOPIC = "$aws/things/iot1/shadow/name/NuevoShadow/update";  // TOPIC where ESP32 publishes
const char* SUBSCRIBE_TOPIC = "gato_distancia_test";                            // TOPIC where ESP32 receive


// Creamos un objeto servo
Servo servoMotor;

StaticJsonDocument<JSON_OBJECT_SIZE(80)> inputDoc;

const char AMAZON_ROOT_CA1[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

const char CERTIFICATE[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUIsaxsllhzKJnXC/RT9NFu63MQeIwDQYJKoZIhvcNAQEL  
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MDUxMzIzMTYw
NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANFCt90LB/P7RHKeM+tE
+VgfTTGpsXpzoattGonpQsdMe5dK4aqnMB8SoAz8o9g7scOkLyyfbWrDeu8UwKI3
04iHBFYhBWXBmDe+ia/wB6/TkwLWhNCO4RebbK9S+6XAwpdpMVUhv4hmRNfNiFAq
YpvLJMl2q7cHvZvYV1jDHurPEnVFm7Y3g5oWbf6/D+jzoFJ0FbBS61612Gii1Ruv
dLyv8S0h/VGkN6H+/0C7gvvbSIHsMLDy+zV1YzwzZqq42ZexUXzIaP9O4VQfIBYh
ZDvMtJrvuSFQG7Cm6XKruQK3XjQhzQUjIGvmSx0KRvce4Ew5yi7gdNjPE+PA1GUX
UgkCAwEAAaNgMF4wHwYDVR0jBBgwFoAUMo3L4TA2uEPy44HLHSIE+tsSnI4wHQYD
VR0OBBYEFBkCPnXKc09Z8JcVKsc4mWJCWGoGMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBjbVn+FwXHl/UsGlaPPcmhWljj
yGpOhlG7tigCyicHvqexuNS14hS1Ie9DV3YhchoMfXXSyF43ES1ULY6HSmvFQjXX
5/lTUFTLRE1DqyZlWDc9lCj8tfptHhbzyCc+rr30Smgk0W228LfafTKiW5Oh4jrr
plJBrlxoWOXaOY7cY8rSaGarz4G2prld030ems8ZByzrydHL7/jI3Yl6Jhj5mLie
qQJY1fI5Q7h1zDNXUjYg+Bga7jDrQTwjpKgVzlMkjs81UAzqAQ6x3mhm8BSc3gPH
Yka//edOzhFDQzP33er5uIPeHnLwnw2PUBEweIRvOBSSPVufARqebSSdO9W7
-----END CERTIFICATE-----
)KEY";

const char PRIVATE_KEY[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEA0UK33QsH8/tEcp4z60T5WB9NMamxenOhq20aielCx0x7l0rh
qqcwHxKgDPyj2Duxw6QvLJ9tasN67xTAojfTiIcEViEFZcGYN76Jr/AHr9OTAtaE
0I7hF5tsr1L7pcDCl2kxVSG/iGZE182IUCpim8skyXartwe9m9hXWMMe6s8SdUWb
tjeDmhZt/r8P6POgUnQVsFLrXrXYaKLVG690vK/xLSH9UaQ3of7/QLuC+9tIgeww
sPL7NXVjPDNmqrjZl7FRfMho/07hVB8gFiFkO8y0mu+5IVAbsKbpcqu5ArdeNCHN
BSMga+ZLHQpG9x7gTDnKLuB02M8T48DUZRdSCQIDAQABAoIBAQCt0xT16V7e3V67
Mwz/ppjVCZNO6J3DkN+/jW6ZFolhEfgr8YjI//3qIcePYiEb0mJUmEKTd48lO2IX
zQ5C8cqmQzln7iAUqZ6u13h6DBHmjQI2PkmvELCLspCyhmUBB8DckwXmqJT4UbNn
65QKu1qT/Uf+jbQP4r8vAKN12QcKlhzJEHlJsTEVHLNqcmbVDx76V+3bU9oTBpkD
8YLUmHnIYWlCg7lJsy6t0MgkQbvcN3tl7gG4e85YXmJW2JHCz3pzpUS1RSHiuElz
74Yqd3t2tLG4aNQEv2rmX48vJbJ/zV7JIAI8w1TYySeEgpQjxqXX4nc4Ti1HOIie
8v4aai9xAoGBAPM0LSkedWn1buqBipxhNicpwAclCb8CxrbfynfbTGcQQiT+7ScZ
FT1HH2/ZObvTFwLjpZazAsLL6MVS3pkc7qfiMtKnbrjfW8jujYXUDYOxtTnpgkDe
J1bVV7t8GFRDzuGzpmP8wkV4wIgegzBwLx8SPPrwJm7zixVvjMEkZkvtAoGBANxF
WGjWydUIOL0hu4UsAPaqtZuSOTE3pqysswg3KycjaTINJKM4X3ZFVpvw587L3fHc
IqqlXACyNYSvpCtIU/GPuZEU0PFkQxzHSEfRhR5cyXehmeDWugkwme2T6bW03nBi
yWPsE3HCPv+nTOQwk2/XpHjud49IQlDfYB5hsnMNAoGAE1Km7+hqncORT3KBZ9FV
cyEfD7JCpvl6flcjiAhVdq7VAol6oCufA2JePg29R44K+wDJTELmykFXOi5OX03s
LEyNiy0gX/HnIfzKI8LoW/lHk9VYsvyzlXOX596BNFi63DXg13rimJByg54hAD05
kTdqvulLtG8HMunM/h99PTUCgYEArakKc9VrWMZbnVk781lnBRI/jgCxkIkUoiyi
FccRAZFcfXZ5PSnTsXYnO1iqGZMHT4jD2A/4wh++K/ciEnG6pMcwgR6NQNNQSh8f
cGfScMasm+HPZMFF+LrVkMY9yuFnQenQZzQXfGetrXdkGr/lbZbw99ribheFGSeT
K0DRJP0CgYEAuIzE4BYCk8DD9+vD/9aOG0KQkpBqRyWBOCGEqSeFkN0rKTnO4cn4
7JYkceD8bTDahUXZNs1UTJl//TCkDkLKt+bdAxJX8TjDbN111xJkpB/8sM6gd9oS
G+FrqlO+OidkjpHT7KZ5PFLDLSb+cmqmIZX9V1M3W6arssObQWN3SnU=
-----END RSA PRIVATE KEY----- 
)KEY";

WiFiManager * wifiManager = NULL;
  
WiFiClientSecure net;
PubSubClient client(net);

// Función para conectarse a AWS IoT
void connectAWS() 
{
  // Configura los certificados y la clave privada
  net.setCACert(AMAZON_ROOT_CA1);
  net.setCertificate(CERTIFICATE);
  net.setPrivateKey(PRIVATE_KEY);

  // Configura el servidor MQTT y el callback
  client.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
  client.setCallback(callback);

  // Intenta conectar a AWS IoT
  while (!client.connected()) {
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println("Connected to AWS IoT");
    } else {
      Serial.print("Failed to connect to AWS IoT: ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Suscribirse al tópico deseado
  client.subscribe(SUBSCRIBE_TOPIC);
  Serial.println("Subscribed to topic: " + String(SUBSCRIBE_TOPIC));
}

float readWeight() {
  if (scale.is_ready()) {
    long reading = scale.read();
    float weight = reading / calibration_factor;
    return weight;
  } else {
    Serial.println("Error: HX711 no se encuentra listo.");
    return -1.0;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println("Conectado");
      client.subscribe(SUBSCRIBE_TOPIC);
    } else {
      Serial.print("Falló, rc=");
      Serial.print(client.state());
      Serial.println(". Intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}


void girarServo() {
  // Angulo inicial
  int anguloInicial = 0;
  // Angulo final (girar una vez, por ejemplo, a 152 grados)
  int anguloFinal = 152;
  // Velocidad de movimiento del servo (ms)
  int velocidad = 15;

  // Girar desde el ángulo inicial al ángulo final
  for (int angulo = anguloInicial; angulo <= anguloFinal; angulo++) {
    // Movemos el servo al ángulo actual
    servoMotor.write(angulo);
    // Esperamos la velocidad especificada
    delay(velocidad);
  }
}

void callback(const char* topic, byte* payload, unsigned int length) {
  // Convierte el payload en un String
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("Message from topic " + String(topic) + ": " + message);

  // Deserializa el JSON
  DeserializationError err = deserializeJson(inputDoc, message);

  if (err) {
    Serial.print("Deserialization failed: ");
    Serial.println(err.f_str());
    return;
  }

  // Maneja el mensaje si coincide con el tema suscrito
  if (String(topic) == SUBSCRIBE_TOPIC) {
    receivedMessage = inputDoc["state"]["desired"]["result"].as<String>();
    pubMessage = inputDoc["state"]["desired"]["pub"].as<String>();
    tmpMessage = inputDoc["state"]["desired"]["temp"].as<float>();
    Serial.println("Received message: " + receivedMessage);
  }
}

float measureDistance() {
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);

  long duration = pulseIn(echo, HIGH);
  float distance = duration * 0.034 / 2;
  return distance;
}

void publishToAWS(float weight, float distance) {
  DynamicJsonDocument doc(1024);
  doc["state"]["reported"]["weight_food_ondish"] = static_cast<int>(weight);
  doc["state"]["reported"]["distancia_gato"] = distance;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(PUBLISH_TOPIC, jsonBuffer);

  Serial.print("Published: ");
  Serial.println(jsonBuffer);
}

void publishFoodStatus(String food) {
  DynamicJsonDocument doc(1024);
  doc["state"]["reported"]["food_status"] = food;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(SUBSCRIBE_TOPIC, jsonBuffer);

  Serial.print("Published: ");
  Serial.println(jsonBuffer);
}

void publishServoResult() {
  DynamicJsonDocument doc(1024);
  doc["state"]["desired"]["result"] = "Idle";

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(SUBSCRIBE_TOPIC, jsonBuffer);

  Serial.print("Published to SUBSCRIBE_TOPIC: ");
  Serial.println(jsonBuffer);
}

void publishPub(String tmp) {
  DynamicJsonDocument doc(1024);
  doc["state"]["desired"]["pub"] = tmp;
  doc["state"]["desired"]["result"] = "modoAuto";

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(SUBSCRIBE_TOPIC, jsonBuffer);

  Serial.print("Published to SUBSCRIBE_TOPIC: ");
  Serial.println(jsonBuffer);
}

void setup() {
  Serial.begin(115200);

  if (wifiManager) {
    delete wifiManager;
    wifiManager = NULL;
  }

  
  wifiManager = new WiFiManager;
  wifiManager->resetSettings();
  
 if (!wifiManager->autoConnect("CatFeederv1", "password")) 
 {
    Serial.println("Falló la conexión y no se pudo iniciar el portal de configuración.");
    ESP.restart();
  }

  // Si llega aquí, está conectado exitosamente a una red WiFi
  Serial.println("Conectado a la red WiFi.");
 
  // Inicializamos el objeto servo
  servoMotor.attach(SERVO_PIN);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare();
  long zero_factor = scale.read_average();  //Obten una lectura de referencia
  Serial.print("Zero factor: ");            //Puede ser utilizado para  quitar la necesidad de calibrar la bascula, muy util en proyectos permamentes.
  Serial.println(zero_factor);              //solo poner el valor cuando ya tengas bien definido el valor del factor cero

  servoMotor.attach(SERVO_PIN);

  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  connectAWS();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  scale.set_scale(calibration_factor);  // Ajuste del factor de calibración
  Serial.print(abs(scale.get_units()), 0);
  weight = abs(scale.get_units());
  Serial.print(" Gramos");
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  // Serial.println();
  if (Serial.available()) {
    char temp = Serial.read();
    if (temp == '+' || temp == 'a')
      calibration_factor += 10;
    else if (temp == '-' || temp == 'z')
      calibration_factor -= 10;
  }

  cm = measureDistance();
  // cm = 20;
  Serial.print(" | Distancia: ");
  Serial.print(cm);
  Serial.println(" cm");
  // publishToAWS(weight, cm);

  // Procesar el mensaje recibido
  Serial.print(receivedMessage);
  if (receivedMessage != "") {
    if (receivedMessage == "food?") {
      Serial.println("food? executed");
      // Verificar si hay comida en el plato
      if (weight < 10) {
        Serial.println("No food");
        publishFoodStatus("noFood");
      } else {
        Serial.println("With food");
        publishFoodStatus("withFood");
      }
      delay(4000);
    } else if (receivedMessage == "giraServo") {
      Serial.println("Gira servo");
      girarServo();
      publishToAWS(weight, cm);
      // Acción para el caso 2
      delay(4000);
      publishServoResult();
    } else if (receivedMessage == "modoAuto") {
      modoAutomatico = true;  // Activar modo automático
      Serial.println("Modo automatico activado");
    } else if (receivedMessage == "modoDesauto") {
      modoAutomatico = false;  // Desactivar modo automático
      Serial.println("Modo automatico desactivado");
      delay(4000);
    } else {
      Serial.println("Default case executed");
      // Acción por defecto
      delay(4000);
    }
    //receivedMessage = "";  // Limpiar el mensaje después de procesarlo
  }

  if (modoAutomatico) {
    unsigned long currentMillis = millis();  // Obtener el tiempo actual
    if (currentMillis - previousMillis >= interval) {
      // Ha pasado el intervalo de tiempo desde la última ejecución
      Serial.println("modo automatico ejecutando");
      if (cm < 10 && weight < 10 && pubMessage == "on") {
        girarServo();
        publishToAWS(weight, cm);
        publishPub("on");
      } else {
        publishPub("off");
      }
      // Actualizar el tiempo de la última ejecución
      previousMillis = currentMillis;
    }
  }

  delay(2000);  // Esperar antes de la próxima lectura
}