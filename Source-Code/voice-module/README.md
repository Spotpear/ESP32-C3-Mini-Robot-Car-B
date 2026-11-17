# Voice Module (ASR-PRO) Quick Start

This folder contains the voice module source (`voice_module.hd`, `main.cpp`) used in the ESP32-C3 Mini Robot Car B. Follow the steps below to install the software, flash the module, and customize the wake words and voice commands.

## 1. Software Download

Download the ASR-PRO software from Google Drive:

- [Download ASR-PRO software](https://drive.google.com/file/d/19ujY1f2GoXHnFev5xYsjjI63T2qsZcQG/view?usp=sharing)

## 2. Flasher Purchase

- **Flasher:** ASR-LINK downloader (CH340K)
- **Purchase link:** [ASR-LINK / ASRPRO Core](https://spotpear.com/shop/ESP32-mini-C3-AI-Toy-AI-Core-DeepSeek-XiaoZhi-DouBao-1.54-1.28-0.71-inch-LCD/ASR-LINK-ASRPRO-Core.html)

## 3. Wake Word Modification

Open `main.cpp` and edit the wake word entries inside `void setup()`:

```cpp
void setup() {
//{ID:0,keyword:"唤醒词",ASR:"hey robot",ASRTO:"I am here"}
//{ID:1,keyword:"唤醒词",ASR:"hello robot",ASRTO:"I am here"}
//{ID:2,keyword:"唤醒词",ASR:"hey buddy",ASRTO:"I am here"}
//{ID:3,keyword:"唤醒词",ASR:"hi desktop",ASRTO:"I am here"}
// .....
```

- To change the **wake word**, edit the `ASR"XXXX"` field.
- To change the **voice feedback**, edit the `ASRTO"XXXX"` field.

## 4. Action Command and Voice Feedback Modification

Edit the command entries in `main.cpp`:

```cpp
//{ID:4,keyword:"命令词",ASR:"forward",ASRTO:"Moving forward"}
//{ID:5,keyword:"命令词",ASR:"move forward",ASRTO:"Moving forward"}
//{ID:6,keyword:"命令词",ASR:"come here",ASRTO:"Coming"}
//{ID:7,keyword:"命令词",ASR:"backward",ASRTO:"Moving back"}
//{ID:8,keyword:"命令词",ASR:"move back",ASRTO:"Moving back"}
//{ID:9,keyword:"命令词",ASR:"go away",ASRTO:"Going away"}
//{ID:10,keyword:"命令词",ASR:"turn left",ASRTO:"Turning left"}
//{ID:11,keyword:"命令词",ASR:"go left",ASRTO:"Turning left"}
//{ID:12,keyword:"命令词",ASR:"to the left",ASRTO:"Turning left"}
//{ID:13,keyword:"命令词",ASR:"turn right",ASRTO:"Turning right"}
//{ID:14,keyword:"命令词",ASR:"go right",ASRTO:"Turning right"}
//{ID:15,keyword:"命令词",ASR:"to the right",ASRTO:"Turning right"}
//{ID:16,keyword:"命令词",ASR:"stop",ASRTO:"Stopping now"}
//{ID:17,keyword:"命令词",ASR:"do not move",ASRTO:"Stopping now"}
//{ID:18,keyword:"命令词",ASR:"halt",ASRTO:"Stopping now"}
//{ID:19,keyword:"命令词",ASR:"curious mode",ASRTO:"Curious mode on"}
//{ID:20,keyword:"命令词",ASR:"be curious",ASRTO:"Curious mode on"}
//{ID:21,keyword:"命令词",ASR:"explore",ASRTO:"Curious mode on"}
//{ID:22,keyword:"命令词",ASR:"wander around",ASRTO:"Curious mode on"}
//{ID:23,keyword:"命令词",ASR:"close curious mode",ASRTO:"Curious mode off"}
//{ID:24,keyword:"命令词",ASR:"stop exploring",ASRTO:"Curious mode off"}
//{ID:25,keyword:"命令词",ASR:"wiggle",ASRTO:"Wiggling"}
//{ID:26,keyword:"命令词",ASR:"move a bit",ASRTO:"Wiggling"}
//{ID:27,keyword:"命令词",ASR:"dance party",ASRTO:"Watch me dance"}
```

- To change a **command word**, edit the `ASR"XXXX"` field.
- To change the **voice feedback**, edit the `ASRTO"XXXX"` field.

## 5. ASR_CODE()

`ASR_CODE()` is the voice-recognition command processing function. It recognizes different voice commands and sends them to the ESP32C3 via the serial port.
