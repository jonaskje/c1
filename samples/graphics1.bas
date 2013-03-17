	w = 640
	h = 480	
	graphics(w, h)
	v = -100
	r = 255
	lineWidth(50)
:loop
	color(r + v - 100, r - v - 100, 255, 255)
	line(-100,v,100,-v)
	line(v,100,-v,-100)
	v = v + 1
	if v >= 100 then v = -100
	if display() = 1 then goto loop

