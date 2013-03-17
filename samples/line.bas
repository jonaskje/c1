	w = 1280
	h = 960
	graphics(w, h)
	lineWidth(50)
	color(50, 50, 50, 255)
:loop
	line(-100,-100,100,100)
	if display() = 1 then goto loop

