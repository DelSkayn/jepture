# jepture
A simple python library to quickly capture images on the Jetson nano

- Works with all camera's lib argus supports.
- Implements jpeg capturing by gpu accelerated jpeg encoding.
- Simple API.

Jepture is designed to quickly and efficently capture camera images on the jetson platform.

If you find an issue, please let us know!.

Setup
-----

```
git clone https://github.com/DelSkayn/jepture.git
cd jepture
pip install ./
```

Jepture requires jetpack for compilation and execution. 
Jepture is tested against jetpack 4.6.1 and python 3.6.9 on a jetson nano. 
Older versions of jetpack and different versions of the jetson might work but are not tested.

Usage
-----

### Capture Jpegs to disk
Below is an example where we use jepture to create a stream of jpegs which are streamed to disk and writen to the 'data' directory.
Cameras are identiefied with a unique incrementing id. The id is assigned by libargus and should always remain the same.
```python
from jepture import JpegStream

stream = JpegStream([(0,"camera")],resolution=(1920,1080),fps=10.0,mode=0,image_dir="./data")

for i in range(20):
    frames = stream.next()
    print(frames[0].time_stamp, frames[0].number)
```

### Capture images as numpy arrays 

In this example we capture images to numpy arrays. The stream returns numpy arrays of shape (1080,1920,4) in the 
BGRA format for compatibility with opencv.
The mode parameter can be ommited and the implementation will select a mode based on the requested fps.
```python
import cv2
from jepture import NumpyStream

stream = NumpyStream([(0,"camera")],resolution=(1920,1080),fps=10.0)

for i in range(20):
    frames = stream.next()
    print(frames[0].time_stamp, frames[0].number)
    cv2.imshow("Jepture image",frames[0].array);
```

### Multiple cameras

Jepture supports as many cameras as you want. 
```python
import cv2
from jepture import NumpyStream

stream = NumpyStream([(0,"left"),(1,"right")],resolution=(1920,1080),fps=3.0)

for i in range(20):
    frames = stream.next()
    cv2.imshow("Left image",frames[0].array);
    cv2.imshow("Right image",frames[1].array);
```
