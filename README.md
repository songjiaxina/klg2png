# klg2png
This is a small tool to change klg-formatted file for running ElasticFusion to OpenCV Mat image. And you can use this to visualize the image and save as *png* format

##  What do I need to build it?
To build this demo .You need the follwings:  
  #### zlib  
  #### libjpeg   
  #### OpenCV 
  You can install the *zlib* and *libjpeg* by this:  
```
sudo apt-get install zlib1g-dev libjpeg-dev
```
##  Build and Run
```cd```to the project folder, and
```
 cmake .
 make
 ./klg2png  your-klg-file
```
 ##  Result
 ![](https://raw.githubusercontent.com/songjiaxina/klg2png/master/1.png)  
