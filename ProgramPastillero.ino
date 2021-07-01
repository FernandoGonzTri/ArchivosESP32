#include <HTTPClient.h>       //Comunicacion HTTP
#include <WiFi.h>             //Conexion WiFi
#include <PubSubClient.h>     //Comunicacion MQTT
#include <Time.h>             //Configuracion tiempo
#include <TimeLib.h>
#include <BluetoothSerial.h>  //Comunicacion Bluetooth

BluetoothSerial SerialBT;

char ssid[15];
char password[15];

//const char* ssid = "MiFibra-AA10";
//const char* password =  "z4HWWQEh";

String url = "http://pastilleroesp32.ddns.net";

const char* mqtt_server = "pastilleroesp32.ddns.net";
const int mqttPort = 1883;

char usuario[15];
char passworduser[15];

WiFiClient espClient;
PubSubClient client(espClient);

// Declaramos la variable del tipo time_t
time_t fecha;

//Antirebote
const int timeThreshold = 150;
long startTime = 0;

//Inicializacion PIN
//Pin boton
int pinBoton=4;

//Pines motor
int pinIN4=17;
int pinIN3=5;
int pinIN2=18;
int pinIN1=19;

//Pin zumbador
int BUZZER=27;

//Pin led RGB
int red = 12;     
int green = 13;   
int blue = 14;    


//Inicializacion de variables
int posMotor = 0;      // Posicion de 0 a 360 grados 
int boton = 0;     // Estado del boton
int tomaRealizada = 0;   // Si se ha tomado la toma 0 - NO y 1 - SI

//Horarios
int hora_desayuno_min=7, minuto_desayuno_min=0, hora_desayuno_max=12, minuto_desayuno_max=30;
int hora_almuerzo_min=13, minuto_almuerzo_min=0, hora_almuerzo_max=16, minuto_almuerzo_max=0;
int hora_merienda_min=17, minuto_merienda_min=0, hora_merienda_max=20, minuto_merienda_max=0;
int hora_cena_min=20, minuto_cena_min=30, hora_cena_max=23, minuto_cena_max=30;

int horario_desayuno=0;
int horario_almuerzo=0;
int horario_merienda=0;
int horario_cena=0;

int pastillas_desayuno=0, pastillas_almuerzo=0, pastillas_merienda=0, pastillas_cena=0;

bool tomaCorrecta;

//Funciones
void MovMotor(int pos1);        //Movimiento del motor
void MostrarFecha();            //Mostrar la fecha
void enviarDatos();             //Enviar datos al servidor
void comprobarTomas();          //Comprobar si se debe tomar la toma

void IRAM_ATTR pulsacionBoton(){
  if (millis() - startTime > timeThreshold){
    Serial.println("Boton pulsado");
    boton=1;
    startTime = millis();
  }
}


void mensajesMQTT(char* topic, byte* payload, unsigned int length){

  String topicAdd="/ESP32/";
  topicAdd.concat(usuario);
  topicAdd.concat("/AddPastillas");
  String topicDelete="/ESP32/";
  topicDelete.concat(usuario);
  topicDelete.concat("/DeletePastillas");

  if(String(topic).equals(topicAdd)){
    if(payload[0]=='D'){
      Serial.println("Pastilla mañana añadida");
      pastillas_desayuno++;  
    }
    if(payload[0]=='A'){
      Serial.println("Pastilla almuerzo añadida");
      pastillas_almuerzo++;  
    }
    if(payload[0]=='M'){
      Serial.println("Pastilla merienda añadida");
      pastillas_merienda++;  
    }
    if(payload[0]=='C'){
      Serial.println("Pastilla cena añadida");
      pastillas_cena++;  
    }
  }

  if(String(topic).equals(topicDelete)){
    if(payload[0]=='D'){
      Serial.println("Pastilla mañana eliminada");
      pastillas_desayuno--;  
    }
    if(payload[0]=='A'){
      Serial.println("Pastilla almuerzo eliminada");
      pastillas_almuerzo--;  
    }
    if(payload[0]=='M'){
      Serial.println("Pastilla merienda eliminada");
      pastillas_merienda--;  
    }
    if(payload[0]=='C'){
      Serial.println("Pastilla cena eliminada");
      pastillas_cena--;  
    }
  }
  sonidoZumbador(1);
  setHorario();
}


