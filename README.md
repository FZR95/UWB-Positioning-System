# 基于STM32的物联网UWB定位系统

系统硬件侧基于STM32，由F1做Tag，F4做Anchor构成基于ADS-TWR双边测距的Anchor-Tag定位系统，系统还由F4Anchor基站上传至IOT云平台，在H5中进行实时Tag位置更新。

## APP

​	虽然文件名叫APP，但却是物联网系统的H5前端（uniApp）和后端（uniCloud）项目。（）

## DWM1000

​	官方使用手册、数据表等，重点已经标注。
​	官方STM32例程 dw1000_api_rev2p14.

## PCB

​	硬件版图，注意有些版图丝印标注可能是错的。
​	UWB芯片转接板 BU01
​	STM32最小系统+UWB+基本外设（目前基站所用的板子） NODE

## STM32

​	UWB芯片STM32驱动，基于dw1000官方驱动改进，增加物联网（ESP8266）、TFT-LCD（F4）等外设。
​	主基站 MainAnchor-F4 
​	子基站和标签 SubAnchor&Tag-F1

## Indoor Positioning System based on UWB.md

​	介绍了UWB系统的工作原理，收发信息流等。