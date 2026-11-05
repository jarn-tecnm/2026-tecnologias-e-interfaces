#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// ---------- CONFIGURACIÓN ----------

const char* WIFI_NOMBRE = "ALEXRAMOS 9879";
const char* WIFI_CLAVE = "1234567890.";

// IP de la computadora donde se ejecuta la API.
// No agregar "/" al final.
const char* API_URL = "http://192.168.1.188:5001";

// Debe existir previamente en la tabla Tarjetas.
const int ID_TARJETA = 1;

// Sensor ultrasónico HC-SR04.
const int PIN_TRIGGER = 23;
const int PIN_ECHO = 22;

// Entrada del módulo de relevador.
const int PIN_RELE = 21;

// true: el relevador se activa con LOW.
// false: el relevador se activa con HIGH.
const bool RELE_ACTIVO_EN_LOW = true;

// Distancias desde el sensor hasta el agua.
// Cambiar por las medidas reales del tanque.
const float DISTANCIA_VACIO_CM = 200.0f;
const float DISTANCIA_LLENO_CM = 10.0f;

// Tiempos en milisegundos.
const uint32_t INTERVALO_NIVEL_MS = 30000;
const uint32_t ESPERA_SOLICITUD_MS = 5000;
const uint32_t REINTENTO_WIFI_MS = 10000;

// ---------- VARIABLES DEL SISTEMA ----------

// Permite conservar el Id procesado después de reiniciar.
Preferences memoria;

bool memoriaDisponible = false;
char claveOrden[16];

uint32_t ultimaSolicitudProcesada = 0;
uint32_t inicioUltimoCiclo = 0;
uint32_t finIntentoEnvio = 0;
uint32_t ultimoIntentoWifi = 0;

bool consultaPendiente = false;

// ---------- CONTROL DE LA BOMBA ----------

void controlarBomba(bool encender)
{
  if (encender)
  {
      digitalWrite(PIN_RELE, LOW);
      Serial.println("Encender");
  }
  else
  {
      digitalWrite(PIN_RELE, HIGH);
      Serial.println("Apagar");
  }
}

// ---------- LECTURA DEL SENSOR ----------

bool leerNivel(float& porcentaje)
{
  if (DISTANCIA_VACIO_CM <= DISTANCIA_LLENO_CM)
  {
    Serial.println("Error: revisar las distancias de calibracion.");
    return false;
  }

  digitalWrite(PIN_TRIGGER, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIGGER, HIGH);
  delayMicroseconds(10);

  digitalWrite(PIN_TRIGGER, LOW);

  // Esperar como máximo 30 milisegundos por el eco.
  unsigned long duracion = pulseIn(PIN_ECHO,HIGH,30000UL);

  if (duracion == 0)
  {
    Serial.println("Sensor sin eco. No se enviara una lectura falsa.");
    return false;
  }

  // El sonido hace un recorrido de ida y vuelta.
  float distanciaCm = duracion * 0.0343f / 2.0f;

  if (distanciaCm < 2.0f || distanciaCm > 400.0f)
  {
    Serial.println("Distancia fuera del rango previsto para HC-SR04.");

    return false;
  }

  porcentaje = 100.0f * (DISTANCIA_VACIO_CM - distanciaCm) / (DISTANCIA_VACIO_CM - DISTANCIA_LLENO_CM);

  // Mantener el porcentaje entre 0 y 100.
  porcentaje = constrain(porcentaje, 0.0f, 100.0f);

  Serial.printf("Distancia: %.2f cm | Nivel: %.2f %%\n", distanciaCm, porcentaje);

  return true;
}

// ---------- ENVÍO DEL NIVEL ----------

void enviarNivel()
{
  float porcentaje;

  if (!leerNivel(porcentaje))
  {
    return;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Sin Wi-Fi: lectura no enviada.");
    return;
  }

  WiFiClient cliente;
  HTTPClient http;

  http.setConnectTimeout(3000);
  http.setTimeout(4000);

  String url = String(API_URL) + "/api/niveles";

  if (!http.begin(cliente, url))
  {
    Serial.println("No se pudo preparar el envio HTTP.");
    return;
  }

  http.addHeader(
      "Content-Type", "application/json"
  );

  // La API admite como máximo dos decimales.
  char cuerpo[96];

  snprintf(
      cuerpo,
      sizeof(cuerpo),
      "{\"nivel\":%.2f,\"idTarjeta\":%d}",
      porcentaje,
      ID_TARJETA
  );

  int codigo = http.POST(String(cuerpo));

  Serial.printf("Enviar nivel -> HTTP %d\n", codigo);

  if (codigo != 201)
  {
    if (codigo > 0)
    {
      Serial.println(http.getString());
    }
    else
    {
      Serial.println(
          HTTPClient::errorToString(codigo)
      );
    }
  }

  http.end();
}

// ---------- CONSULTA DE SOLICITUDES ----------

