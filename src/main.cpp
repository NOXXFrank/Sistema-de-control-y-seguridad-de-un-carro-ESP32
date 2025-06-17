#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>


const char* ssid = "ESP-SCSC";
const char* password = "NOXX_Frank06082005";


// IP estática
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);


WebServer server(80);


// Pines del módulo RFID
#define SS_PIN 19
#define RST_PIN 34
MFRC522 rfid(SS_PIN, RST_PIN);


// SPI
#define SCK 21
#define MOSI 22
#define MISO 23


// Habilitación de los motores
#define ENA 14  // Motor izquierdo
#define ENB 12  // Motor derecho


// Control de los motores Izquierdo
#define IN1_I 16
#define IN2_I 4
#define IN3_I 2
#define IN4_I 15


// Control de los motores Derecho
#define IN1_D 26
#define IN2_D 25
#define IN3_D 33
#define IN4_D 32


// Velocidad inicial (0 - 255)
int velocidad = 60;
bool tarjetaAutorizadaDetectada = false; // Verificar si la tarjeta ya fue autorizada
bool mensajeImpreso = false; // Variable para controlar la impresión del mensaje


// UID de la tarjeta autorizada (F3 B3 CD 0D)
byte tarjetaAutorizada[] = {0xF3, 0xB3, 0xCD, 0x0D};


// Función para establecer la velocidad con PWM
void setSpeed(int vel) {
    velocidad = constrain(vel, 0, 255); // Limitar entre 0 y 255
    ledcWrite(0, velocidad);  // PWM en canal 0 para ENA
    ledcWrite(1, velocidad);  // PWM en canal 1 para ENB
}


// Configuración de los pines de los motores
void setupMotors() {
    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);


    int pins[] = {IN1_I, IN2_I, IN3_I, IN4_I, IN1_D, IN2_D, IN3_D, IN4_D};
    for (int i = 0; i < 8; i++) {
        pinMode(pins[i], OUTPUT);
        digitalWrite(pins[i], LOW); // Apagar motores al inicio
    }


    // Configuración de PWM en ESP32
    ledcSetup(0, 5000, 8); // Canal 0, 5kHz, 8 bits
    ledcSetup(1, 5000, 8); // Canal 1, 5kHz, 8 bits


    ledcAttachPin(ENA, 0);
    ledcAttachPin(ENB, 1);


    setSpeed(velocidad); // Establecer velocidad inicial
}


// Funciones de movimiento del carro
void adelante() {
    if (tarjetaAutorizadaDetectada) { // Solo mover si la tarjeta está autorizada
        setSpeed(velocidad);
        digitalWrite(IN1_I, LOW);
        digitalWrite(IN2_I, HIGH);
        digitalWrite(IN3_I, HIGH);
        digitalWrite(IN4_I, LOW);
        digitalWrite(IN1_D, HIGH);
        digitalWrite(IN2_D, LOW);
        digitalWrite(IN3_D, LOW);
        digitalWrite(IN4_D, HIGH);
    }
}


void atras() {      
    if (tarjetaAutorizadaDetectada) { // Solo mover si la tarjeta está autorizada
        setSpeed(velocidad);
        digitalWrite(IN1_I, HIGH);  // Motor izquierdo hacia atras
        digitalWrite(IN2_I, LOW);
        digitalWrite(IN3_I, LOW);
        digitalWrite(IN4_I, HIGH);
       
        digitalWrite(IN1_D, LOW); // Motor derecho hacia atras
        digitalWrite(IN2_D, HIGH);
        digitalWrite(IN3_D, HIGH);
        digitalWrite(IN4_D, LOW);
    }
}


void detener() {
    setSpeed(0);
    int pins[] = {IN1_I, IN2_I, IN3_I, IN4_I, IN1_D, IN2_D, IN3_D, IN4_D};
    for (int i = 0; i < 8; i++) {
        digitalWrite(pins[i], LOW);
    }
}


