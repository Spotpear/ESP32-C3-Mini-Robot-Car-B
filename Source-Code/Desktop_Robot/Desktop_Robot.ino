#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include <esp_wifi.h>          // esp_wifi_set_max_tx_power()

/* ================= OLED ================= */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 8
#define OLED_SCL 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

#define LF   0
#define LB   1
#define RF   2
#define RB   3
#define STBY 10

/* ================= MOTOR SPEED (LEDC PWM) =================
 * 原来 4 根方向脚是纯 digitalWrite，只有"全速 / 停止"两态。
 * 要支持网页调速，改用 ESP32 的 LEDC：每个方向脚占一个通道，
 * 占空比 = 转速百分比。注意 ledcAttachPin() 之后这 4 个脚不能再
 * digitalWrite，所以 motorWifi() / MOTOR() 统一走 motorSet()。
 * -------------------------------------------------------- */
#define PWM_FREQ      1000                              // 1kHz，TB6612 常用
#define PWM_RES       8                                 // 8 位 → 占空 0..255
static const uint8_t MOTOR_PINS[4] = { LF, LB, RF, RB };  // LEDC 通道 0..3

volatile uint8_t g_speed      = 80;   // 转速 0..100%，默认 80%
volatile byte    g_manual_dir = 0;    // 当前手动方向，供 /speed 立即重新套用

static inline uint8_t speedDuty() {
  return (uint8_t)((255U * g_speed + 50U) / 100U);
}
static inline void motorSet(uint8_t lf, uint8_t lb, uint8_t rf, uint8_t rb) {
  ledcWrite(0, lf); ledcWrite(1, lb);
  ledcWrite(2, rf); ledcWrite(3, rb);
}

/* ================= VOICE MODULE: 天问 ASR PRO2.0 (UART1) =================
 * 电平：ASR PRO2.0 的 IO 是 3.3V，和 ESP32-C3 同为 3.3V，TX 可直连，
 *       不需要分压 / 电平转换（ESP32-C3 的 GPIO 不耐 5V，别接 5V 逻辑）。
 *
 * 接线：
 *   ASR PRO2.0 TX (UART1_TX = PA2；若用串口0 则是 PB5) -> ESP32-C3 GPIO4 (RX)
 *   ASR PRO2.0 GND                                     -> ESP32-C3 GND  ← 必须共地
 *   ESP32-C3 GPIO5                                     -> 可选：收到指令输出 6ms 脉冲
 *   模块 RX 可不接（本项目只收不发，单向通信）
 *
 * 供电：ASR PRO2.0 在识别/播报瞬间有电流尖峰，不要用 ESP32-C3 的 3V3
 *       取电（会把主控电压拉垮导致重启）。用独立电源按模块标注供电，
 *       并在模块电源脚就近并 100uF + 0.1uF 退耦。
 *
 * 波特率：天问Block 里 Serial / Serial1 常用 9600 8N1，必须和 VOICE_BAUD
 *       一致；若在天问里改成 115200，这里同步改。
 * ------------------------------------------------------------------------ */
#define VOICE_RX     4
#define VOICE_ACK    5
#define VOICE_BAUD   9600
#define VOICE_GAP_MS 30       // 一帧字节连续到达，静默 30ms 判定一帧结束
#define VOICE_DEBUG  0        // 改 1：USB 串口打印收到的原始字节/命令码，便于对帧

/* 1..4 与 motorWifi() 的方向码一致，方便直接复用。
 * 1..8 是原有指令；9..12 是本轮为天问 ASR PRO2.0 新增的指令。
 * 线上码用可打印 ASCII '1'..'9','A'..'C'（避开 \r \n \t），
 * 同时兼容旧版单字节十六进制 0x01..0x08。 */
enum VoiceCmd {
  V_NONE = 0,
  V_FORWARD = 1, V_BACK = 2, V_LEFT = 3, V_RIGHT = 4,
  V_DANCE = 5, V_SPIN = 6, V_WIGGLE = 7, V_SLEEP = 8,
  V_STOP = 9, V_CURIOUS_ON = 10, V_CURIOUS_OFF = 11, V_PARTY = 12
};
#define VOICE_CMD_MAX  V_PARTY

