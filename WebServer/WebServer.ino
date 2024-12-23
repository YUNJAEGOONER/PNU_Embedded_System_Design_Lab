#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ArduinoJson.h>
#define MAX_CONNECTION_DELAY 10000
#define USART_DELAY 1000

// 네트워크 수동 입력
String ssid = "Galaxy anonymous";
String password = "leemin104";
String LocalIP = "";

// UART2 설정 - Sensor Device
#define UART2_RX 16
#define UART2_TX 17


// html 문서
const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet">
    <link href="https://getbootstrap.com/docs/5.3/assets/css/docs.css" rel="stylesheet">
    <title>IoT Mini Grow Farm</title>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js"></script>
    <style>
        .center-card {
            margin: 0 auto;
            margin-top: 20px;
        }
        .center-card .card-header {
            font-size: 1.5rem;
        }
        .list-group-item {
            display: flex;
            font-size: 1.2rem;
            justify-content: space-between;
            align-items: center;
        }
        .a {
            color : white;
            text-decoration: none;
        }
    </style>
</head>

<body class="p-3 m-0 border-0 bd-example m-0 border-0">
    <h1 class="text-center mb-5">IoT 스마트팜 생장 키트</h1>
    <div class="card center-card">
        <div class="card-header">현재 상태</div>
            <ul class="list-group list-group-flush">
                <li class="list-group-item">
                    <span>💡 밝기</span><span id="light"></span>
                </li>
                <li class="list-group-item">
                    <span>🌱 토양 습도</span><span id="moisture"></span>
                </li>
                <li class="list-group-item">
                    <span>🐳 물통 상태</span> <span id="level"></span>
                </li>
                <li class="list-group-item">
                    <span>🤖 현재 모드</span> <span id="mode"></span> 
                </li>
                <li class="list-group-item">
                    <span>🌞 LED 상태</span> <span id="led"></span>
                </li>
                <li class="list-group-item">
                    <span>🚪 문 상태</span> <span id="door"></span> 
                </li>
            </ul>
        </div>
    </div>



<script>
setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var target = document.getElementById('light');
            target.innerHTML = this.responseText;
        }
    };  
    xhttp.open('GET', '/light', true);
    xhttp.send();
}, 2000); // 2000 milliseconds

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) { 
            var target = document.getElementById('moisture');
            target.innerHTML = this.responseText;
        }
    };
    xhttp.open('GET', '/moisture', true);
    xhttp.send();
}, 500); // 500 milliseconds

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200){ 
            var target = document.getElementById('level')
            if (this.responseText == '0') target.innerHTML = '물 부족';
            else target.innerHTML = '물 충분';
        }
    };
    xhttp.open('GET', '/level', true);
    xhttp.send();
}, 500); // 500 milliseconds


setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var target = document.getElementById('mode');
            if (this.responseText == '0') target.innerHTML = '수동';
            else target.innerHTML = '자동';
        }
    };
    xhttp.open('GET', '/mode', true);
    xhttp.send();
}, 500); // 500 milliseconds


setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var target = document.getElementById('led');
            if (this.responseText == '0') target.innerHTML = '꺼짐';
            else target.innerHTML = '켜짐';
        }
    };
    xhttp.open('GET', '/led', true);
    xhttp.send();
}, 500); // 500 milliseconds


setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var target = document.getElementById('door');
            if (this.responseText == '1') target.innerHTML = '닫힘';
            else target.innerHTML = '열림';
        }
    };
    xhttp.open('GET', '/door', true);
    xhttp.send();
}, 500); // 500 milliseconds
</script>
</html>
)rawliteral";

// UART2로 받아온 데이터를 저장할 Json 문서
JsonDocument doc;

// 80번 포트로 서버 생성
AsyncWebServer server(80);

// "{\"mode\":%d, \"led\":%d, \"door\":%d, \"soil\":%d, \"light\":%d, \"level\":%d}\r\n",
        
// 초기화 함수
void initializeDoc() {
  // 진리값 : 0, 1
  doc["mode"] = 0; 
  doc["led"] = 0;
  doc["door"] = 0;
  doc["level"] = 0;

  // 수치값
  doc["soil"] = 0;
  doc["light"] = 0;
}


String readLight() { 
  int light = doc["light"].as<int>();
  return String(light);
}

String readSoil() {
  int soil = doc["soil"].as<int>();
  return String(soil);
}

// 진리값
String readLevel() { 
  int level = doc["level"].as<int>();
  return String(level);
}

// 기기 제어 함수
String readMode() {
  int mode = doc["mode"].as<int>();
  return String(mode);
}


String readLed() {
  int led = doc["led"].as<int>();
  return String(led);
}

String readDoor() {
  int door = doc["door"].as<int>();
  return String(door);
}

bool WiFiSetup() {
  WiFi.begin(ssid, password);
  Serial.print("연결 중.");

  int delayCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    delayCount += 500;
    Serial.print(".");
    if (delayCount > MAX_CONNECTION_DELAY) {
      Serial.println("연결 실패");
      return false;
    }
  }

  Serial.println("연결 성공");
  Serial.print("IP 주소: ");
  Serial.println(WiFi.localIP());
  LocalIP = WiFi.localIP().toString();
  for (int i=0; i<LocalIP.length(); ++i) {
    int ascii = LocalIP[i];
    Serial2.write(ascii);
    delayMicroseconds(USART_DELAY);
  }
  Serial2.write((int)'\r');
  delayMicroseconds(USART_DELAY);
  Serial2.write((int)'\n');
  delayMicroseconds(USART_DELAY);

  return true;
}

void executeCommand(const char* command) {
  for (int i = 0; i < strlen(command); i++) {
    int ascii = command[i];
    Serial2.write(ascii);
    delayMicroseconds(USART_DELAY);
  }
  
  Serial2.write((int)'\r');
  delayMicroseconds(USART_DELAY);
  Serial2.write((int)'\n');
  delayMicroseconds(USART_DELAY);
}




void ServerSetup() {
  // 루트 경로에 index_html 파일 전송
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/light", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readLight().c_str());
  });
  
  server.on("/moisture", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readSoil().c_str());
  });
  
  server.on("/level", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readLevel().c_str());
  });

  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readMode().c_str());
  });

  server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readLed().c_str());
  });

  server.on("/door", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDoor().c_str());
  });
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, UART2_RX, UART2_TX);
  initializeDoc();

  // WiFi 및 서버 설정
  
  if(WiFiSetup()) {
    ServerSetup();
    server.begin();
    Serial.println("Starting server!!");
  } else {
    Serial.println("서버 시작 실패");
  }
}


void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.replace("\r", "");
    Serial.println(data);
    deserializeJson(doc, data);
  }
}
