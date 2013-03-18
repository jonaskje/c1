	w = 1280
	h = 960
	graphics(w, h)
	
	r = 0
:loop

	penReset()
	penDown()

	l = 0
	n = 0
	penRotate(-r)
	width = 5 
	extral = 0
:loopSpiral
	color((l * 345) % 256, (l*88 + 243) % 256, (33 + r + l) % 255, 255)
	lineWidth(width/10)
	penForward(1 + l + extral / 2)
	penRotate(25)

	l = l + 1
	n = n + 1
	extral = extral + 1
	width = width + 1
	if n < 200 then goto loopSpiral
	
	r = r + 1
	if r >= 360 then r = 0
	
	if display() = 1 then goto loop