/* ================= WIFI ================= */
WebServer server(80);
DNSServer dnsServer;

/* ================= STATE ================= */
volatile bool manualActive = false;

/* ================= RANDOM MODE ================= */
enum RandomMode {
  RANDOM_OFF,
  RANDOM_SOFT,
  RANDOM_NORMAL
};

/* 上电默认「休眠」模式：仅关闭随机动作（loop 里不再触发 MOTOR）。
 * 与网页「休眠」按钮 /mode_off 语义完全一致 —— 眼睛仍睁眼/自动眨眼，
 * STBY 仍保持使能，网页方向键与语音指令照常可用。
 * 闭眼 + 关眨眼的完整休眠由语音 V_SLEEP / 网页「退下」触发，不上电强制。 */
volatile RandomMode randomMode = RANDOM_OFF;

/* ================= WIFI MOTOR ================= */
void motorWifi(byte c) {
  digitalWrite(STBY, HIGH);
  g_manual_dir = c;
  uint8_t d = speedDuty();
  switch (c) {
    case 1: motorSet(d,0,0,d);  break;    // 前进：两轮同进
    case 2: motorSet(0,d,d,0);  break;    // 后退：两轮同退
    case 3: motorSet(0,d,0,d);  break;    // 左转：LB+RB，与右转互补镜像（写法照右转）
    case 4: motorSet(d,0,d,0);  break;    // 右转：LF+RF
    default: motorSet(0,0,0,0); break;    // 0 / 未知 → 停止
  }
}

static inline bool due(uint32_t t) {
  return (int32_t)(millis() - t) >= 0;      // 用减法，millis() 回绕也正确
}

static void pulseSet(byte c, uint8_t d) {
  switch (c) {
    case 1: motorSet(0,d,0,d); break;
    case 2: motorSet(d,0,d,0); break;
    case 3: motorSet(0,d,d,0); break;
    case 4: motorSet(d,0,0,d); break;
    case 5: motorSet(0,d,0,0); break;
    case 6: motorSet(0,0,0,d); break;
    case 7: motorSet(d,0,0,0); break;
    case 8: motorSet(0,0,d,0); break;
    default: motorSet(0,0,0,0); break;
  }
}

struct PulseState {
  bool active; byte c, remain; uint16_t t1, t2; bool on; uint32_t nextMs;
};
static PulseState pulse = {false, 0, 0, 0, 0, false, 0};

// 注意：不再阻塞。只是"启动"一组脉冲，实际推进在 pulseUpdate()。
void MOTOR(byte c,int t1,int t2,int Time){
  if (Time <= 0) return;
  pulse.active = true;
  pulse.c = c;
  pulse.t1 = (uint16_t)t1;
  pulse.t2 = (uint16_t)t2;
  pulse.remain = (byte)Time;
  pulse.on = true;
  pulse.nextMs = millis() + t1;
  pulseSet(c, speedDuty());
}

void pulseUpdate() {
  if (!pulse.active || !due(pulse.nextMs)) return;
  if (pulse.on) {
    pulseSet(0, 0);                       // 进入断电阶段
    pulse.on = false;
    pulse.nextMs = millis() + pulse.t2;
  } else {
    pulse.remain--;
    if (pulse.remain == 0) {
      pulse.active = false;
      pulseSet(0, 0);
      return;
    }
    pulseSet(pulse.c, speedDuty());       // 每次重读转速，调速即时生效
    pulse.on = true;
    pulse.nextMs = millis() + pulse.t1;
  }
}

/* ---- 语音动作：静态步骤序列 ---- */
struct ActStep { byte dir; uint16_t ms; };   // dir 用 motorWifi() 的 1..4，0 = 停

static const ActStep ACT_DANCE[] = {
  {3,170},{4,170},{3,170},{4,170},{1,240},{2,240},{3,150},{4,150}
};
static const ActStep ACT_SPIN[] = {
  {3,520},{0,80},{3,520},{0,80},{3,520}
};
static const ActStep ACT_WIGGLE[] = {
  {3,90},{4,90},{3,90},{4,90},{3,90},{4,90}
};
/* 「跳个舞」：比 ACT_DANCE 更长的一段组合动作（左右摆 → 进退 → 左右转
 * → 碎步 → 收尾），约 3.9 秒。dir 仍用 motorWifi 的 1..4，0 = 停车。 */
