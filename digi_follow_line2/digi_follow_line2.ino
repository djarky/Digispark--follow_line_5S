// ============================================================
//       AUTO RC SEGUIDOR DE LINEA - 5 SENSORES
//
//       1 motor  = ACELERACION
//       1 electroiman/actuador = DIRECCION
//                  _________
//               __|0-0-0-0-0|__
//          ||==|---------------|==||
//             |  DIGISPARK PRO  |
//      ||    |                   |    ||
//      ||===| oo  ===========  oo |===||
//      ||===|-----===========-----|===||
//      ||     = /  =========   \ =    ||
//
// ============================================================


// ============================================================
// MOTOR DE ACELERACION
// ============================================================

#define MOTOR_ACC_1 0
#define MOTOR_ACC_2 1


// ============================================================
// DIRECCION
// ============================================================

#define MOTOR_DIR_1 2
#define MOTOR_DIR_2 3


// ============================================================
// SENSORES
// ============================================================

#define SENSOR_1 A8
#define SENSOR_2 A9
#define SENSOR_3 A10
#define SENSOR_4 A11
#define SENSOR_5 A12


// ============================================================
// VELOCIDAD
// ============================================================

#define VELOCIDAD_RECTO       100
#define VELOCIDAD_CURVA        60
#define VELOCIDAD_CURVA_FUERTE 40


// ============================================================
// DETECCION
// ============================================================

// Intensidad minima total para considerar
// que realmente estamos viendo la linea.

#define UMBRAL_LINEA 80


// ============================================================
// CALIBRACION
// ============================================================


int sensorMin[5];
int sensorMax[5];

int rawValores[5];
byte lineaValores[5];

int rawValoresAnt[5];
byte lineaValoresAnt[5];

//definir direciones

#define D_LEFT   -1
#define D_CENTRE  0
#define D_RIGHT   1
#define D_LOST    2

// ============================================================
// ULTIMA DIRECCION
// Sirve para saber hacia donde buscar
// si perdemos temporalmente la linea.
// ============================================================


int ultimaDireccion = 0;


// ============================================================
// INICIALIZAR CALIBRACION
// ============================================================
void inicializarCalibracion() {
  for (byte i = 0; i < 5; i++) {
    sensorMin[i] = 1023;
    sensorMax[i] = 0;
    rawValoresAnt[i] = -1;
    lineaValoresAnt[i] = 255;
  }
}


// ============================================================
// LEER SENSORES
// ============================================================

void leerSensores(int valores[]) {

  valores[0] = analogRead(SENSOR_1);
  valores[1] = analogRead(SENSOR_2);
  valores[2] = analogRead(SENSOR_3);
  valores[3] = analogRead(SENSOR_4);
  valores[4] = analogRead(SENSOR_5);
}


// ============================================================
// ACTUALIZAR CALIBRACION AUTOMATICA
// ============================================================
void actualizarCalibracion(int valores[]) {
  for (byte i = 0; i < 5; i++) {
    if (valores[i] < sensorMin[i]) {
      sensorMin[i] = valores[i];
    }
    if (valores[i] > sensorMax[i]) {
      sensorMax[i] = valores[i];
    }
  }
}


// ============================================================
// NORMALIZAR SENSOR
//
// Devuelve:
// 0   = no ve linea
// 100 = ve linea con mucha intensidad
//
// ============================================================

// ============================================================
// NORMALIZAR VALOR DE SENSOR (0 A 100)
// ============================================================

//cambiar esto para hacer que siga linas blancas sobre fondo negro
#define LINEA_BLANCA false


byte sensorLinea(int valor, byte i) {

  int minimo = sensorMin[i];
  int maximo = sensorMax[i];

  if (maximo <= minimo + 10)
    return 0;

  long resultado;

  if (LINEA_BLANCA){
    resultado = (long)(valor - minimo) * 100 / (maximo - minimo);
  }
  else{
    resultado = (long)(maximo - valor) * 100 / (maximo - minimo);
  }
  
  return constrain(resultado, 0, 100);
}


