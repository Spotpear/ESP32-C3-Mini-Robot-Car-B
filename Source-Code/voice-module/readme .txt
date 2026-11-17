1】Software download: https://drive.google.com/file/d/19ujY1f2GoXHnFev5xYsjjI63T2qsZcQG/view?usp=sharing
2】Flasher purchase: ASR-LINK downloader【CH340K】
Purchase link：https://spotpear.com/shop/ESP32-mini-C3-AI-Toy-AI-Core-DeepSeek-XiaoZhi-DouBao-1.54-1.28-0.71-inch-LCD/ASR-LINK-ASRPRO-Core.html
3】Wake word modification
void setup() {
//{ID:0,keyword:"唤醒词",ASR:"hey robot",ASRTO:"I am here"}
//{ID:1,keyword:"唤醒词",ASR:"hello robot",ASRTO:"I am here"}
//{ID:2,keyword:"唤醒词",ASR:"hey buddy",ASRTO:"I am here"}
//{ID:3,keyword:"唤醒词",ASR:"hi desktop",ASRTO:"I am here"}
.....
Wake word modification: ASR"XXXX" Wake word feedback: ASRTO"XXXX"
4】Action command and voice feedback modification
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
Action command word modification: ASR"XXXX", voice feedback modification: ASRTO"XXXX"
5】ASR_CODE()
Voice recognition command processing function: recognizes different voice commands and sends them to ESP32C3 via serial port.