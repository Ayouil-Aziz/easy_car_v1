//-------------------------------
//Created By Aziz Ayoujil
//Email:ayoujil.aziz0@gmail.com
//-------------------------------
//
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>


WiFiClient client;
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>
<h2>By Aziz Ayoujil<h2>
<h3> Easy car v 0.1</h3>

<form action="/gear">
  data:<br>
  <input type="text" name="data" value="Easy car">
  <br>
 
</form> 

</body>
</html>
)=====";

const char* host = "172.20.10.3";
const char* update_path = "/update";
const char* update_username = "admin";
const char* update_password = "admin";
const char* ssid = "EasyCar";
const char* password = "azizEasy@car";
String header;

// Auxiliary variables to store the current output state
String output5State = "zri";
// Assign output variables to GPIO pins
//const int output5 = 1;

const int buttonPin = 4;   
int buttonState = 0;        
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
String  command =""; // Command received from Android device

int igacc = 15;
int lights = 13;
int ig = 12;

int lock = 0;
int windows= 2; 
int unlock = 14;

ESP8266WebServer httpServer(81);

WiFiServer server(80);

ESP8266HTTPUpdateServer httpUpdater;


void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 httpServer.send(200, "text/html", s); //Send web page
}

void handleForm() {
 String data = httpServer.arg("data"); 
 httpServer.send(200, "text/html"); //Send web page
if (data =="automode") {
   automode();
    }
else if (data=="manual"){
    manual();
    }
}

  
 void Start (){
 
  if (digitalRead(buttonPin) ==HIGH && digitalRead(igacc)==LOW)digitalWrite(igacc, HIGH),delay (7000),digitalWrite(ig,HIGH),delay(1000),digitalWrite(ig,LOW),digitalWrite(unlock,HIGH),delay(500),digitalWrite(unlock,LOW);


  }
   void Stop (){
    if (digitalRead(buttonPin) ==HIGH && digitalRead(igacc)==HIGH )digitalWrite(lock,HIGH),delay(500),digitalWrite(lock,LOW),digitalWrite(igacc, LOW),restarta();

  }
void automode(){
  long rssi = WiFi.RSSI();
  Serial.print("RSSI: ");
  Serial.println(rssi);
  delay(500);
 

  if (rssi <= -20 & rssi >= -70) {
       Start();
       

  }
 else if (rssi <= -80 & rssi >= -90) {
       Stop();

       
  } 
   else if (rssi >=0) {
       Stop();
              delay(1000);
              restarta();

       
  } 


  
  
  }




void setup()
{
  
  Serial.begin(115200);
 
  pinMode(ig, OUTPUT); 
  pinMode(igacc, OUTPUT);  
  pinMode(lock, OUTPUT);  
  pinMode(unlock, OUTPUT);  
  pinMode(lights, OUTPUT);  
  pinMode(windows, OUTPUT);
  pinMode(buttonPin, INPUT);


  digitalWrite(ig,LOW);
  digitalWrite(igacc,LOW);
  digitalWrite(lock,LOW);
  digitalWrite(unlock,LOW);
  digitalWrite(lights,LOW);
  digitalWrite(windows,LOW);
  WiFi.softAPdisconnect (true);

  connectWiFi();
  server.begin();

  httpServer.begin();

  httpServer.on("/", handleRoot);      //Which routine to handle at root location
  httpServer.on("/action_page", handleForm); //form action is handled here
  MDNS.begin(host);
  //TạhttpSo server
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

  MDNS.addService("http", "tcp", 81);
  //Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);

}
 
void loop()
{
httpServer.handleClient();
handleForm();
client.stop();
delay(100);



}
 
/* connecting WiFi */
void connectWiFi()
{
  Serial.println("Connecting to WIFI");
  WiFi.begin(ssid, password);
  while ((!(WiFi.status() == WL_CONNECTED)))
  {
    delay(300);
    Serial.print("..");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("NodeMCU Local IP is : ");
  Serial.print((WiFi.localIP()));
}
 
/* check command received from Android Device */
String checkClient (void)
{
  while(!client.available()) delay(1); 
  String request = client.readStringUntil('\r');
  request.remove(0, 5);
  request.remove(request.length()-9,9);
  return request;

}
 
/* send command echo back to android device */
void sendBackEcho(String echo)
{
  client.println("HTTP/1.1 200 OK ");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println(echo);
  client.println("</html>");
  client.stop();
  delay(1);
}

 




void restarta(){

delay(1000);

  ESP.reset();
  }
void manual(){
   
    client = server.available();
    if (!client) return; 
command = checkClient ();
 
         if (command == "start"  || command == "start"    || command == "démarrer" || command == "que" || command == "quel" || command == "coeur" )Start();
    else if (command == "stop" || command == "stop" || command == "arrêter" || command == "6" || command == "merci" || command == "jeune")Stop();
    
    else if (command == "unlock"  || command == "personne"   || command == "argent"  || command == "ouvrez"  || command == "ouvrir" )      digitalWrite(unlock,1), delay(500),digitalWrite(unlock,0); 
    else if (command == "lock" || command == "argile" ||command == "jeux" || command == "gilles" ||command == "ferme"  || command == "algérie" )digitalWrite(lock,1), delay(500),digitalWrite(lock,0);   /* add windows here*/
    
    else if (command == "lighton"  || command == "acide"    || command == "light") digitalWrite(lights,1);
    else if (command == "lightoff" || command == "si" || command == "light off")  digitalWrite(lights,0);
    
    else if (command == "dashon"  || command == "tableau"    || command == "dashboard")digitalWrite(igacc,1),delay(500),digitalWrite(igacc,1);
    else if (command == "dashoff" || command == "si" || command == "off")digitalWrite(igacc,0),delay(500),digitalWrite(igacc,1);
    
    else if (command == "windowup" || command == "window" || command == "sergent")digitalWrite(windows,1),delay(2000),digitalWrite(windows,0);
    else if (command == "lockandwin1" || command == "window" || command == "sergent")digitalWrite(igacc,1),digitalWrite(lock,1), delay(500),digitalWrite(lock,0),digitalWrite(windows,1),digitalWrite(windows,1),delay(8000),digitalWrite(windows,0);
    gear();



    sendBackEcho(command); // send command echo back to android device
    command = "";
  }  
 
void gear(){
  
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
//        header = client.readStringUntil('\r');
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
       header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            Serial.println(header);
            
            // turns the GPIOs on and off
              buttonState = digitalRead(buttonPin);

            if (header.indexOf("GET /5/on")  && buttonState == HIGH) {
              Serial.println("GPIO 5 on");
              output5State = "orzri";
              
              //digitalWrite(output5, HIGH);
            } else if (header.indexOf("GET /5/off")  && buttonState == LOW) {
              Serial.println("GPIO 5 off");
              output5State = "zri";
             // digitalWrite(output5, LOW);
           
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP8266 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 5  
            client.println("<p>GPIO 5 - State" + output5State + "</p>");
            // If the output5State is off, it displays the ON button       
            if (output5State=="zri") {
              client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            
            
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
} 
