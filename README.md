# How to setup and run HSI camera from robonation of Robotx2022

## 1. Clone repo
You can download with ssh or just download zip file.
```bash
(ssh)
$git clone git@github.com:wellyowo/hsi_camera.git
```

## 2.hardware setup
setup the HSI camera connect to the laptop with 48V POE and conncect the ethernet cable to your laptop.
Then change the connection setting of your system.
```bash
$sudo nmtui
Edit a connection 
1.change IPV4 from DHCP to Manual
Address: 192.168.0.100/24
Gateway: 192.168.0.1

2.Edit MTU parameter(under ETHERNET) to 9000 bytes
```

## 3. environment setup
We use docker to do environment setup.
(You need do it every time to run the docker environment.)
Here is the tutorial of install docker on your ubuntu machine.
https://docs.docker.com/engine/install/ubuntu/
After you finish the installation, run instruction below.
```bash
$cd ~/hsi_camera
$source Docker/docker_run.sh
```    
    
## 4.How to run?(In ubuntu18.04 docker)
with jupyter-notebook(after you finish 3.)
```bash
(Docker)$source colab_jupyter.sh
```
-Then go to: http://127.0.0.1:8888/?token=assistive
-if need password or token : assistive
-*warning: using notebook will slow down the scanning speed, run as script will be faster
  
## 5.run notebook to setup
(In jupyter notebook UI)
-(1)go folder hsi_camera
-(2)run notebook 01_gdown, it will download the .json and .pkl file.
-(3)run notebook 03_hsi_camera_scan start to collect HSI data

## 6. unfinished part
1.For 6SV model(for using level6 setting)
    -extract 6SV-1.1.tar on machine
    (In docker)you need compile the code on your machine 
    $cd /home/arghsi/hsi_camera/6SV-1.1/6SV1.1
    $make
    
    How to test it works
    $cd /home/arghsi/hsi_camera/6SV-1.1/6SV1.1
    $./sixsV1.1 < ../Examples/Example_In_1.txt 

2.For using level 6 scanning
  (1)update calibration file
    run notebook 09 (it takes time to update calibration file)
  (2)scan with level 6
    run notebook 03 and change the json_path and pkl_path to file name ..._fix.json ..._fix.pkl
    change processing_lvl to 6