static const ActStep ACT_PARTY[] = {
  {3,180},{4,180},{3,180},{4,180},
  {1,260},{0,90},{2,260},{0,90},
  {3,420},{0,120},{4,420},{0,120},
  {1,180},{2,180},{1,180},{2,180},
  {3,150},{4,150},{0,120}
};

struct SeqState {
  const ActStep* steps; byte count, index; uint32_t nextMs; bool active;
};
static SeqState seq = {nullptr, 0, 0, 0, false};

void seqStart(byte which) {
  const ActStep* s; byte n;
  switch (which) {
    case V_DANCE:  s = ACT_DANCE;  n = sizeof(ACT_DANCE)  / sizeof(ACT_DANCE[0]);  break;
    case V_SPIN:   s = ACT_SPIN;   n = sizeof(ACT_SPIN)   / sizeof(ACT_SPIN[0]);   break;
    case V_PARTY:  s = ACT_PARTY;  n = sizeof(ACT_PARTY)  / sizeof(ACT_PARTY[0]);  break;
    default:       s = ACT_WIGGLE; n = sizeof(ACT_WIGGLE) / sizeof(ACT_WIGGLE[0]); break;
  }
  pulse.active = false; pulseSet(0,0);
  seq.steps = s; seq.count = n; seq.index = 0;
  seq.active = true; seq.nextMs = millis();
}

void seqUpdate() {
  if (!seq.active || !due(seq.nextMs)) return;
  if (seq.index >= seq.count) {
    seq.active = false;
    motorWifi(0);                         // 动作结束自动停车
    return;
  }
  const ActStep& st = seq.steps[seq.index++];
  motorWifi(st.dir);
  seq.nextMs = millis() + st.ms;
}

static inline bool actionBusy() { return pulse.active || seq.active; }

void stopAction() {
  pulse.active = false;
  seq.active = false;
  pulseSet(0, 0);
}

static uint32_t g_ackOffMs     = 0;
static byte     g_voicePending = 0;
static uint32_t g_voiceLastMs  = 0;

void voiceSetup() {
  pinMode(VOICE_ACK, OUTPUT);
  digitalWrite(VOICE_ACK, LOW);
  Serial1.setRxBufferSize(256);              // 必须在 begin() 之前
  Serial1.begin(VOICE_BAUD, SERIAL_8N1, VOICE_RX, -1);   // TX 不用，-1 表示不映射
}

void voiceAckPulse() {
  digitalWrite(VOICE_ACK, HIGH);
  g_ackOffMs = millis() + 6;                 // 6ms 确认脉冲，非阻塞拉低
}

void voiceAckUpdate() {
  if (g_ackOffMs && due(g_ackOffMs)) {
    digitalWrite(VOICE_ACK, LOW);
    g_ackOffMs = 0;
  }
}

void voiceExecute(byte cmd) {
  if (cmd != V_SLEEP) {
    // 唤醒：恢复自动眨眼与 idle，再睁眼
    roboEyes.setAutoblinker(true);
    roboEyes.setIdleMode(true);
    roboEyes.open();
  }
  switch (cmd) {
    case V_FORWARD: case V_BACK: case V_LEFT: case V_RIGHT:
      stopAction();
      motorWifi(cmd);                        // 持续保持，与网页方向键一致
      break;
    case V_DANCE:  seqStart(V_DANCE);  break;
    case V_SPIN:   seqStart(V_SPIN);   break;
    case V_WIGGLE: seqStart(V_WIGGLE); break;
    case V_PARTY:  seqStart(V_PARTY);  break;   // 「跳个舞」：新增的组合舞蹈
    case V_STOP:
      stopAction();
      motorWifi(0);
      // 不关随机模式的话几秒后 loop() 又会自己触发动作，「不要动」就失效了
      randomMode = RANDOM_OFF;
      break;
    case V_CURIOUS_ON:
      randomMode = RANDOM_NORMAL;               // 等价网页「好动」
      break;
    case V_CURIOUS_OFF:
      randomMode = RANDOM_OFF;                 // 只关随机，不强制停车
      break;
    case V_SLEEP:
      stopAction();
      motorWifi(0);
      randomMode = RANDOM_OFF;              // 连随机扭动也一起停
      // 库里 autoblinker 每 3~4 秒调 blink()（= close()+open()），
      // 不关掉的话"退下"闭眼几秒后就会被自动睁眼
      roboEyes.setAutoblinker(false);
      roboEyes.setIdleMode(false);
      roboEyes.close();
      break;
  }
  /* 注意：这里不再回 ACK。voiceExecute() 现在也被网页 /act 复用，
     ACK 脉冲只应由串口收到的指令触发，放在 voicePoll() 里。 */
}