void conexionBluetooth(){
  int fin=0;
  int cont=0;
  char dato = ' ';
  Serial.println("Escuchando por bluetooth");
  while(fin!=4){
    if(SerialBT.available()){
        dato = SerialBT.read();
        if(dato == '\n'){
          if(fin==0){
            ssid[cont]='\0';
            fin++; 
            cont=0;
          }else if(fin==1){
            password[cont]='\0';
            fin++;
            cont=0;  
          }else if(fin==2){
            usuario[cont]='\0';
            fin++;
            cont=0;  
          }else{
            passworduser[cont]='\0';
            fin++;
            cont=0;
          }
        }else{
          if(fin==0){
            ssid[cont]=dato;
          }else if(fin==1){
            password[cont]=dato;
          }else if(fin==2){
            usuario[cont]=dato;  
          }else{
            passworduser[cont]=dato;
          }
          cont++;
        }
        
        //Serial.print(dato);
    }  
  }
}

void conexionWifi(){
  //Intento de conexion wifi
  WiFi.begin(ssid, password);

  Serial.print("Conectando...");
  while (WiFi.status() != WL_CONNECTED) { //Comprobar la conexión
    delay(500);
    Serial.print(".");
  }

  Serial.print("Conectado con éxito, mi IP es: ");    //CONECTADO
  Serial.println(WiFi.localIP());  
}

void conexionMQTT() {
  while (!client.connected()) {
    Serial.println("Conectando a MQTT...");
    // Attempt to connect
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(),usuario,passworduser)){
      //Suscripcion a canales
      String topicAdd="/ESP32/";
      topicAdd.concat(usuario);
      topicAdd.concat("/AddPastillas");
      char cad1[topicAdd.length()+1];
      topicAdd.toCharArray(cad1,sizeof(cad1));

      Serial.println(cad1);
      client.subscribe(cad1);

      
      String topicDelete="/ESP32/";
      topicDelete.concat(usuario);
      topicDelete.concat("/DeletePastillas");
      char cad2[topicDelete.length()+1];
      topicDelete.toCharArray(cad2,sizeof(cad2));
      
      client.subscribe(cad2);

      Serial.println("Conectado");
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Se intentara de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  //Pin del buffer
  pinMode(BUZZER, OUTPUT);   //Pin 14

  //Pin led RGB
  pinMode (red, OUTPUT);
  pinMode (green, OUTPUT);
  pinMode (blue, OUTPUT);

  colorLed("verde");
  //Bluetooth
  SerialBT.begin("PastilleroInteligente");
  conexionBluetooth();
  sonidoZumbador(1);


  Serial.print("WiFi: ");
  Serial.println(ssid);
  Serial.print("Contraseña: ");
  Serial.println(password);
  Serial.print("Hola ");
  Serial.println(usuario);
  Serial.println(passworduser);

  colorLed("rojo");
  //Wifi
  conexionWifi();
  sonidoZumbador(2);


  //Pin del boton
  pinMode(pinBoton, INPUT_PULLUP);
  attachInterrupt(pinBoton,pulsacionBoton,FALLING);

  //Pin motor
  pinMode(pinIN4, OUTPUT);    // Pin 17 conectar a IN4
  pinMode(pinIN3, OUTPUT);    // Pin 5 conectar a IN3
  pinMode(pinIN2, OUTPUT);     // Pin 18 conectar a IN2
  pinMode(pinIN1, OUTPUT);     // Pin 19 conectar a IN1

  
  colorLed("azul");
  //MQTT
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(mensajesMQTT);
  conexionMQTT();

  // Establecemos la fecha
  ActualizarFecha();
  
  setHorario();

  colorLed("apagado");
  
}



void loop() {

  //Por si se desconecta de MQTT
  if (!client.connected()) {
    conexionMQTT();
  }
  client.loop();      //Comprueba la cola

  //Por si se desconecta de WiFi
  if(WiFi.status() != WL_CONNECTED) { //Comprobar la conexión
    conexionWifi();
  }

  //Espera a que se pulse el boton
  if(boton==1){
    
    boton=0;            //Estado del boton 0 de nuevo
    ActualizarFecha();
    MostrarFecha();
    tomaCorrecta=false;
    if(tomaDisponible()){
      tomaCorrecta=true;
      tomaRealizada=1;
      posMotor=posMotor+24;       //Aumento 24 grados  
      MovMotor(posMotor);
      if(posMotor==360){
        posMotor=0;  
      }
      colorLed("apagado"); 
    }else{
      Serial.println("Toma no correcta");  
    }
    
    enviarDatos();      //Envio la pulsacion del boton a la base de datos.  
  }
  
  comprobarTomas();

  encenderLed();

}