void consultarSolicitud()
{
  if (!memoriaDisponible)
  {
    Serial.println(
        "Sin memoria persistente: no se ejecutaran ordenes."
    );

    return;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Sin Wi-Fi: no se pudo consultar la solicitud.");
    return;
  }

  WiFiClient cliente;
  HTTPClient http;

  http.setConnectTimeout(3000);
  http.setTimeout(4000);

  String url =
      String(API_URL) +
      "/api/solicitudes/ultimo?idTarjeta=" +
      String(ID_TARJETA);

  if (!http.begin(cliente, url))
  {
    Serial.println("No se pudo preparar la consulta HTTP.");
    return;
  }

  int codigo = http.GET();

  // La API devuelve 204 si todavía no existen solicitudes.
  if (codigo == 204)
  {
    Serial.println("Todavia no hay solicitudes para esta tarjeta.");
    http.end();
    return;
  }

  if (codigo != 200)
  {
    Serial.printf("Consultar solicitud -> HTTP %d\n", codigo);
    http.end();
    return;
  }

  String respuesta = http.getString();
  http.end();

  JsonDocument documento;

  DeserializationError error = deserializeJson(documento,respuesta);

  if (error || !documento["id"].is<int>() || !documento["idTipoOrden"].is<int>() || !documento["idTarjeta"].is<int>())
  {
    Serial.println("Respuesta JSON invalida o incompleta.");
    return;
  }

  int idRecibido = documento["id"].as<int>();
  int tipoOrden = documento["idTipoOrden"].as<int>();
  int tarjeta = documento["idTarjeta"].as<int>();

  if (idRecibido <= 0 || tarjeta != ID_TARJETA || (tipoOrden != 0 && tipoOrden != 1))
  {
    Serial.println("Solicitud invalida: no se ejecuta.");
    return;
  }

  // Ignorar solicitudes que ya se procesaron.
  if (static_cast<uint32_t>(idRecibido) <= ultimaSolicitudProcesada)
  {
    Serial.printf("Solicitud %d ya procesada; se ignora.\n", idRecibido);
    return;
  }

  // 1 = encender; 0 = apagar.
  controlarBomba(tipoOrden == 1);

  // Guardar el Id en memoria persistente.
  size_t bytesGuardados = memoria.putUInt(claveOrden, static_cast<uint32_t>(idRecibido));

  if (bytesGuardados != sizeof(uint32_t))
  {
    Serial.println(
        "No se pudo guardar el Id. "
        "Se apaga la salida y se reintentara "
        "el siguiente ciclo."
    );

    controlarBomba(false);
    return;
  }

  ultimaSolicitudProcesada = static_cast<uint32_t>(idRecibido);

  Serial.printf(
      "Solicitud %d procesada: salida de bomba %s.\n",
      idRecibido,
      tipoOrden == 1 ? "ENCENDIDA" : "APAGADA"
  );
}

// ---------- INICIALIZACIÓN ----------

void setup()
{
  Serial.begin(115200);

  // Preparar la salida apagada antes de habilitar el pin.
  digitalWrite(PIN_RELE, RELE_ACTIVO_EN_LOW ? HIGH : LOW);

  pinMode(PIN_RELE, OUTPUT);
  controlarBomba(false);

  pinMode(PIN_TRIGGER, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  digitalWrite(PIN_TRIGGER, LOW);

  // Usar una clave distinta para cada Id de tarjeta.
  snprintf(claveOrden, sizeof(claveOrden), "id-%d", ID_TARJETA);

  memoriaDisponible = memoria.begin("tanque", false);

  if (memoriaDisponible)
  {
    ultimaSolicitudProcesada = memoria.getUInt(claveOrden, 0);
  }

  Serial.printf("Ultima solicitud procesada: %lu\n", static_cast<unsigned long>(ultimaSolicitudProcesada));

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_NOMBRE, WIFI_CLAVE);

  Serial.println("Conectando a Wi-Fi...");

  uint32_t inicioConexion = millis();

  // Esperar hasta 15 segundos al iniciar.
  while (WiFi.status() != WL_CONNECTED && millis() - inicioConexion < 15000UL)
  {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("IP de ESP32: ");
    Serial.println(WiFi.localIP());
  }

  // Primera lectura inmediata.
  // Las siguientes se realizan cada 60 segundos.
  inicioUltimoCiclo = millis() - INTERVALO_NIVEL_MS;
}

// ---------- CICLO PRINCIPAL ----------

void loop()
{
  uint32_t ahora = millis();

  // Reintentar la conexión si se pierde el Wi-Fi.
  if (WiFi.status() != WL_CONNECTED && ahora - ultimoIntentoWifi >= REINTENTO_WIFI_MS)
  {
    ultimoIntentoWifi = ahora;
    WiFi.reconnect();
  }

  // Consultar la solicitud cinco segundos después de terminar el intento de envío del nivel.
  if (consultaPendiente && millis() - finIntentoEnvio >= ESPERA_SOLICITUD_MS
  )
  {
    consultaPendiente = false;
    consultarSolicitud();
  }

  // Iniciar una nueva medición cada 60 segundos.
  if (!consultaPendiente && millis() - inicioUltimoCiclo >= INTERVALO_NIVEL_MS)
  {
    inicioUltimoCiclo = millis();

    enviarNivel();

    // Se programa la consulta aunque falle el sensor o el envío, para poder recibir una orden de apagado.
    finIntentoEnvio = millis();
    consultaPendiente = true;
  }

  delay(10);
}