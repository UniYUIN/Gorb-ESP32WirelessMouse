<p align="center">
<a href="https://www.bilibili.com/video/BV1JnivBpEYX" target="_blank">演示视频</a>｜
<a href="./README.md">English</a>
</p>

# Gorb概述

Gorb是使用ESP32-S3作为主控的鼠标项目，支持USB和无线(BLE)双模切换，以及4个微动按键热插拔更换。\

## 主要功能

* 基础鼠标功能及两个拓展侧键，且按键支持热插拔。
* 驱动支持设置DPI和移动宏。
* 最高1000Hz回报率。
* 通过用户操作中断服务ISR和事件驱动，无轮询减少了CPU消耗，搭配ESP-PM当空闲时进入省电休眠。
* 通过累计差值上报，做到基本不丢操作。

![Gorb](https://gorb-1301890033.cos.ap-chengdu.myqcloud.com/gorb.jpg)

### 使用体验

直接说结论：不如我自己的GPro，但比公司里的办公鼠标好使。主要原因是树脂外壳手感不佳(有点软，且经打磨工序后表面较粗糙)，复刻可以考虑换种材质。

## 如何使用

### 项目结构

    gorb/
    |- app/
    |  |- driver/       驱动软件
    |  |- mouse/        鼠标程序
    |- design/          设计图 --已删除， 去https://oshwhub.com/no_panic/gorb下载设计项目
    |- shell/           3D外壳
    |- README.zh-CN.md  

### 硬件需求

本质上是一个最小系统板，考虑到易于个人焊接，采用ESP32无线模组。具体设计参考Gorb硬件设计 : [Gorb Design](https://oshwhub.com/no_panic/gorb)。\
设计中包含一块带有额外接收器板子的PCB，如果你想使用2.4G模式，可以尝试从这个板子入手。在之前的尝试中我发现ESP32在频繁射频时，电池供电可能不太稳定，所以本项目不包含WIFI模式，如果你解决了这个问题，希望你能提交PR，非常感谢！

#### 主要硬件

* 鼠标主控： ESP32-S3-WROOM 系列芯片
* 光传感器： 原相PAW3395DM

#### 3D外壳

3D外壳来自[MatNS](https://www.thingiverse.com/MatNS/designs)
在Thingiverse上的发布的：[G Pro Wireless - G305 Shell Swap Mod MatNS edit V2](https://www.thingiverse.com/thing:4727172)
，感谢开源。

### 固件烧录

鼠标基于ESP-IDF架构开发，烧录前请先安装ESP-IDF或基于IDE的插件。\
如果使用VSCode ESP-IDF插件，可以参考以下步骤烧录:

    1. 打开 ESP-IDF Explorer
    2. 点击 "ESP-IDF: Select Flash Method" 选择 UART
    3. 点击 "Set Espressif Device Target" 选择 esp32s3(鼠标)
    4. 根据芯片具体型号前往 "SDK Configuration Editor (menuconfig)" 修改Flash和PSRAM配置
    5. 点击 "Full Clean" 清理之前构建文件
    6. 点击 "Build Project" 构建项目，并等待构建完成
    7. 主板断电 (鼠标如果已经插上电池，需要关闭电源开关)
    8. 按住主板Boot按钮通电一小段时间后松开，通过USB连接电脑(没有插上电池的鼠标直接通过连接USB通电)，如果硬件焊接没问题，此时点击 "Select Port to Use" 应该会出现一个新的端口，选择对应端口
    9. 点击 "Flash Project" 烧录项目，并等待烧录完成
    10.再次断电，并重新通电连接电脑

### 驱动使用

* 驱动通过Python开发，发行版本已包含基础.exe程序。目前驱动仅支持鼠标通过USB连接，驱动无法列出通过BLE连接的设备(
  但不影响正常使用)。
* 运行程序后 输入 "help" 查看详细用法。

![Gorb Driver](https://gorb-1301890033.cos.ap-chengdu.myqcloud.com/driver.png)

## 参与

欢迎通过以下方式一起共建Gorb:

* 通过 [Issue](https://github.com/UniYUIN/Gorb-ESP32WirelessMouse/issues) 报告 bug 或进行咨询。
* 提交 [Pull Request](https://github.com/UniYUIN/Gorb-ESP32WirelessMouse/pulls) 改进 Gorb 的代码。

> 如果对你有帮助, 请考虑给gorb一个⭐
