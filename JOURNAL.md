---
title: "QuakeCord"
author: "Know Scratcher"
description: "An earthquake sensor that connects with discord."
created_at: "2025-05-25"
---

# May 25th: Research and Design + Build a Prototype.

I finally decided to start this project. I've been wanting to make something useful and about earthquake since I lived in a place with lots of earthquake.

The function I planned to have are:
- detect earthquake
- record the acceleration data for further research
- send notification to discord

Here's a quick picture explaining this:

![structure](https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250525structure.png)

So the things I would need:
- EPS32 (controlling the sensor)
- An accelerometer (of course, for obtaining data)
- A case (for dust protection)
- A data server (for collecting data)

I'll use an ESP-32 dev module to control the hardware because I'm more familiar with it.

Next, I'll need an acceleration sensor. I ask Gemini for suggestion on the sensors, but it seemed like ADXL345 and MPU6050, which I already have, are the most sensitive sensor I can find.

So I read through the data sheet of the [ADXL345](https://www.analog.com/media/en/technical-documentation/data-sheets/adxl345.pdf) and [MPU6050](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf).

After all, I decide to use MPU6050 because its extensibility. It has an accelerometer and a gyroscope, which might make it more possible for something more complicated than only an accelerometer.


I use a breadboard to hold my sensor, which it flat enough at the bottom to make the sensor stable, and also I glued a little piece of plastic that somehow just fit the height of the sensor to hold the sensor's other end. 

![breadboard](https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250525breadboard.jpg)

Then, I found a library for MPU6050 and spend about one hour trying to understand the example program.

**Total time spent: 4h**

# May 31th: Draw design with FreeCAD.

Basically what I did today is draw the case for the controller and sensor.

I use an existing box as a reference and slightly change the design a little bit to fit.

I got a little pissed when freeCAD not responding every time I was finishing a step.

Because I'm the first time using this software, so I feel a little hard to control everything, but after all the pissing and agony, I finally got something that can be showed.

![model](https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531model.png)

Then I extract the case from the overall design to make it 3D printable. 

![separate](https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531seperate.png)

Quickly I realized how stupid I am. I just forgot to add a hole for the power line.

So I quickly drill a hole on the model, then, even more stupid, I forgot that the board needs to be placed facing up to let the user be able to read the builtin lights. I almost cry out, like I spent 3 hour just for this thing to have a shape, and now there's lots and lots of problem. 

After fixing that, the model looks fine, so I export it and put it into Cura to slice it for 3D print.

A very huge problem popped out. Though I am going to use my school's 3D printer, most of the printers have a time limit of around 2hr, anything above that might got removed by the teacher. So I gotta go find my teacher and try my best to persuade him to let me print this part that takes 6 hr to print (9 hr when all are printed at the same time).

![case print time](https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531case.png)

![cap print time](https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531cap.png)

(時=hour, 分=minute)

Well, at least I worked something out. Just... have to tell my teacher... Hope this works.

I worked a little bit more at night to fix the closing gap. Now it look something like this

![new model](https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531new_model.png)

**Total time spent: 8h**

# June 2nd: Try printing the model and fixing it.

Today, I ask my teacher to print the case out for me. Well, you know, teacher... he just bring out his super fast 3d printer and get everything done under 2 hrs.

The led holes are too small, so I change it to be bigger in the CAD and also drill the printed model to make things look nice. In fact the alignment is incredible.

<img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250602led.jpg" height="400" alt="led fits well">

The board holder worked as expected, but a little bit tight, so I fixed it again.

<img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250602fit.jpg" height="400" alt="led fits well">

After I got the case, I soon found that there's two main problem:

First is that when I touch the four "feet" of the cap, it instantly break apart, which means it's not a good solution to use the structure. But it surprised me another time by being able to fit and close just using the little "trench" I dug around the edge.

Second is that the hole for power doesn't fit the power plug at all,

<img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250602plug.jpg" height="400" alt="the hole doesn't fit">


so I just fix that simply going back suffering in the CAD.

Also, I added a little bump to hold the ESP32 just to make sure it doesn't slip like this

<img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250602slip.jpg" height="400" alt="board slip when unplugging">

<img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250602bump.png" height="400" alt="bump">

also I worked on the [MagFire](https://github.com/KnowScratcher/MagFire) project today, and you should definitely check it out.

**Total time spent: 2h**