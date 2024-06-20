
#include "HX711.h" //la libreria la puedes instalar en el IDE de arduino desde aqui: http://librarymanager/All#Avia_HX711

#define LOADCELL_DOUT_PIN  5 //pin DT del modulo HX711 ira conectado al pin A0 de arduino.
#define LOADCELL_SCK_PIN  18  //pin SCK del modulo HX711 ira conectado al pin A1 de arduino.

HX711 scale;

float calibration_factor = -610; //-501 funciona para mi celda de carga de 5 kgs

void setup() {
  Serial.begin(115200);
  Serial.println("Skecth Básico HX711");
  Serial.println("Remueve cualquier peso de la báscula");
  Serial.println("Despues de las lecturas po un objeto con peso conocido en la báscula");
  Serial.println("Presiona + para aumentar el factor de calibración");
  Serial.println("Presiona - para disminuir el factor de calibración");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare();	//Reset the scale to 0

  long zero_factor = scale.read_average(); //Obten una lectura de referencia
  Serial.print("Zero factor: "); //Puede ser utilizado para  quitar la necesidad de calibrar la bascula, muy util en proyectos permamentes.
  Serial.println(zero_factor); //solo poner el valor cuando ya tengas bien definido el valor del factor cero
}

void loop() {

  scale.set_scale(calibration_factor); //Ajuste del factor de calibración

  Serial.print("Lectura: ");
  Serial.print(scale.get_units(), 0);
  Serial.print(" Gramos"); //Cambia A KILOGRAMOS Y REAJUSTA EL VALOR DE CALIBRACIÓN
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+' || temp == 'a')
      calibration_factor += 10;
    else if(temp == '-' || temp == 'z')
      calibration_factor -= 10;
  }
}