// ============================================================
// CALCULAR DIRECCION
//
// S1 S2 S3 S4 S5
//
// ←              →
//
// -1 = izquierda
//  0 = centro
// +1 = derecha
// +2 = perdida
//
// ============================================================
int calcularDireccion(int valores[]) {

  byte s1 = sensorLinea(valores[0], 0);
  byte s2 = sensorLinea(valores[1], 1);
  byte s3 = sensorLinea(valores[2], 2);
  byte s4 = sensorLinea(valores[3], 3);
  byte s5 = sensorLinea(valores[4], 4);

  int total = s1 + s2 + s3 + s4 + s5; //suma total de la linea

  if (total < UMBRAL_LINEA){
    return D_LOST;  // se perdio la linea
  }

  int izquierda = s1 + s2;
  int derecha   = s4 + s5;
  int direccion = D_CENTRE;

  if (s3 > izquierda && s3 > derecha){
    direccion = D_CENTRE; // si la linea central es mas fuerte
  }
  else if (izquierda > derecha){
    direccion = D_LEFT;
  }
  else if (derecha > izquierda){
    direccion = D_RIGHT;
  }

  ultimaDireccion = direccion;
  return direccion;
}


// ============================================================
// ACELERACION
// ============================================================

void acelerar(int velocidad) {

  velocidad = constrain(velocidad, 0, 255);

  analogWrite(MOTOR_ACC_1, velocidad);
  analogWrite(MOTOR_ACC_2, 0);
}

void retroceder(int velocidad) {

  velocidad = constrain(velocidad, 0, 255);

  analogWrite(MOTOR_ACC_1, 0);
  analogWrite(MOTOR_ACC_2, velocidad);
}


// ============================================================
// PARAR
// ============================================================

void parar() {

  analogWrite(MOTOR_ACC_1, 0);
  analogWrite(MOTOR_ACC_2, 0);
}


// ============================================================
// DIRECCION IZQUIERDA
// ============================================================

void direccionIzquierda() {

  digitalWrite(MOTOR_DIR_1, LOW);
  digitalWrite(MOTOR_DIR_2, HIGH);
}

// ============================================================
// DIRECCION DERECHA
// ============================================================

void direccionDerecha() {

  digitalWrite(MOTOR_DIR_1, HIGH);
  digitalWrite(MOTOR_DIR_2, LOW);
}

// ============================================================
// CENTRAR DIRECCION
// ============================================================

void direccionCentro() {

  digitalWrite(MOTOR_DIR_1, LOW);
  digitalWrite(MOTOR_DIR_2, LOW);
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  pinMode(MOTOR_ACC_1, OUTPUT);
  pinMode(MOTOR_ACC_2, OUTPUT);

  pinMode(MOTOR_DIR_1, OUTPUT);
  pinMode(MOTOR_DIR_2, OUTPUT);


  pinMode(SENSOR_1, INPUT);
  pinMode(SENSOR_2, INPUT);
  pinMode(SENSOR_3, INPUT);
  pinMode(SENSOR_4, INPUT);
  pinMode(SENSOR_5, INPUT);


  parar();
  direccionCentro();

  inicializarCalibracion();

  // ----------------------------------------------------------
  // CALIBRACION INICIAL
  //
  // Durante este segundo mueve el auto sobre la superficie
  // para que los sensores conozcan blanco y linea.
  // ----------------------------------------------------------
  
  for (int i=0;i<600;i++){
    int lectura[5];
    leerSensores(lectura);
    actualizarCalibracion(lectura);
  }
  delay(1000);
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  int valores[5];
  // LEER
  leerSensores(valores);
  // ACTUALIZAR CALIBRACION
  //actualizarCalibracion(valores);
  // DIRECCION
  int direccion = calcularDireccion(valores);

  // ----------------------------------------------------------
  // VELOCIDAD
  //
  // Si estamos centrados → rapido
  // Si estamos girando → mas lento
  // ----------------------------------------------------------

  int velocidad = (direccion == D_CENTRE)? VELOCIDAD_RECTO : VELOCIDAD_CURVA;

  //si se pierde la linea
  if(direccion == D_LOST){
    //invertir la direcion para maniobrar
    switch(ultimaDireccion){
      case D_RIGHT : direccionIzquierda();break;
      case D_LEFT  : direccionDerecha();  break;
      case D_CENTRE: direccionCentro();   break;
      default:       direccionCentro();   break;
    }
    if(ultimaDireccion != D_LOST){
      retroceder(VELOCIDAD_CURVA_FUERTE);
      actualizarCalibracion(valores);
    }
    else{
      parar();
    }
  }
  else{
    //si no, avanzamos normal
    switch(direccion){
      case D_RIGHT : direccionDerecha();  break;
      case D_LEFT  : direccionIzquierda();break;
      case D_CENTRE: direccionCentro();   break;
      default:       direccionCentro();   break;
    }
    acelerar(velocidad);
  }
  

  // FRECUENCIA DEL CONTROL


  delay(5);
}