void voicePoll() {
  // 1) 先把硬件 FIFO 里本帧的字节全部取走，只记住最后一个有效命令码
  while (Serial1.available() > 0) {
    int b = Serial1.read();
#if VOICE_DEBUG
    Serial.printf("[voice] rx=%02X\r\n", (unsigned)b);
#endif
    byte cmd = 0;
    if      (b >= 0x01 && b <= 0x08)  cmd = (byte)b;                    // 旧版 16进制 01..08
    else if (b >= '1'  && b <= '9')   cmd = (byte)(b - '0');            // '1'..'9' → 1..9
    else if (b >= 'A'  && b <= 'C')   cmd = (byte)(10 + (b - 'A'));     // 'A'..'C' → 10..12
    if (cmd && cmd <= VOICE_CMD_MAX) {
      g_voicePending = cmd;
      g_voiceLastMs  = millis();
    }
  }

  // 2) 静默窗口结束才算一帧收完，只提交一次
  if (g_voicePending && (int32_t)(millis() - g_voiceLastMs) >= VOICE_GAP_MS) {
    byte cmd = g_voicePending;
    g_voicePending = 0;
#if VOICE_DEBUG
    Serial.printf("[voice] cmd=%d\r\n", (int)cmd);
#endif
    voiceExecute(cmd);
    voiceAckPulse();              // 仅对串口指令回「已接收」脉冲
  }
}

