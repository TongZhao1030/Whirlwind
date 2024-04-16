设计概述
=====
本项目于的机器人控制组件由视觉识别模块（K210）和机械控制模块（Arduino核心版）组成。识别模块主要的任务加载预训练的视觉识别网络，从实时图像中提取图片，将图片传入视觉识别网络得到识别结果并将其传入到控制模块；控制模块的主要任务是接受识别模块传输的参数，通过算法将其转化为小车各个部分的控制信号，实现寻球、转向、运动、捡球等功能。具体效果可以参见此[视频](https://www.bilibili.com/video/BV1ns4y1y7rU)。

![image](https://github.com/TongZhao1030/Whirlwind/assets/164134563/5978b214-b1d0-4dc8-856b-31a00c1185c0 "技术模块组成")


整体结构
=====
如下图，该捡球机器人主要由小车，K210视觉识别模块，Arduino控制模块，滚筒毛刷，控制毛刷的电机，红外测距装置及连接件几部分构成。其中，K210视觉识别模块用于识别球类并将信号传输给控制模块，控制小车移动与毛刷转运。滚筒毛刷通过电机的驱动将小球扫进收纳盒，相较于传统的利用机械臂夹取等捡球方式，利用滚筒捡球可以一次性将多个球同时扫入收纳盒中，大幅度提高了捡球效率，同时它只需一个电机就可以控制，控制逻辑也非常简单，这是本发明的一个创新之处。此外，小车采用了麦克纳姆轮，可以灵活的变换方向，即使在狭小的区域也能实现转向，也进一步提升了小球的适用性。

![image](https://github.com/TongZhao1030/Whirlwind/assets/164134563/ac5051bc-1c97-403f-b8fb-c6c099895be5 "捡球机器人的整体结构")

数据集采集与标注
=====
为了实现球类识别和追踪，需要用标注的球类检测数据训练模型，由于目前开源的数据集无法完成所需网络训练，所以我们在捡球机器人实际应用场景：网球场，乒乓球场等地方采集了数据集并人工进行标注，标注出每张图片中网球或乒乓球所在区域以供网络训练，下图所示为数据集及标注示例。我们的数据集包含样本数量共5442个，包含网球和乒乓球在不同光线，场景，日照下的情况，不同的场景下训练的模型具有更强的泛化性和鲁棒性。

![image](https://github.com/TongZhao1030/Whirlwind/assets/164134563/b70ab328-da32-4058-b024-1e74a3395461 "数据集及标注示例")

视觉识别网络的训练
=====
我们设计的视觉识别神经网络以YOLO2为模型网络，以MobileNet v1为主干网络，下图为MobileNet v1网络模型结构图。在有标注数据集下，我们对该网络进行有监督训练。具体来说，我们以224*224的分辨率将图片输入网络，设置图片预处理平均值为123.5，标准差为58.395。训练批量大小为32，学习率为0.001，训练次数为200。经过训练后模型收获了较高的识别准确率,在测试集上准确率达到90.3%。在实际应用中，我们的模型在10米之内可以以高达95%的准确率识别到球类。识别到球类后视觉识别模块输出球类在图像中的水平位置（-1到1）以及球类在图像中的大小，并将其通过串口通讯传输给Arduino主控核心板从而对机器人进行机械控制。

![image](https://github.com/TongZhao1030/Whirlwind/assets/164134563/89661b17-c84c-4489-951b-b99d51034bce "MobileNet v1 网络结构")

串口通信设计
=====
如何联系识别模块与控制模块是我们产品技术中的一个难点。机器人控制由K210视觉识别模块和Arduino控制模块的通讯与控制组成。我们设置的通信逻辑如下：识别模块会输出两个参数，第一个参数是一个-1到1之间的实数，代表球在视野当中所处的水平相对位置；另一个参数是一个正整数，代表识别框的大小。其中相对位置参数控制小车的转向，而框大小参数则作为阈值，判断什么时候该启动毛刷并冲刺捡球。当视野中有多个球时，我们的算法会选取视野中最大的那个球（即框大小最大的球）并输出它的相关参数，该球即为距离小车最近的球。我们使用MaixPy软件平台提取并向串口输出参数。具体来说，YOLO2神经网络输出球类在图片中的水平位置dirt和预测框大小scale，利用MicroPython包含的machine库中的UART函数可以将dirt和scale信息以串口通讯传输给控制模块，在Arduino控制核心板接受到该信号后即可利用该信息对机器人进行机械控制。

![image](https://github.com/TongZhao1030/Whirlwind/assets/164134563/bf5d8f59-0676-43c8-a606-9cce72d33e2f "识别模块和串口通信技术原理")

控制逻辑设计
=====
捡球小车有两种模式：手动模式和自动模式，可通过手柄上的按键进行选择。手动模式下，用户可以通过控制手柄来控制小车得运动，我们将手柄上的一个按键设置其控制驱动滚筒毛刷的电机的转动，按下时电机开始转动，带动毛刷转动。至于小车得前进与转向，则可以通过手柄上的摇杆来进行控制。该模式下用户可以较为精确地实现捡球，同时也让该机器人多了一些趣味性和可玩性。

自动模式下，小车首先开始寻球，方式是原地自旋，每次大概旋转15°，旋转后K210视觉模块会检测画面中是否出现小球，如果出现，并且超声波模块未识别到障碍物时，则小车将会朝小球的方向前进。在前进过程中，如果识别到障碍物，则会进行转向直至未识别到障碍物为止，如果K210视觉模块传输过来的小球位置信号显示其超出阈值-0.3-0.3，则表明小车偏离了小球的方向，需要进行修正，如果位置信号小于-0.3或大于0.3，则表明小球偏左或偏右，小车需要右转或左转直至位置信号回到-0.3-0.3的区间范围内。由于摄像头具有一定角度，当小车前进到距离小球过近时，小球可能从摄像头中消失，此时我们认为小车距离小球足够近并且方向正对小球，因此我们让小车以原来的速度继续按此方向前进2s，这样小车便可以顺利到达小球的位置。在前进的这个2s过后，我们认为小车已经捡起刚才识别到的小球，此时需进行下一轮捡球，因此小车会重新开始原地自旋，直至K210摄像头中再次出现小球，重复之前的过程。当小车原地旋转一周仍未识别到小球时，我们让小车前进2s，然后再进行原地自旋寻找球体，这样能够避免小车一直原地转圈，让小车能够尽可能的进行大范围的寻球。需要注意的是，整个控制逻辑中优先级最高的是避障，小车在进行任何操作之前都会检测其与障碍物的距离并进行避障操作，以保证小车及用户的安全。整体逻辑如下图所示。

![image](https://github.com/TongZhao1030/Whirlwind/assets/164134563/925d5ef0-6dc8-4b88-b919-c3ca3c3ee553 "控制模块技术原理")

![image](https://github.com/TongZhao1030/Whirlwind/assets/164134563/926eaaad-c376-4ad5-a82f-cdb32d2d3dc1 "捡球机器人的控制逻辑流程图")

项目优点
====
- 成本低廉，结构简单，维护方便。本产品的驱动部分为单片机调控的小车，另有简易的储藏盒；视觉识别功能依赖于K210视觉模块，其将参数传输至主控板，通过程序生成控制信号从而控制小车的运动；最后，设置滚筒排刷以实现收集功能。综上可见，本产品的功能分区简单清晰，机械连接简易稳固，同时便于拆解维护；采用的素材性价比高，综合成本低廉，经济价值较高。
- 应用场景的多样性：本产品应用了转动排刷对目标进行收集拾取，目标球可以是乒乓球、网球、高尔夫球等，只需对控制程序的参数进行一定的微调，就可以使产品灵活应用于不同的场地，从而实现对不同目标的拾取。针对有多种需求的用户，本产品可以做到“一步到位”。
- 操作模式的可调性：本产品支持自动寻球捡球与手动遥控捡球两种操控模式。针对待捡球数量较多且散落区域广、分布较密集的应用场景，系统的自动收集模式能够很好的对场地内的待捡球进行寻找与收集，极大地减轻人力工作量；而针对遗落位置较为偏僻、收集待捡球球需要的角度较为刁钻的情况，用户可以切换至手动控制模式，遥控小车前往指定目标处对目标进行收集。
- 视觉识别的高准确性：本系统最核心的功能与最突出的优点是基于K210视觉模块，在识别出目标球后，生成两个参数：一个参数是一个-1到1之间的数，代表球在视野当中所处的相对位置；另一个参数是一个正整数，代表识别框的大小。其中相对位置参数控制小车的转向，而框大小参数则作为阈值，判断什么时候该启动毛刷并冲刺捡球。我们采用Mobile-Net卷积神经网络对模块的识别进行训练，最终模型的在没试数据集上识别准确率达90%以上。在实际应用测试下，我们的视觉模块目前具备10米的识别范围和95%的识别准确率，在同类产品中属于较高水平。
- 收集结构的耐耗性：对于一件应用性较高的产品，其在使用期间的磨损老化速度是研发人员与用户都十分关心的一个特性。我们针对本产品运行时间最长、最易磨损的环节，即收集模块，应用了独特的滚动排刷，刷毛材料的柔韧性很好的解决了收集装置与球体和地面碰撞摩擦易受损的问题；同时，本系统采用较为粗硬的刷毛材料，可以很好保证收集较重目标时的收集效果。




