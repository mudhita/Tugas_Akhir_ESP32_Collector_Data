#include <painlessMesh.h>

#define LED             2

#define MESH_SSID       "whateverYouLike"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

#define UART2_TX        17
#define UART2_RX        16

#define BLINK_PERIOD    3000        // milliseconds until cycle repeat
#define BLINK_DURATION  100         // milliseconds LED is on for

Scheduler userScheduler;            // untuk mengontrol tugas pribadi Anda
painlessMesh mesh;

String stationNumber = "Receiver";  // ID dari stasiun penerima

bool onFlag = false;
Task blinkNoNodes;

HardwareSerial uart2(2);            // HardwareSerial untuk UART2

void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

void setup() {
  Serial.begin(115200);
  uart2.begin(115200, SERIAL_8N1, UART2_RX, UART2_TX);  // Inisialisasi UART2

  pinMode(LED, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG);                 // Set sebelum init() sehingga Anda bisa melihat pesan kesalahan

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Task untuk berkedip jumlah node
  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, [=]() {
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;
    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration()) {
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
      blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                 (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();
}

void loop() {
  mesh.update();
  digitalWrite(LED, !onFlag);
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Receiver: Data diterima dari %u msg=%s\n", from, msg.c_str());

  // Mengirim kembali data yang diterima melalui UART2
  uart2.println(msg);
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset berkedip
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  Serial.printf("--> Receiver: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> Receiver: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback() {
  Serial.printf("Receiver: Changed connections\n");
  // Reset berkedip
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  SimpleList<uint32_t> nodes = mesh.getNodeList();

  Serial.printf("Receiver: Num nodes: %d\n", nodes.size());
  Serial.printf("Receiver: Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Receiver: Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
