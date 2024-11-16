#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20PIN 16

/* Create an instance of OneWire */
OneWire oneWire(DS18B20PIN);

DallasTemperature sensor(&oneWire);

#define EEPROM_SIZE 64  // Tamaño de EEPROM para guardar SSID y password
#define SSID_ADDR 0
#define PASSWORD_ADDR 32
#define BUTTON_PIN 0    // Pin del pulsador (GPIO0, modificar según tu circuito)
#define BUTTON_HOLD_TIME 5000  // Tiempo en ms para detectar pulsación prolongada

// Inicializa el servidor web en el puerto 80
WebServer server(80);

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  /* Start the DS18B20 Sensor */
  sensor.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Comprueba si el botón está siendo presionado al iniciar
  if (digitalRead(BUTTON_PIN) == LOW) {
    long pressTime = millis();
    while (digitalRead(BUTTON_PIN) == LOW) {
      if (millis() - pressTime > BUTTON_HOLD_TIME) {
        Serial.println("Botón presionado prolongadamente, iniciando modo AP...");
        startAccessPoint();
        return;
      }
    }
  }

  // Lee el SSID y la contraseña guardados en EEPROM
  String ssid = readStringFromEEPROM(SSID_ADDR);
  String password = readStringFromEEPROM(PASSWORD_ADDR);
  Serial.println(ssid);
  Serial.println(password);

  if (ssid.length() > 0 && password.length() > 0) {
    // Si hay un SSID y una contraseña guardados, intentamos conectarnos a la red WiFi
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Conectando a WiFi...");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" ¡conectado!");
        startWebServer();
    } else {
        Serial.println(" ¡falló!");
        startAccessPoint();
    }
    
  } else {
    // Si no hay datos guardados, iniciamos el modo AP
    startAccessPoint();
  }
}

void loop() {
  server.handleClient();

  // Comprueba si el botón es presionado durante la ejecución
  if (digitalRead(BUTTON_PIN) == LOW) {
    long pressTime = millis();
    while (digitalRead(BUTTON_PIN) == LOW) {
      if (millis() - pressTime > BUTTON_HOLD_TIME) {
        Serial.println("Botón presionado prolongadamente, reiniciando en modo AP...");
        // Borra los datos guardados en EEPROM
        clearEEPROM();
        // Reinicia el ESP32
        ESP.restart();
      }
    }
  }
}

void startAccessPoint() {
  // Inicia el ESP32 en modo Access Point
  WiFi.softAP("ESP32-Config");
  Serial.println("Modo AP iniciado");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.softAPIP());

  // Configura el manejador del formulario
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getHTML());
  });

  server.on("/", HTTP_POST, []() {
    // Lee los datos del formulario
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    // Guarda los datos en la EEPROM
    writeStringToEEPROM(SSID_ADDR, ssid);
    writeStringToEEPROM(PASSWORD_ADDR, password);
    EEPROM.commit();

    // Responde al cliente
    server.send(200, "text/html", "<h1>Datos guardados! Intentando conectar...</h1>");

    // Intenta conectar a la red WiFi
    WiFi.begin(ssid.c_str(), password.c_str());

    Serial.print("Conectando a WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" ¡conectado!");
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(" ¡falló!");
        server.send(200, "text/html", "<h1>Conexión fallida, por favor reinicie el dispositivo e intente nuevamente.</h1>");
    }

    // Si se conecta, reinicia el ESP32
    if (WiFi.status() == WL_CONNECTED) {
        delay(1000);
        ESP.restart();
    }
  });

  // Inicia el servidor web
  server.begin();
}

void startWebServer() {
  // Configura el servidor web para manejar peticiones
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  /*
  Respuesta default OK
  */
  server.on("/", HTTP_GET, []() {
    String jsonResponse = "{\"status\":\"success\",\"message\":\"ESP32 conectado exitosamente!\"}";
    server.send(200, "application/json", jsonResponse);
  });

  /*
  Respuesta a peticion retornando info sobre distintos sensores o estados
  Prueba con Sensor de Temperatura DS18B20
  */
  server.on("/temperature", HTTP_GET, []() {
    sensor.requestTemperatures(); 
    float temperature = sensor.getTempCByIndex(0);

    String jsonResponse;
    
    if (isnan(temperature)) {
        jsonResponse = "{\"status\":\"error\",\"message\":\"Failed to read from DHT sensor!\"}";
        server.send(500, "application/json", jsonResponse);
    } else {
        jsonResponse = "{\"status\":\"success\",\"temperature\":" + String(temperature) + "}";
        server.send(200, "application/json", jsonResponse);
    }

  });											
  server.begin();
}

