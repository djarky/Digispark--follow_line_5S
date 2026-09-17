// ============================================================
//   DEBUG DE SENSORES Y CALIBRACION VIA USB MIDI (DIGISPARK)
// ============================================================

#include <DigiMIDI.h>

DigiMIDIDevice midi;

// --- SENSORES ANALOGICOS ---
 #define SENSOR_5 A8
 #define SENSOR_4 A9
 #define SENSOR_3 A10
 #define SENSOR_2 A11
 #define SENSOR_1 A12 

// --- LED DE ESTADO / DEBUG (Pin 1 en Digispark Pro / Model A) ---
#define LED_PIN 1

// --- STRUCT Y ARRAYS DE CALIBRACION ---
int sensorMin[5];
int sensorMax[5];

int rawValores[5];
byte lineaValores[5];

int rawValoresAnt[5];
byte lineaValoresAnt[5];

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
// LEER SENSORES EN BRUTO
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


//-------------------------------------------
//CALCULO DE DIRECIONES SIMULANDO SU FUNCIONAMIENTO
//---------------------------------------------

//definir direciones

#define D_LEFT   -1
#define D_CENTRE  0
#define D_RIGHT   1
#define D_LOST    2

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
// SETUP
// ============================================================
void setup() {
    pinMode(LED_PIN, OUTPUT);

    // Parpadeo inicial para indicar que el programa arrancó
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);

    inicializarCalibracion();

    // ----------------------------------------------------------
    // RUTINA DE CALIBRACION INICIAL (Aprox. 3 Segundos)
    // Mueve el sensor sobre la línea y la superficie durante este tiempo
    // ----------------------------------------------------------
    for (int i = 0; i < 600; i++) {
        int lectura[5];
        leerSensores(lectura);
        actualizarCalibracion(lectura);

        // Mantiene la pila USB viva durante la calibración
        midi.update();
        midi.delay(5);
    }

    // Indicador visual: LED encendido al finalizar calibración
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    // Mantener activa la comunicación USB MIDI
    midi.update();

    // 1. LEER SENSORES
    leerSensores(rawValores);

    // 2.RECALIBRAR SI NO ENCUENTRA LINEA
    if(calcularDireccion(rawvalores)==D_LOST){
      actualizarCalibracion(rawValores);
    }


    // 3. PROCESAR VALORES PONDERADOS
    for (byte i = 0; i < 5; i++) {
        lineaValores[i] = sensorLinea(rawValores[i], i);
    }

    

    // 4. ENVIAR CC SENSORES EN BRUTO (CC 1 a CC 5)
    // Solo envía el mensaje si el valor mapeado a 7-bit (0-127) ha cambiado
    for (byte i = 0; i < 5; i++) {
        int rawMapeado = map(rawValores[i], 0, 1023, 0, 127);

        if (rawMapeado != rawValoresAnt[i]) {
            midi.sendControlChange(i + 1, rawMapeado, 1); // CC 1, 2, 3, 4, 5
            rawValoresAnt[i] = rawMapeado;
            midi.delay(1);
        }
    }

    // 5. ENVIAR CC SENSORES PONDERADOS / NORMALIZADOS (CC 6 a CC 10)
    // Transmite la intensidad escalada de la línea (0 a 100 mapeada a 0-127)
    for (byte i = 0; i < 5; i++) {
        byte lineaMapeada = map(lineaValores[i], 0, 100, 0, 127);

        if (lineaMapeada != lineaValoresAnt[i]) {
            midi.sendControlChange(i + 6, lineaMapeada, 1); // CC 6, 7, 8, 9, 10
            lineaValoresAnt[i] = lineaMapeada;
            midi.delay(1);
        }
    }

    midi.delay(5);
}