void enviarDatos(){
  //Envio de datos
    if(WiFi.status()== WL_CONNECTED){   //Comprobar si estamos conectados

      WiFiClient client;
      HTTPClient http;

      String serverPath = url + "/ESP32/pulsacion.php";
      
      if (http.begin(serverPath)){     //Inicamos conexion
        http.addHeader("Content-Type","application/x-www-form-urlencoded");

        String user=usuario;
        String datos_envio="usuario=" + user +"&password="+passworduser+"&boton="+tomaCorrecta;
     
        int httpResponseCode = http.POST(datos_envio);
      
        if (httpResponseCode>0) {
//        Serial.print("HTTP Response code: ");
//        Serial.println(httpResponseCode);
          String payload = http.getString();
          Serial.println(payload);
        }else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }
        // Libero recursos
        http.end();
          
      }else{
        Serial.println("Cliente no conectado"); 
      }
      

      
   }else{

     Serial.println("Error en la conexión WIFI");

   }  
}

int retardo=5;          // Tiempo de retardo en milisegundos (Velocidad del Motor)

void paso_izq() {        // Pasos a la izquierda
 digitalWrite(pinIN4, HIGH); 
 digitalWrite(pinIN3, HIGH);  
 digitalWrite(pinIN2, HIGH);  
 digitalWrite(pinIN1, LOW);  
   delay(retardo);
 digitalWrite(pinIN4, HIGH); 
 digitalWrite(pinIN3, HIGH);  
 digitalWrite(pinIN2, LOW);  
 digitalWrite(pinIN1, LOW);  
   delay(retardo);
 digitalWrite(pinIN4, HIGH); 
 digitalWrite(pinIN3, HIGH);  
 digitalWrite(pinIN2, LOW);  
 digitalWrite(pinIN1, HIGH);  
   delay(retardo);  
 digitalWrite(pinIN4, HIGH); 
 digitalWrite(pinIN3, LOW);  
 digitalWrite(pinIN2, LOW);  
 digitalWrite(pinIN1, HIGH);  
   delay(retardo);
 digitalWrite(pinIN4, HIGH); 
 digitalWrite(pinIN3, LOW);  
 digitalWrite(pinIN2, HIGH);  
 digitalWrite(pinIN1, HIGH);  
    delay(retardo); 
 digitalWrite(pinIN4, LOW); 
 digitalWrite(pinIN3, LOW);  
 digitalWrite(pinIN2, HIGH);  
 digitalWrite(pinIN1, HIGH);  
    delay(retardo);
 digitalWrite(pinIN4, LOW); 
 digitalWrite(pinIN3, HIGH);  
 digitalWrite(pinIN2, HIGH);  
 digitalWrite(pinIN1, HIGH);  
    delay(retardo); 
 digitalWrite(pinIN4, LOW); 
 digitalWrite(pinIN3, HIGH);  
 digitalWrite(pinIN2, HIGH);  
 digitalWrite(pinIN1, LOW);  
    delay(retardo);  
}
        
void apagado() {         // Apagado del Motor
 digitalWrite(pinIN4, LOW); 
 digitalWrite(pinIN3, LOW);  
 digitalWrite(pinIN2, LOW);  
 digitalWrite(pinIN1, LOW);  
}

int numero_pasos = 0;   // Valor en grados donde se encuentra el motor
 
void MovMotor(int pos1){
   Serial.print("Posición: ");
   Serial.println(pos1);
   int mov = (pos1 * 1.4222222222); // Ajuste de 512 vueltas a los 360 grados
   while (mov>numero_pasos){   // Girohacia la izquierda en grados
       paso_izq();
       numero_pasos = numero_pasos + 1;
   }

   if(pos1==360){ 
    do{
       Serial.print("Numero de pasos: ");
       Serial.println(numero_pasos);
       numero_pasos++;
       paso_izq();
    }while(numero_pasos!=512);
    numero_pasos=0;
   }

   apagado();         // Apagado del Motor para que no se caliente
}

