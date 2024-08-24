#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimedAction.h>

const char *ssid = "";
const char *password = "";

void blinkIT();
boolean isValidNumber(String str);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

TimedAction timedAction = TimedAction(60000, blinkIT);
WiFiServer server(80);

// Variable to store the HTTP request
String header;
boolean daylightSaving = true;  //false for winter, true for summer
int StartTime = 6;
int EndTime = 22;

// Auxiliar variables to store the current output state
int output12State = 1;
String opt;
// Assign output variables to GPIO pins
const int output12 = 12;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output12, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output12, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  timeClient.begin();
  timeClient.setTimeOffset(7200);
}

void loop() {
  if (daylightSaving) {
    timeClient.setTimeOffset(7200);
  } else {
    timeClient.setTimeOffset(10800);
  }
  WiFiClient client = server.available();  // Listen for incoming clients
  timeClient.update();
  timedAction.check();
  String myTime = timeClient.getFormattedTime();

  if (client) {                     // If a new client connects,
    Serial.println("New Client.");  // print a message out in the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out the serial monitor
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /12/on") >= 0) {
              //Serial.println(header);
              Serial.println("GPIO 12 on");
              output12State = 1;
              digitalWrite(output12, HIGH);
              client.println("ok");
              client.println();
              break;
            } else if (header.indexOf("GET /12/off") >= 0) {
              //Serial.println(header);
              Serial.println("GPIO 12 off");
              output12State = 0;
              digitalWrite(output12, LOW);
              client.println("ok");
              client.println();
              break;
            } else if (header.indexOf("GET /DayLight/off") >= 0) {
              Serial.println("DayLight off");
              daylightSaving = true;
              client.println("ok");
              client.println();
              break;
            } else if (header.indexOf("GET /DayLight/on") >= 0) {
              Serial.println("DayLight on");
              daylightSaving = false;
              client.println("ok");
              client.println();
              break;
            } else if (header.indexOf("GET /Time") >= 0) {
              Serial.println("Time Parameters");
              if (header.indexOf("StartTime=") && header.indexOf("EndTime=")) {
                int startParam = header.indexOf("StartTime=");
                int EndParam = header.indexOf("EndTime=");
                String strStartParam = header.substring((startParam + 10), (startParam + 12));
                int intStartParam = int(strStartParam.toInt());
                String strEndParam = header.substring((EndParam + 8), (EndParam + 10));
                int intEndParam = int(header.substring((EndParam + 8), (EndParam + 10)).toInt());
                if (isValidNumber(strStartParam) && isValidNumber(strEndParam)) {
                  if (intStartParam >= 0 && intEndParam <= 23) {
                    StartTime = intStartParam;
                    EndTime = intEndParam;
                    Serial.print("True Its A Num: ");
                    Serial.println(intStartParam);
                    Serial.print("And Also This: ");
                    Serial.println(intEndParam);
                  } else {
                    Serial.print("Not Valid Hour!!!");
                  }
                } else {
                  Serial.println(isDigit(intStartParam));
                }
              }
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"1800\" >");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");

            // Web Page Heading
            client.println("<script>function ControlLinks(e,t){var n=new XMLHttpRequest;document.getElementById(t).innerHTML;n.onreadystatechange=function(){4==this.readyState&&this.status},n.open(\"GET\",e,!0),n.send();setTimeout(() => {console.log(\"Bam! 5 seconds have passed.\");location.reload();}, 2000);}function addtt(){for(var e=document.getElementById(\"myTime\").innerHTML,t=[0,0,0],n=t.length,a=(e||\"\").split(\":\"),i=\"00:00:01\".split(\":\"),s=0;s<n;s++)a[s]=isNaN(parseInt(a[s]))?0:parseInt(a[s]),i[s]=isNaN(parseInt(i[s]))?0:parseInt(i[s]);for(s=0;s<n;s++)t[s]=a[s]+i[s];var r=t[0],o=t[1],d=t[2];if(d>=60){var l=d/60<<0;o+=l,d-=60*l}if(o>=60){var c=o/60<<0;r+=c,o-=60*c}document.getElementById(\"myTime\").innerHTML=(\"0\"+r).slice(-2)+\":\"+(\"0\"+o).slice(-2)+\":\"+(\"0\"+d).slice(-2)}setInterval((()=>{addtt()}),1e3);</script>");
            client.println("<body bgcolor=#006468><h1>AirFrash Control!!</h1>");
            client.println("<p><h2>Time: <div id=\"myTime\">" + myTime + "</div></h2></p>");
            if (output12State==1) { opt = "Bypass ";} else { opt = "Timer Based";};
            client.println("<p>GPIO 12 - State " + opt + "</p>");
            // If the output12State is off, it displays the ON button
            if (output12State == 0) {
              client.println("<p>Bypass Power:</p><p><button class=\"button\" onclick=\"ControlLinks('/12/on','BypassBTN');\"><div id=\"BypassBTN\">ON</div></button></p>");
            } else {
              client.println("<p>Bypass Power:</p><p><button class=\"button\" onclick=\"ControlLinks('/12/off','BypassBTN');\"><div id=\"BypassBTN\">OFF</div></button></p>");
            }
            if (daylightSaving) {
              client.println("<p>DayLight:</p><p><button class=\"button\" onclick=\"ControlLinks('/DayLight/on','DayBTN');\"><div id=\"DayBTN\">ON</div></button></a></p>");
            } else {
              client.println("<p>DayLight:</p><p><button class=\"button\" onclick=\"ControlLinks('/DayLight/off','DayBTN');\"><div id=\"DayBTN\">OFF</div></button></p>");
            }
            client.println("<p>Set when to start and when to stop</p>");
            client.println("<p>Example: StartTime - 6 EndTime - 22</p>");
            client.println("<p>Example: This will set poweron at 6am and will stop at 10pm</p>");
            client.println("<form action=\"/Time\" method=\"get\" target=\"_self\">");
            client.println("<label for=\"StartTime\">StartTime:</label>");
            client.println("<input type=\"text\" size=2 maxlength=2 id=\"StartTime\" name=\"StartTime\" value=\"" + String(StartTime) + "\"><br><br>");
            client.println("<label for=\"EndTime\">EndTime:</label>");
            client.println("<input type=\"text\" size=2 maxlength=2 id=\"EndTime\" name=\"EndTime\" value=\"" + String(EndTime) + "\"><br><br>");
            client.println("<input type=\"submit\" value=\"Submit\">");
            client.println("</form>");
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else {  // if you got a newline, then clear currentLine
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

void blinkIT() {
  timeClient.update();
  int currentHour = timeClient.getHours();
  if (output12State == 1) {
    digitalWrite(output12, HIGH);
  } else {
    if (currentHour >= StartTime && currentHour <= EndTime) {
      digitalWrite(output12, HIGH);
    } else {
      digitalWrite(output12, LOW);
    }
  }
  Serial.print("PowerOn Is in: ");
  Serial.print(opt);
  Serial.println(" State");
  //Serial.print(timeClient.getFormattedTime());
  //Serial.println("Waiting 30 mins");
  //delay(1800000);
}

boolean isValidNumber(String str) {
  for (byte i = 0; i < str.length(); i++) {
    if (isDigit(str.charAt(i))) return true;
  }
  return false;
}