/* ================= WEB UI ================= */
void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>桌面机器人 / Desktop Robot</title>
<style>
body{
  margin:0;min-height:100vh;
  padding:16px 0;box-sizing:border-box;
  background:radial-gradient(circle at top,#0f2027,#000);
  color:#00ffe1;
  font-family:"Microsoft YaHei","PingFang SC",Arial,sans-serif;
  display:flex;align-items:center;justify-content:center;
}
.panel{
  width:300px;
  padding:20px;
  border-radius:18px;
  background:rgba(0,255,225,0.05);
  border:1px solid rgba(0,255,225,0.4);
  box-shadow:0 0 25px rgba(0,255,225,0.3);
}
h2{text-align:center;margin:0 0 14px;letter-spacing:1px;font-size:17px;line-height:1.4;}
.grid{
  display:grid;
  grid-template-columns:1fr 1fr 1fr;
  grid-template-rows:60px 60px 60px;
  gap:10px;
}
button{
  border:none;border-radius:12px;
  font-size:12px;font-weight:bold;
  line-height:1.25;
  padding:4px 2px;
  word-break:break-word;
  color:#00352f;
  background:linear-gradient(145deg,#0ff,#00b3a4);
}
.stop{background:linear-gradient(145deg,#ff5555,#aa0000);color:#fff;}
.empty{background:none;}

.mode{
  margin-top:12px;
  display:flex;
  gap:6px;
}
.mode button{
  flex:1;
  font-size:11px;
  line-height:1.2;
  opacity:0.6;
}
.mode button.active{
  opacity:1;
  background:linear-gradient(145deg,#00ff9c,#00c46a);
  box-shadow:0 0 12px rgba(0,255,180,0.8);
}

.act{
  margin-top:14px;
  padding-top:12px;
  border-top:1px solid rgba(0,255,225,0.25);
}
.act .cap{
  display:flex;justify-content:space-between;flex-wrap:wrap;
  font-size:10px;opacity:0.7;margin-bottom:8px;gap:6px;
}
.act .row{
  display:grid;
  grid-template-columns:1fr 1fr 1fr;
  gap:6px;
}
.act button{
  padding:8px 2px;font-size:10px;line-height:1.25;
  background:linear-gradient(145deg,#0ff,#00918a);
}
.act button.hot{
  background:linear-gradient(145deg,#ffd25a,#ff8a00);color:#3a2000;
}
.act button.sleep{
  background:linear-gradient(145deg,#8a7cff,#4b3fc0);color:#fff;
}

.speed{
  margin-top:14px;
  padding-top:12px;
  border-top:1px solid rgba(0,255,225,0.25);
  font-size:13px;
}
.speed .row{
  display:flex;justify-content:space-between;align-items:center;
  margin-bottom:10px;
}
.speed .val{color:#00ff9c;font-weight:bold;font-size:14px;}
input[type=range]{
  -webkit-appearance:none;appearance:none;
  width:100%;height:6px;margin:0;
  border-radius:3px;outline:none;
  background:linear-gradient(90deg,#00897b,#00ffe1);
}
input[type=range]::-webkit-slider-thumb{
  -webkit-appearance:none;appearance:none;
  width:22px;height:22px;border-radius:50%;
  background:#00ffe1;border:2px solid #003b36;
  box-shadow:0 0 8px rgba(0,255,225,0.8);
  cursor:pointer;
}
input[type=range]::-moz-range-thumb{
  width:22px;height:22px;border-radius:50%;
  background:#00ffe1;border:2px solid #003b36;
  cursor:pointer;
}
.hint{margin-top:8px;font-size:10px;line-height:1.5;opacity:0.55;text-align:center;}
/* 双语标签：中文 / English，英文略小且不拆词，按钮窄时整体换到第二行 */
.en{font-size:0.85em;opacity:0.85;white-space:nowrap;}

</style>
</head>
<body>

<div class="panel">
<h2>桌面机器人 / <span class="en">Desktop Robot</span></h2>

<div class="grid">
  <div class="empty"></div>
  <button onclick="fetch('/f')">前进 / <span class="en">Forward</span></button>
  <div class="empty"></div>

  <button onclick="fetch('/l')">左转 / <span class="en">Turn Left</span></button>
  <button class="stop" onclick="fetch('/s')">停止 / <span class="en">Stop</span></button>
  <button onclick="fetch('/r')">右转 / <span class="en">Turn Right</span></button>

  <div class="empty"></div>
  <button onclick="fetch('/b')">后退 / <span class="en">Backward</span></button>
  <div class="empty"></div>
</div>

<div class="mode">
  <button id="btn_sleep" class="active" onclick="setMode('off')">休眠 / <span class="en">Sleep</span></button>
  <button id="btn_wiggle" onclick="setMode('soft')">轻动 / <span class="en">Gentle</span></button>
  <button id="btn_curious" onclick="setMode('normal')">好动 / <span class="en">Active</span></button>
</div>
<div class="hint">好动 = 好奇模式 · 休眠 = 关闭好奇模式 / Active = Curious · Sleep = Off</div>

<div class="act">
  <div class="cap"><span>动作 / <span class="en">Actions</span></span><span>与语音同一套指令 / <span class="en">Same as Voice</span></span></div>
  <div class="row">
    <button onclick="act(5)">跳舞 / <span class="en">Dance</span></button>
    <button onclick="act(7)">扭一扭 / <span class="en">Wiggle</span></button>
    <button class="hot" onclick="act(12)">跳个舞 / <span class="en">Party</span></button>
    <button onclick="act(8)">退下 / <span class="en">Rest</span></button>
    <button class="sleep" onclick="act(9)">完全停止 / <span class="en">Full Stop</span></button>
  </div>
</div>

<div class="speed">
  <div class="row">
    <span>电机转速 / <span class="en">Motor Speed</span></span>
    <span class="val" id="speed_val">__SPEED__%</span>
  </div>
  <input type="range" id="speed" min="0" max="100" step="5" value="__SPEED__"
         oninput="setSpeed(this.value)" onchange="setSpeedNow(this.value)">
  <div class="hint">0% 停止 · 100% 全速 / 0% Stop · 100% Full Speed</div>
</div>

</div>

<script>
function clearActive(){
  document.getElementById('btn_sleep').classList.remove('active');
  document.getElementById('btn_wiggle').classList.remove('active');
  document.getElementById('btn_curious').classList.remove('active');
}

function setMode(mode){
  fetch('/mode_' + mode);
  clearActive();

  if(mode === 'off') document.getElementById('btn_sleep').classList.add('active');
  if(mode === 'soft') document.getElementById('btn_wiggle').classList.add('active');
  if(mode === 'normal') document.getElementById('btn_curious').classList.add('active');
}

/* 网页动作按钮直接发 VoiceCmd 编号，服务端用同一个 voiceExecute() 分发，
   所以网页和语音的动作、眼睛表情、随机模式行为完全一致。 */
function act(c){ fetch('/act?c=' + c); }

/* 语音改了好奇/随机模式后，网页按钮高亮也要跟着变。
   1.5 秒拉一次设备真实状态；只同步模式，不覆盖正在拖动的转速滑块。 */
function syncMode(){
  fetch('/state').then(function(r){ return r.json(); }).then(function(o){
    clearActive();
    var ids = ['btn_sleep','btn_wiggle','btn_curious'];
    var el = document.getElementById(ids[o.mode]);
    if (el) el.classList.add('active');
  }).catch(function(){});
}
setInterval(syncMode, 1500);

/* 滑块拖动时 oninput 每动一下都会触发，做 60ms 节流，
   避免 ESP32 单线程 WebServer 被并发请求打爆；松手用 onchange 兜底发最终值。 */
var speedTimer = null;
function setSpeed(v){
  document.getElementById('speed_val').textContent = v + '%';
  clearTimeout(speedTimer);
  speedTimer = setTimeout(function(){ fetch('/speed?val=' + v); }, 60);
}
function setSpeedNow(v){
  clearTimeout(speedTimer);
  document.getElementById('speed_val').textContent = v + '%';
  fetch('/speed?val=' + v);
}
</script>

</body>
</html>
)rawliteral";

  // 把 __SPEED__ 占位符换成设备当前转速，刷新网页后滑块不会跳回默认值
  page.replace("__SPEED__", String((int)g_speed));
  // 必须显式带 charset：否则浏览器按本地默认编码解析，中文会乱码
  server.send(200, "text/html; charset=utf-8", page);
}

/* ================= SERVER ================= */
void setupServer() {
  server.on("/", handleRoot);

  // 手动指令要能打断语音/随机动作序列，否则状态机会把电平又写回去
  server.on("/f", [](){ manualActive=true; stopAction(); motorWifi(1); server.send(200); manualActive=false; });
  server.on("/b", [](){ manualActive=true; stopAction(); motorWifi(2); server.send(200); manualActive=false; });
  server.on("/l", [](){ manualActive=true; stopAction(); motorWifi(3); server.send(200); manualActive=false; });
  server.on("/r", [](){ manualActive=true; stopAction(); motorWifi(4); server.send(200); manualActive=false; });
  server.on("/s", [](){ manualActive=true; stopAction(); motorWifi(0); server.send(200); manualActive=false; });

  server.on("/mode_off",    [](){ randomMode = RANDOM_OFF;    server.send(200); });
  server.on("/mode_soft",   [](){ randomMode = RANDOM_SOFT;   server.send(200); });
  server.on("/mode_normal", [](){ randomMode = RANDOM_NORMAL; server.send(200); });

  // 网页动作按钮：c = VoiceCmd 编号，和语音共用同一个 voiceExecute()，
  // 避免网页/语音两套逻辑走偏（眼睛、随机模式、非阻塞动作序列都一致）。
  server.on("/act", [](){
    if (server.hasArg("c")) {
      int c = server.arg("c").toInt();
      if (c > 0 && c <= VOICE_CMD_MAX) voiceExecute((byte)c);
    }
    server.send(200, "text/plain", String("ok"));
  });

  // 设备真实状态回读：语音改了好奇/随机模式后，网页按钮高亮能跟上
  server.on("/state", [](){
    String s = "{\"mode\":" + String((int)randomMode) + "}";
    server.send(200, "application/json", s);
  });

  // 电机转速：/speed?val=0..100；不带参数时只回读当前值
  server.on("/speed", [](){
    if (server.hasArg("val")) {
      int v = server.arg("val").toInt();
      if (v < 0)   v = 0;
      if (v > 100) v = 100;
      g_speed = (uint8_t)v;
      // 手动方向还在保持时立刻重新套用，拖动滑块才能实时变速
      if (g_manual_dir != 0) motorWifi(g_manual_dir);
    }
    server.send(200, "text/plain", String((int)g_speed));
  });

  // 方向脚直驱诊断：/drv?a=&b=&c=&d= 各 0..255，对应 ch0..ch3 占空。
  // 用于排查接线 / 单脚驱动；会停掉动作状态机与随机动作，
  // 避免刚写进去的电平被 loop() 覆盖。
  server.on("/drv", [](){
    static const char* key[4] = {"a","b","c","d"};
    uint8_t v[4];
    for (int i = 0; i < 4; i++) {
      int x = server.hasArg(key[i]) ? server.arg(key[i]).toInt() : 0;
      if (x < 0)   x = 0;
      if (x > 255) x = 255;
      v[i] = (uint8_t)x;
    }
    stopAction();
    randomMode = RANDOM_OFF;
    digitalWrite(STBY, HIGH);
    g_manual_dir = 0;
    motorSet(v[0], v[1], v[2], v[3]);
    String s = "{\"a\":" + String((int)v[0]) + ",\"b\":" + String((int)v[1]) +
               ",\"c\":" + String((int)v[2]) + ",\"d\":" + String((int)v[3]) + "}";
    server.send(200, "application/json", s);
  });

  server.onNotFound(handleRoot);
  server.begin();
}

/* ================= SETUP ================= */
void setup() {
#if VOICE_DEBUG
  Serial.begin(115200);                // 对不上帧时打开，串口监视器看原始字节
#endif
  pinMode(STBY,OUTPUT); digitalWrite(STBY,LOW);
  voiceSetup();                        // GPIO4 = ASR PRO2.0 TX(3.3V 直连)，GPIO5 = ACK

  // 4 根方向脚交给 LEDC，此后只能用 ledcWrite 控制，不能再 digitalWrite
  for (int i = 0; i < 4; i++) {
    ledcSetup(i, PWM_FREQ, PWM_RES);
    ledcAttachPin(MOTOR_PINS[i], i);
    ledcWrite(i, 0);                 // 上电先全停，防止乱转
  }

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay(); display.display();

  roboEyes.begin(SCREEN_WIDTH,SCREEN_HEIGHT,100);
  roboEyes.setAutoblinker(ON,3,2);
  roboEyes.setIdleMode(ON,2,2);
  roboEyes.setMood(DEFAULT);

  randomSeed(esp_random());

  WiFi.softAP("Desktop-Robot");

  // 限制 AP 发射功率到 8.5dBm。
  // WIFI_POWER_8_5dBm 定义在 WiFiGeneric.h，值 = 34；
  // esp_wifi_set_max_tx_power() 的单位是 0.25dBm，34 / 4 = 8.5dBm。
  // 必须放在 softAP()（内部 esp_wifi_start）之后：提前调用会返回
  // ESP_ERR_WIFI_NOT_STARTED，设置不生效。
  // 等价的 Arduino 写法：WiFi.setTxPower(WIFI_POWER_8_5dBm);
  esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);

  dnsServer.start(53,"*",WiFi.softAPIP());
  setupServer();

  digitalWrite(STBY,HIGH);
}

/* ================= LOOP ================= */
void loop() {
  roboEyes.update();
  server.handleClient();
  dnsServer.processNextRequest();

  voicePoll();            // 串口命令（硬件 FIFO，loop 偶尔忙也不会立刻丢字节）
  pulseUpdate();          // 随机动作状态机
  seqUpdate();            // 语音动作状态机
  voiceAckUpdate();       // ACK 脉冲拉低

  static unsigned long lastTick = 0;
  if (!manualActive && !actionBusy() && millis() - lastTick > 40) {
    lastTick = millis();

    if (randomMode == RANDOM_SOFT) {
      if (random(120) == 1) {
        MOTOR(random(9), random(6,18), random(40,90), 1);
      }
    }
    else if (randomMode == RANDOM_NORMAL) {
      if (random(100) == 1) {
        MOTOR(random(9), random(5,50), random(10,100), random(20));
      }
    }
  }
}