void ActualizarFecha(){
  HTTPClient http;
  String serverPath = url + "/ESP32/hora.php";
  
  if (http.begin(serverPath)){     //Inicamos conexion
    int httpResponseCode = http.GET();
      
    if (httpResponseCode>0) {
      //Serial.print("HTTP Response code: ");
      //Serial.println(httpResponseCode);
      String payload = http.getString();      //Recibo la hora
      //Serial.println(payload);

      int pos=0;
      int inicio=0;
      int fin=payload.indexOf('/',inicio);
      String campo;
      // El proceso se repetira hasta que no haya mas barras (/).
      int vector[6];
      while (fin!=-1) {
        // Proceso el campo.
        campo = payload.substring(inicio, fin); 
        vector[pos]=campo.toFloat();
        pos++;
        inicio = fin+1;
        fin = payload.indexOf('/', inicio);
      }
      campo = payload.substring(inicio, fin);
      vector[pos]=campo.toFloat();

      
      setTime(vector[3], vector[4], vector[5], vector[0], vector[1], vector[2]);    //Actualizamos
     
    }else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Libero recursos
    http.end();
  } 
}

void MostrarFecha(){
  Serial.print("Hora: ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
 
  Serial.print("  ");
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.println(year());  
}


void comprobarTomas(){
  //Comprobar mañana
  if(hour()==hora_desayuno_min && minute()==minuto_desayuno_min && horario_desayuno==0 && pastillas_desayuno>0){
    horario_desayuno=1;
    tomaRealizada=0;
    Serial.println("Comienza el horario de toma de mañana");
    encenderLed();
  }
  if(hour()==hora_desayuno_max && minute()==minuto_desayuno_max && horario_desayuno==1){
    horario_desayuno=0;
    if(tomaRealizada==0){
      Serial.println("Aviso de la toma de mañana");
      sonidoZumbadorContinuo(); 
    }
    Serial.println("Termina el horario de toma de mañana");  
  }

  //Comprobar almuerzo
  if(hour()==hora_almuerzo_min && minute()==minuto_almuerzo_min && horario_almuerzo==0 && pastillas_almuerzo>0){
    horario_almuerzo=1;
    tomaRealizada=0;
    Serial.println("Comienza el horario de toma de almuerzo");  
  }
  if(hour()==hora_almuerzo_max && minute()==minuto_almuerzo_max && horario_almuerzo==1){
    horario_almuerzo=0;
    if(tomaRealizada==0){
      Serial.println("Aviso de la toma de almuerzo");
      sonidoZumbadorContinuo(); 
    }
    Serial.println("Termina el horario de toma de almuerzo");  
  }

  //Comprobar almuerzo
  if(hour()==hora_merienda_min && minute()==minuto_merienda_min && horario_merienda==0 && pastillas_merienda>0){
    horario_merienda=1;
    tomaRealizada=0;
    Serial.println("Comienza el horario de toma de merienda");  
  }
  if(hour()==hora_merienda_max && minute()==minuto_merienda_max && horario_merienda==1){
    horario_merienda=0;
    if(tomaRealizada==0){
      Serial.println("Aviso de la toma de merienda");
      sonidoZumbadorContinuo(); 
    }
    Serial.println("Termina el horario de toma de merienda");  
  }

  //Comprobar noche
  if(hour()==hora_cena_min && minute()==minuto_cena_min && horario_cena==0 && pastillas_cena>0){
    horario_cena=1;
    tomaRealizada=0;
    Serial.println("Comienza el horario de toma de cena");  
  }
  if(hour()==hora_cena_max && minute()==minuto_cena_max && horario_cena==1){
    horario_cena=0;
    if(tomaRealizada==0){
      Serial.println("Aviso de la toma de cena");
      sonidoZumbadorContinuo(); 
    }
    Serial.println("Termina el horario de toma de cena");  
  } 
}

boolean flag = true;
long previousMillis = 0;
long On = 500;
long Off = 500; 


void encenderLed(){
  if(tomaDisponible()){
    if (millis() < (previousMillis + On + Off)){
      flag = true;
    }else {
      flag = false;
      previousMillis = millis();
    }
    if (flag == true){
      if (millis() > (previousMillis + On)){
        colorLed("azul");
      }else{
        colorLed("apagado");
      }
    }
  }  
}

void sonidoZumbador(int cont){
  for(int i=0; i<cont; i++){
    digitalWrite(BUZZER,HIGH);
    delay(500);  
    digitalWrite(BUZZER,LOW);
    delay(500);  
  }

}

void sonidoZumbadorContinuo(){
  while(boton==0){
    digitalWrite(BUZZER,HIGH);
    delay(500);  
    digitalWrite(BUZZER,LOW);
    delay(500);  
  }

}

void colorLed(String color){
  if(color=="azul"){
    digitalWrite(green, LOW); digitalWrite (red,LOW);  digitalWrite(blue, HIGH);
  }
  if(color=="verde"){
    digitalWrite(green, HIGH); digitalWrite (red,LOW);  digitalWrite(blue, LOW);
  }
  if(color=="rojo"){
    digitalWrite(green, LOW); digitalWrite (red,HIGH);  digitalWrite(blue, LOW);
  }
  if(color=="amarillo"){
    digitalWrite(green, HIGH); digitalWrite (red,HIGH);  digitalWrite(blue, LOW);
  }
  if(color=="blanco"){
    digitalWrite(green, HIGH); digitalWrite (red,HIGH);  digitalWrite(blue, HIGH);
  }
  if(color=="celeste"){
    digitalWrite(green, HIGH); digitalWrite (red,LOW);  digitalWrite(blue, HIGH);
  }
  if(color=="rosa"){
    digitalWrite(green, LOW); digitalWrite (red,HIGH);  digitalWrite(blue, HIGH);
  }
  if(color=="apagado"){
    digitalWrite(green, LOW); digitalWrite (red,LOW);  digitalWrite(blue, LOW);
  }       
}

int tomaDisponible(){
  if((tomaRealizada==0) && (horario_desayuno==1 || horario_almuerzo==1 || horario_merienda==1 || horario_cena==1)){
      return 1;
  }
  return 0; 
}

void setHorario(){

   //Comprobar periodo de mañana
   if(hour()>hora_desayuno_min && hour()<hora_desayuno_max && pastillas_desayuno>0){
      horario_desayuno=1;
      Serial.println("Estamos en desayuno"); 
   }else if(hour()==hora_desayuno_min && minute()>minuto_desayuno_min && pastillas_desayuno>0){
      horario_desayuno=1;
      Serial.println("Estamos en desayuno"); 
   }else if(hour()==hora_desayuno_max && minute()<minuto_desayuno_max && pastillas_desayuno>0){
      horario_desayuno=1;
      Serial.println("Estamos en desayuno"); 
   }else{
      horario_desayuno=0; 
   }

   //Comprobar periodo de almuerzo
   if(hour()>hora_almuerzo_min && hour()<hora_almuerzo_max && pastillas_almuerzo>0){
      horario_almuerzo=1;
      Serial.println("Estamos en almuerzo"); 
   }else if(hour()==hora_almuerzo_min && minute()>minuto_almuerzo_min && pastillas_almuerzo>0){
      horario_almuerzo=1;
      Serial.println("Estamos en almuerzo"); 
   }else if(hour()==hora_almuerzo_max && minute()<minuto_almuerzo_max && pastillas_almuerzo>0){
      horario_almuerzo=1;
      Serial.println("Estamos en almuerzo"); 
   }else{
      horario_almuerzo=0; 
   }
   
   //Comprobar periodo de merienda
   if(hour()>hora_merienda_min && hour()<hora_merienda_max && pastillas_merienda>0){
      horario_merienda=1;
      Serial.println("Estamos en merienda"); 
   }else if(hour()==hora_merienda_min && minute()>minuto_merienda_min && pastillas_merienda>0){
      horario_merienda=1;
      Serial.println("Estamos en merienda"); 
   }else if(hour()==hora_merienda_max && minute()<minuto_merienda_max && pastillas_merienda>0){
      horario_merienda=1;
      Serial.println("Estamos en merienda"); 
   }else{
      horario_merienda=0;
   }

   //Comprobar periodo cena
   if(hour()>hora_cena_min && hour()<hora_cena_max && pastillas_cena>0){
      horario_cena=1;
      Serial.println("Estamos en merienda"); 
   }else if(hour()==hora_cena_min && minute()>minuto_cena_min && pastillas_cena>0){
      horario_cena=1;
      Serial.println("Estamos en merienda"); 
   }else if(hour()==hora_cena_max && minute()<minuto_cena_max && pastillas_cena>0){
      horario_cena=1;
      Serial.println("Estamos en merienda"); 
   }else{
      horario_cena=0;
   }

   if(horario_desayuno==0 && horario_almuerzo==0 && horario_merienda==0 && horario_cena==0){
      Serial.println("No estamos en ningun periodo de toma"); 
   }
}