void girarI() {
    if (tarjetaAutorizadaDetectada) { // Solo mover si la tarjeta está autorizada
        setSpeed(velocidad);
        digitalWrite(IN1_I, LOW);  // Motor izquierdo hacia atras
        digitalWrite(IN2_I, HIGH);
        digitalWrite(IN3_I, HIGH);
        digitalWrite(IN4_I, LOW);


        digitalWrite(IN1_D, LOW);  // Motor derecho hacia adelante
        digitalWrite(IN2_D, HIGH);
        digitalWrite(IN3_D, HIGH);
        digitalWrite(IN4_D, LOW);
    }
}


void girarD() {
    if (tarjetaAutorizadaDetectada) { // Solo mover si la tarjeta está autorizada
        setSpeed(velocidad);
        digitalWrite(IN1_I, HIGH);  // Motor izquierdo hacia adelante
        digitalWrite(IN2_I, LOW);
        digitalWrite(IN3_I, LOW);
        digitalWrite(IN4_I, HIGH);


        digitalWrite(IN1_D, HIGH);  // Motor derecho hacia atras
        digitalWrite(IN2_D, LOW);
        digitalWrite(IN3_D, LOW);
        digitalWrite(IN4_D, HIGH);
    }
}


bool verificarTarjeta() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return false;
    }
    // Verificar si el UID de la tarjeta coincide con el autorizado
    for (byte i = 0; i < sizeof(tarjetaAutorizada); i++) {
        if (rfid.uid.uidByte[i] != tarjetaAutorizada[i]) {
            return false;
        }
    }
    return true;
}


// Función para bloquear el carro
void bloquearCarro() {
    tarjetaAutorizadaDetectada = false; // Desactivar el movimiento
    detener(); // Detener el carro
    Serial.println("Carro bloqueado. Se requiere tarjeta autorizada para moverlo nuevamente.");
}


// Funciones handle para las rutas
void handleAdelante() {
    if (tarjetaAutorizadaDetectada) { // Solo mover si la tarjeta está autorizada
        Serial.println("Solicitud recibida: /adelante");
        adelante();
        server.send(200, "text/plain", "Adelante");
    } else {
        server.send(403, "text/plain", "No autorizado");
    }
}


void handleAtras() {
    if (tarjetaAutorizadaDetectada) { // Solo mover si la tarjeta está autorizada
        Serial.println("Solicitud recibida: /atras");
        atras();
        server.send(200, "text/plain", "Atras");
    } else {
        server.send(403, "text/plain", "No autorizado");
    }
}


void handleGirarI() {
    if (tarjetaAutorizadaDetectada) { // Solo mover si la tarjeta está autorizada
        Serial.println("Solicitud recibida: /izquierda");
        girarI();
        server.send(200, "text/plain", "Izquierda");
    } else {
        server.send(403, "text/plain", "No autorizado");
    }
}


void handleGirarD() {
    if (tarjetaAutorizadaDetectada) { // Solo mover si la tarjeta está autorizada
        Serial.println("Solicitud recibida: /derecha");
        girarD();
        server.send(200, "text/plain", "Derecha");
    } else {
        server.send(403, "text/plain", "No autorizado");
    }
}


void handleDetener() {
    Serial.println("Solicitud recibida: /detener");
    detener();
    server.send(200, "text/plain", "Detener");
}


void handleBloquear() {
    Serial.println("Solicitud recibida: /bloquear");
    bloquearCarro();
    server.send(200, "text/plain", "Carro bloqueado");
}


void handleCambiarVelocidad() {
    if (server.hasArg("velocidad")) {
        int nuevaVelocidad = server.arg("velocidad").toInt();
        setSpeed(nuevaVelocidad);
        Serial.println("Velocidad cambiada a: " + String(nuevaVelocidad));
        server.send(200, "text/plain", "Velocidad cambiada");
    } else {
        server.send(400, "text/plain", "Falta el parámetro 'velocidad'");
    }
}


