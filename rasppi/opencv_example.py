#!/usr/bin/python

import cv

print "Starting OpenCV example..."

cap = cv.CaptureFromCAM(0)

if (cap):
    print "Opened feed!"
else:
    print "Failed to open feed."
    

image = cv.QueryFrame(cap)

#cv.NamedWindow('Window', cv.CV_WINDOW_AUTOSIZE)
#image = cv.LoadImage('image0.jpg', cv.CV_LOAD_IMAGE_COLOR)

font = cv.InitFont(cv.CV_FONT_HERSHEY_SIMPLEX, 1,1,0,1,8)
cv.PutText(image,"Testing!", (5,20), font, 255)

#cv.ShowImage('Window', image)
cv.SaveImage('imageOut.jpg',image)

print "Done!"




