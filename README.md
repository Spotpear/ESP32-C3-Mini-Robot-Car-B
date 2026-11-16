# ESP32-C3-Mini-Robot-Car-B

ESP32-C3-Mini-Robot-Car-B [产品购买链接](https://spotpear.com/shop/ESP32-C3-0.96-inch-Robot-LCD-Display.html)

## 使用步骤

1. 按照 [硬件接法参考图（基础版）](Hardware/Basic%20Version-Wiring%20Diagram.png) / [（语音版）](Hardware/Voice%20Version-Wiring%20Diagram.png)、[整机装配图](Hardware/Robot%20Frame%20Diagram.png) 和 [视频参考](wiki-video.mp4)，成功焊接好硬件。
2. 使用 [刷机软件](Firmware/flash_download_tool/flash_download_tool/) 刷写 [ESP32C3 的 bin 固件](Firmware/Firmware-Basic/ROBOOT.bin)（[刷写教程参考](ESP32C3-update-firmware-wiki.pdf)）。
3. 查看 [Readme](Readme.txt)，了解手机网络控制或者语音控制。
4. 修改 [主控 ESP32C3 源码](Source-Code/Desktop_Robot/Desktop_Robot.ino)，修改 [语音模块源码](Source-Code/voice-module/main.cpp)。

## 资料索引

| 类别 | 资料 |
| --- | --- |
| 购买 | [产品购买链接](https://spotpear.com/shop/ESP32-C3-0.96-inch-Robot-LCD-Display.html) |
| 硬件接线 | [基础版接线图](Hardware/Basic%20Version-Wiring%20Diagram.png) · [语音版接线图](Hardware/Voice%20Version-Wiring%20Diagram.png) · [语音模块固件升级图](Hardware/Voice%20Version-Voice-Recognition-Module-updated-Firmware.png) · [整机装配图](Hardware/Robot%20Frame%20Diagram.png) |
| 视频 | [演示视频 wiki-video.mp4](wiki-video.mp4) |
| 固件 | [基础版固件 ROBOOT.bin](Firmware/Firmware-Basic/ROBOOT.bin) · [语音模块固件（中文）](Firmware/Firmware-Voice/For-Voice-Recognition-Module/Voice%20firmware-CN.bin) · [语音模块固件（English）](Firmware/Firmware-Voice/For-Voice-Recognition-Module/Voice%20firmware-EN.bin) |
| 工具与驱动 | [Flash 下载工具](Firmware/flash_download_tool/flash_download_tool/) · [CH340 驱动](USB-CH340/CH341SER.EXE) |
| 文档 | [固件更新教程 PDF](ESP32C3-update-firmware-wiki.pdf) · [使用说明 Readme.txt](Readme.txt) |
| 源码 | [主控 Desktop_Robot.ino](Source-Code/Desktop_Robot/Desktop_Robot.ino) · [语音模块 main.cpp](Source-Code/voice-module/main.cpp) · [RoboEyes 依赖库](Source-Code/libraries/RoboEyes-main/) |