// Manejo de la página principal
void handleRoot() {
    String html = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Control del Carro</title>
        <style>
            body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
            button { padding: 15px 25px; font-size: 18px; margin: 10px; }
            #bloquear { background-color: red; color: white; }
        </style>
    </head>
    <body>
        <h1>Control del Carro</h1>
        <button ontouchstart="sendCommand('adelante')" ontouchend="sendCommand('detener')"
                onmousedown="sendCommand('adelante')" onmouseup="sendCommand('detener')">Adelante</button><br>
        <button ontouchstart="sendCommand('atras')" ontouchend="sendCommand('detener')"
                onmousedown="sendCommand('atras')" onmouseup="sendCommand('detener')">Atras</button><br>
        <button ontouchstart="sendCommand('izquierda')" ontouchend="sendCommand('detener')"
                onmousedown="sendCommand('izquierda')" onmouseup="sendCommand('detener')">Izquierda</button>
        <button ontouchstart="sendCommand('derecha')" ontouchend="sendCommand('detener')"
                onmousedown="sendCommand('derecha')" onmouseup="sendCommand('detener')">Derecha</button><br>
        <button id="bloquear" onclick="sendCommand('bloquear')">Bloquear Carro</button><br>
        <label for="velocidad">Velocidad: </label>
        <input type="range" id="velocidad" name="velocidad" min="0" max="255" value=")" + String(velocidad) + R"=====(" oninput="cambiarVelocidad(this.value)">
        <br>
        <script>
            function sendCommand(command) {
                fetch(`/${command}`)
                    .then(response => response.text())
                    .then(data => console.log(data))
                    .catch(error => console.error('Error:', error));
            }
            function cambiarVelocidad(velocidad) {
                fetch(`/cambiarVelocidad?velocidad=${velocidad}`)
                    .then(response => response.text())
                    .then(data => console.log(data))
                    .catch(error => console.error('Error:', error));
            }
        </script>
    </body>
    </html>
    )=====";
    server.send(200, "text/html", html);
}


// Configuración inicial
void setup() {
    Serial.begin(9600);
    Serial.println("Iniciando el AP del ESP32...");
    delay(2000);


    if (!WiFi.config(local_IP, gateway, subnet)) {
        Serial.println("Error al configurar IP...");
    }


    WiFi.softAP(ssid, password);
    Serial.println("Punto de acceso creado correctamente");
    Serial.println(WiFi.softAPIP());


    setupMotors();


    SPI.begin(SCK, MISO, MOSI, SS_PIN);
    rfid.PCD_Init();


    Serial.println("Sistema RFID listo");


    // Configurar rutas del servidor web
    server.on("/", handleRoot);
    server.on("/adelante", handleAdelante);
    server.on("/atras", handleAtras);
    server.on("/izquierda", handleGirarI);
    server.on("/derecha", handleGirarD);
    server.on("/detener", handleDetener);
    server.on("/bloquear", handleBloquear);
    server.on("/cambiarVelocidad", handleCambiarVelocidad);


    // Manejar rutas no encontradas
    server.onNotFound([]() {
        server.send(404, "text/plain", "Ruta no encontrada");
    });


    server.begin();
    Serial.println("Servidor web iniciado");
}


void loop() {
    server.handleClient(); // Manejar solicitudes HTTP


    // Solo verificar la tarjeta una vez para autorizar el movimiento
    if (verificarTarjeta() && !tarjetaAutorizadaDetectada) {
        Serial.println("Tarjeta autorizada detectada, el carro puede moverse.");
        tarjetaAutorizadaDetectada = true; // Se marca que ya se detectó la tarjeta
        if (!mensajeImpreso) {
            Serial.println("El carro está activo.");
            mensajeImpreso = true; // Se imprime el mensaje una sola vez
        }
    }


    // No detener el carro incluso si la tarjeta ya no se detecta
    if (!verificarTarjeta() && tarjetaAutorizadaDetectada) {
        // El carro sigue en movimiento, no hacemos nada adicional
    }


    delay(500);
}