String getHTML() {
  return "<!DOCTYPE html><html><head><style>"
         "body {"
         "    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;"
         "    font-size: 1rem;"
         "    line-height: 1.5;"
         "    color: #212529;"
         "    background-color: #f8f9fa;"
         "    margin: 0;"
         "    padding: 1rem;"
         "}"
         ".container {"
         "    max-width: 500px;"
         "    margin: 0 auto;"
         "}"
         ".form-group {"
         "    margin-bottom: 1rem;"
         "}"
         "label {"
         "    display: inline-block;"
         "    margin-bottom: 0.5rem;"
         "}"
         "input[type='text'],"
         "input[type='password'] {"
         "    display: block;"
         "    width: 100%;"
         "    padding: 0.375rem 0.75rem;"
         "    font-size: 1rem;"
         "    font-weight: 400;"
         "    line-height: 1.5;"
         "    color: #495057;"
         "    background-color: #fff;"
         "    background-clip: padding-box;"
         "    border: 1px solid #ced4da;"
         "    border-radius: 0.25rem;"
         "    box-shadow: inset 0 0.075rem 0.15rem rgba(0, 0, 0, 0.075);"
         "    transition: border-color 0.15s ease-in-out, box-shadow 0.15s ease-in-out;"
         "}"
         "input[type='text']:focus,"
         "input[type='password']:focus {"
         "    color: #495057;"
         "    background-color: #fff;"
         "    border-color: #80bdff;"
         "    outline: 0;"
         "    box-shadow: 0 0 0 0.2rem rgba(0, 123, 255, 0.25);"
         "}"
         "button[type='submit'] {"
         "    display: inline-block;"
         "    font-weight: 400;"
         "    color: #fff;"
         "    text-align: center;"
         "    vertical-align: middle;"
         "    user-select: none;"
         "    background-color: #007bff;"
         "    border: 1px solid #007bff;"
         "    padding: 0.375rem 0.75rem;"
         "    font-size: 1rem;"
         "    line-height: 1.5;"
         "    border-radius: 0.25rem;"
         "    transition: color 0.15s ease-in-out, background-color 0.15s ease-in-out, border-color 0.15s ease-in-out, box-shadow 0.15s ease-in-out;"
         "}"
         "button[type='submit']:hover {"
         "    color: #fff;"
         "    background-color: #0056b3;"
         "    border-color: #004085;"
         "}"
         "button[type='submit']:focus {"
         "    color: #fff;"
         "    background-color: #0056b3;"
         "    border-color: #004085;"
         "    box-shadow: 0 0 0 0.2rem rgba(0, 123, 255, 0.5);"
         "}"
         "</style></head><body>"
         "<div class='container'>"
         "<h1>Configura tu WiFi</h1>"
         "<form action='/' method='POST'>"
         "<div class='form-group'>"
         "<label for='ssid'>SSID:</label>"
         "<input type='text' id='ssid' name='ssid' required>"
         "</div>"
         "<div class='form-group'>"
         "<label for='password'>Password:</label>"
         "<input type='password' id='password' name='password' required>"
         "</div>"
         "<button type='submit'>Guardar</button>"
         "</form>"
         "</div>"
         "</body></html>";
}

void writeStringToEEPROM(int addr, String data) {
  int len = data.length();
  for (int i = 0; i < len; i++) {
    EEPROM.write(addr + i, data[i]);
  }
  for (int i = len; i < 32; i++) {
    EEPROM.write(addr + i, 0);
  }
}

String readStringFromEEPROM(int addr) {
  char data[32];
  for (int i = 0; i < 32; i++) {
    data[i] = EEPROM.read(addr + i);
  }
  return String(data);
}

void clearEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("EEPROM borrada");
}


