	w = 1280
	h = 960
	graphics(w, h)
	
	r = 0
:loop

	color(0, 255, 0, 255)
	penReset()
	lineWidth(1)

	penDown()
	penRotate(r)

	penForward(30)
	penRotate(45)
	penForward(30)
	
	

	color(255, 255, 0, 255)
	penReset()

	penRotate(r)
	s = 100
	penForward(s/2)
	penDown()
	penRotate(90)
	penForward(s/2)
	penRotate(90)
	penForward(s)
	penRotate(90)
	penForward(s)
	penRotate(90)
	penForward(s)
	penRotate(90)
	penForward(s/2)
	
	
	color(0, 255, 255, 255)
	penReset()
	penDown()

	l = 3
	n = 0
	penRotate(-r)
	width = 5 
:loopSpiral
	lineWidth(width/5)
	penForward(l)
	penRotate(20)

	l = l + 1
	n = n + 1
	width = width + 1
	if n < 100 then goto loopSpiral
	
	r = r + 1
	if r >= 360 then r = 0
	
	if display() = 1 then goto loop

