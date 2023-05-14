# Indoor Positioning System based on UWB

## Chapter 1. Basic TWR

### Communication

The basic algorithm follows the Double-Sided Two way ranging solution. The system consists of two basic nodes namely, initiator and responder. Typical communication message frames are listed below.

```c
static uint8 rx_poll_msg[] 	= {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21, 0, 0};
static uint8 tx_resp_msg[] 	= {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0, 0, 0};
static uint8 rx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8 tx_poll_msg[] 	= {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21, 0, 0};
static uint8 rx_resp_msg[] 	= {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0, 0, 0};
static uint8 tx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
```

The messages here comply with the IEEE 802.15.4 standard MAC data frame encoding and they are following the ISO/*IEC:*24730-62:2013 standard. 

### Sequence

To compare with the Single-Sided Two way ranging, the communication sequence are:

- a poll message sent by the initiator to trigger the ranging exchange.

- a response message sent by the responder to complete the exchange and provide all information needed by the initiator to compute the time-of-flight (distance) estimate.
- **SUMMARY:** Initiator send - responder receive - responder respond - initiator receive. 

The messages used in the Double-Sided TWR are:

- a poll message sent by the initiator to trigger the ranging exchange.
- a response message sent by the responder allowing the initiator to go on with the process
- a final message sent by the initiator to complete the exchange and provide all information needed by the responder to compute the time-of-flight (distance) estimate.
- **SUMMARY:** Initiator send - responder receive - responder respond - initiator receive - initiator send all information needed - responder receive. 

### Message frame

| Frame Control | Sequence No. |  PAN ID   | Destination addr. | Source addr. | Function Mode | Content |
| :-----------: | :----------: | :-------: | :---------------: | :----------: | :-----------: | :-----: |
|    byte0/1    |    byte2     |  byte3/4  |      byte5/6      |   byte7/8    |     byte9     | byte10+ |
|   0x41,0x81   |      0+      | 0xCA,0xDE |                   |  Hard Coded  |               |         |

**The first 10 bytes of those frame are common and are composed of the following fields:**

- byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
- byte 2: sequence number, incremented for each new frame.
- byte 3/4: PAN ID (0xDECA).
- byte 5/6: destination address. ('W','A')
- byte 7/8: source address.('V','E')
- byte 9: function code (specific values to indicate which message it is in the ranging process).

*Notes:* 

1. Source and destination addresses are hard coded constants, for a real product every device should have a unique ID. 

2. 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.

**The remaining bytes are specific to each message as follows:**

*Poll message:*

- no more data

*Response message:*

- byte 10: activity code (0x02 to tell the initiator to go on with the ranging exchange).

- byte 11/12: activity parameter, not used for activity code 0x02.

*Final message:*

- byte 10 -> 13: poll message transmission timestamp.

- byte 14 -> 17: response message reception timestamp.

- byte 18 -> 21: final message transmission timestamp.

 All messages end with a 2-byte checksum automatically set by DW1000. **When declaring the char array, always remember these 2 byte.**

### Delay

Every node follows the sequence above with a specific delay between two process. Delays between frames have been chosen here to ensure proper synchronization of transmission and reception of the frames between the initiator and the responder. And to ensure a correct accuracy of the computed distance. The user is referred to DecaRanging ARM Source Code Guide for more details about the timings involved in the ranging process.

*Single-Sided:* As we want to send final TX timestamp in the final message, we have to compute it in advance instead of relying on the reading of DW1000 register. Timestamps and delayed transmission time are both expressed in device time units so we just have to add the desired response delay to response RX timestamp to get final transmission time. The delayed transmission time resolution is 512 device time units which means that the lower 9 bits of the obtained value must be zeroed. This also allows to encode the 40-bit value in a 32-bit words by shifting the all-zero lower 8 bits.

*In short, we want a delayed transmission, hence the interval is known. The tof interval can be calculated as total interval minus delayed time with error time.*

### Implementation

Naming rules : 

+ Relates with device tx : tx_  
+ Relates with device rx : rx_
+ Initiator : _poll_msg
+ Responder : _resp_msg
+ TimeStamp related : _ts
+ Delay Time related : _time	

#### Single-Sided

**Initiator:	tx_poll_msg**

Poll msg with least 9 bytes. Start rx automatically after tx.

**Responder:	tx_resp_msg**

(Comparing msg with **rx_poll_msg** first. See Double-Sided below.)Define respond msg index(RESP_MSG_POLL_RX_TS_IDX) as 10, then the respond msg starts as the index of 10. The following byte can be msg receive timestamp. However we may consider the error between send command set and  antenna really send the msg out of the device. In that case, we may add another timestamp indicates the time antenna really sent. 

+ Byte 10->13 : Receive timestamp ***poll_rx_ts***
+ Byte 14->17 : Response timestamp ***resp_tx_ts***

The tx_resp_msg can be overwritten by those bytes above using ***Function*** *resp_msg_set_ts()*. Delay is necessary, so starttx mode is delayed tx.

**Initiator:**

(Comparing msg with **rx_resp_msg** first). Get the information using ***Function*** *resp_msg_get_ts()*, then calculate the result : see Calculation chapter below.

#### Double-Sided

**Initiator:	tx_poll_msg** 

Poll msg with least 9 bytes. Start rx automatically after tx.

**Responder:	tx_resp_msg**

Receive first poll. Compare with the rx_poll_msg to check the poll is sent by the initiator using:

```c
static uint8 rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21, 0, 0};
memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0
```

To make sure the msg is correct to receive and handle, declare a template char that includes destination, source addr. and function mode is necessary. Since the destination addr. is hard coded, so every device has their own destination addr. which is, in this example 'W''A'. This function will be used in the Anchor-Tag positioning.

The first respond will have no other information.

**Initiator:	tx_final_msg**

Receive first respond. Check if the msg is correct.(Compare with **rx_resp_msg**) 

The second poll is alike the respond in Single-Sided, but this time, the Byte 9 will be the first poll timestamp(tx_timestamp) as poll_tx_ts.

+ Byte 10->13 : First poll timestamp ***poll_tx_ts***
+ Byte 14->17 : Receive first respond timestamp ***resp_rx_ts***
+ Byte 18->22 : Second poll timestamp + error(antenna) ***final_tx_ts***

**Responder:**

(Comparing msg with **rx_final_msg** first). Get the information using ***Function*** final_msg_get_ts()*, then calculate the result : see Calculation chapter below.

## Calculation

#### Single-Sided - Initiator 

We have a poll receive timestamp and a respond msg send timestamp from responder. We also have a poll timestamp and a respond receive timestamp from initiator. 

**RTD:** Real Time Differential

**clkOffsetRatio:** Using clock offset ratio to correct for differing local and remote clock rates

**dwtTimeUnits:** Device basic time unit.
$$
\begin{aligned}
rtdInitiator = respondReceiveTime - pollSendTime 									
\\rtdResponder = respondSendTime - pollReceiveTime
\\tof = ((rtdInitiator - rtdResponder *(1 - clkOffsetRatio)) / 2)\cdot dwtTimeUnits
\\Distance = tof \cdot speedOfLight 
\end{aligned}
$$

#### Double-Sided - Responder

We have a *first poll send timestamp*, a *first respond receive timestamp*  and a *second poll send timestamp* from initiator's final msg. We also have a *first respond send timestamp* and a *second poll receive timestamp* from responder itself. 

***Asymmetric DS TWR***

```c
Ra = resp_rx_ts - poll_tx_ts;
Rb = final_rx_ts_32 - resp_tx_ts_32;
Da = final_tx_ts - resp_rx_ts;
Db = resp_tx_ts_32 - poll_rx_ts_32;
tof_dtu = (Ra * Rb - Da * Db) / (Ra + Rb + Da + Db);

tof = tof_dtu * DWT_TIME_UNITS;
distance = tof * SPEED_OF_LIGHT;
```

At these error levels the precision of determining the arrival time of the messages at each of the receivers is a more significant contributor to overall T_prop error than The clock-induced error : Even with a relatively large UWB operating range of say 100 m, the TOF is just 333 ns, so the error is 20 × 10-6× 333 × 10-9 seconds, which is 6.7 × 10-12 seconds or 6.7 picoseconds which is approximately 2.2 mm. 

**At these error levels the precision of determining the arrival time of the message is actually the more significant source of error.** 

***Symmetric DS TWR***

# Chapter 2. Anchor-Tag System

### 同步异步双边测距的比较

#### 方式一 ： 被动式定位 

ADS ：即基站向各个标签发消息，标签做Responder。那么基站广播发送第一次Poll，此时总数据包数为1，接下来每个标签接收到Poll后进行一定延时来逐个向基站发送Resp，此时总数据包数为1+N，最后基站接收到所有的Resp后，广播第二次Poll。各个标签根据二次Poll中的信息确定自己的位置。因此数据包数量为N+2。

SDS ：基站和标签之间的延时必须准确，且时钟校准，因此基站不能广播，只能一对一，数据包量为3N。

#### 方式二：主动式定位

ADS：即标签主动向各基站发消息，标签做Initiator。标签广播第一次Poll，此时总数据包数为1，接下来每个基站返回给标签Resp，此时总数据包数为1+M，最后广播第二次Poll结束。因此数据包数量为M+2。

SDS：仍然一对一，数据包量不变。

#### 方式三：P2P定位

不同标签之间自行确立。

综上，不想找事儿的话就用ADS吧。

### 两种定位方式的比较

主动式定位以标签为Initiator，向基站广播时要求基站必须处于空闲状态，因为标签位置在移动，需要所有基站同时返回数据进行定位，因此所有基站必须同处于空闲状态，因此要加一个值来告诉标签是否空闲。如果不是所以基站都处于空闲状态就开始测量，如果排队等特别长时间就有较大偏差。最后得到的当前位置信息倒是在基站手里，所以汇总起来不需要额外的通信。

被动式定位，标签始终处于被动接收状态，当然一次也只能单个基站向标签通信，但是基站之间的时间是可控的，可以同步的，标签随时有可能重启，导致碰到多个标签同时向基站发送消息的拥挤情况。基站之间时间同步后，可依次向所有标签广播。这样每个标签都会得到一组相对每个基站的距离信息，但是这个信息还在标签手里，此时可就近向基站发送这组位置信息，进一步汇总到主基站进行集中处理。发送这组信息的时候会造成不可避免的拥挤，当然这一组当前位置信息已经确定，所以即便有点延迟也可以。

如果标签设备不是特别特别多，对于标签定位频率没有那么高，那么两种方式都可以接受。

### 基于ADS的主动式定位

#### 标签

1. 广播Poll建立测距通信
2. 紧急避险功能的实现：手动广播Poll，并在第一次Poll中加一紧急信号

发两次，接受一次

#### 子基站

1. 接收Poll广播并建立测距通信
2. 判断是否紧急信号
3. 计算位置并将标签的对应位置信息广播

#### 主基站

1. 履行子基站各项任务
2. 明确各子基站位置
3. 判断广播是否为子基站位置信息广播，汇总位置信息并计算最终位置

### 基于ADS的被动式定位

#### 子基站

1. 广播Poll建立测距通信
2. 待无设备进行相应后，延时并建立下次测距通信
3. 判断是否紧急信号
4. 判断是否位置上报信号

#### 标签

1. 接收Poll广播并建立测距通信
2. 判断基站是否回传resp，如果没有则固定延时后重试直至接收resp
3. 紧急避险功能的实现：手动广播Poll，并在第一次Poll中加一紧急信号

发两次，接受两次

#### 主基站

1. 履行子基站各项任务
2. 明确各子基站位置
3. 判断广播是否为子基站位置信息广播，汇总位置信息并计算最终位置

# Chapter 3. 基于ADS的被动式定位实现

### 建立通信

根据第一章的通信框架，我们分别设定标签和基站的信息框架，其中Ax代表基站及其序号，Tx代表标签及其序号，大写表示Hardcoded序列号，小写表示根据信息已知对象序列号。

|        | Frame Control | Sequence No. |  PAN ID   | Function Mode | Destination addr. | Source addr. |   Content    |
| ------ | :-----------: | :----------: | :-------: | :-----------: | :---------------: | :----------: | :----------: |
|        |    byte0/1    |    byte2     |  byte3/4  |     byte5     |      byte6/7      |   byte8/9    |   byte10+    |
| POLL   |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x11      |       NONE        | A, ANCHOR_SN |              |
| RESP   |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x21      |   A, anchor_sn    |  T, TAG_SN   | ACTIVITYCODE |
| FINAL  |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x31      |     T, tag_sn     | A, ANCHOR_SN | INFORMATIONS |
| DISTAN |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x41      |   A, anchor_sn    |  T, TAG_SN   |   DISTANCE   |
| EMERG  |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x51      |       NONE        |  T, TAG_SN   |              |

校验前5位基本任务码，后两位为终点校验，再后两位确认起点，最后为内容。

#### 交互步骤 - 方法一

1. 标签时刻准备接收信息
2. 基站1广播Poll，终点为ALL_SN
3. 标签全部接收到消息，记录广播基站SN码，一并向基站1发送RESP，终点为ANCHOR1_SN
4. 基站1只能逐条处理，接收到一条消息后，记录标签SN码，向标签发送FINAL，终点为TAGn_SN
5. 所有标签收到基站1的消息，确定终点，若果不是，等待基站1下一次广播
6. 正确终点的标签计算位置后，回传DISTANCE_MSG，终点为ANCHOR1_SN，并在一段时间内不再响应该基站广播
7. 所有基站不停广播……循环上述操作，直至所有标签冷却期完毕，巧妙的是如果区域内标签数量较为稳定，则再次广播不会出现信息拥挤

#### 交互步骤 - 方法二

1. 标签时刻准备接收信息，基站配置为自动ACK
2. 基站1广播Poll，信息发送后立即进行systick计时以保证FINAL发送间隔固定而不受接收数量的影响
3. 标签RESP直至基站ACK
4. 计时时间到后，基站广播带有TAG_SN的信息串
5. 标签接收信息串提取属于自己的信息后计算距离
6. 向基站发送DISTANCE直至ACK

# Chapter 4. 基于ADS的主动式定位实现

### 建立通信

根据第一章的通信框架，我们分别设定标签和基站的信息框架，其中Ax代表基站及其序号，Tx代表标签及其序号，大写表示Hardcoded序列号，小写表示根据信息已知对象序列号。

|        | Frame Control | Sequence No. |  PAN ID   | Function Mode | Destination addr. | Source addr. |        Content        |
| ------ | :-----------: | :----------: | :-------: | :-----------: | :---------------: | :----------: | :-------------------: |
|        |    byte0/1    |    byte2     |  byte3/4  |     byte5     |      byte6/7      |   byte8/9    |        byte10+        |
| POLL   |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x11      |     A, ALL_SN     |  T, TAG_SN   |                       |
| RESP   |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x21      |     T, tag_sn     | A, ANCHOR_SN |     ACTIVITYCODE      |
| FINAL  |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x31      |   A, anchor_sn    |  T, TAG_SN   |     INFORMATIONS      |
| DISTAN |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x41      |  A, ANCHOR_MAIN   | A, ANCHOR_SN | DISTANCE of every tag |
| EMERG  |   0x41,0x81   |      0+      | 0xCA,0xDE |     0x51      |     A, ALL_SN     |  T, TAG_SN   |                       |

校验前5位基本任务码，后两位为终点校验，再后两位确认起点，最后为内容。

#### 交互步骤 - 方法一

1. 标签广播Poll
2. 基站1收到Poll，回复RESP，其余基站依次延时后回复RESP
3. 标签全部接收到消息，记录广播基站SN码和接收时刻，广播带有所有基站的FINAL
4. 所有基站接收广播后提取自己的信息进行位置计算
5. 子基站向主基站发送自己测量到的所有标签距离

