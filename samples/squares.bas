	screenw = 1280
	screenh = 960
	graphics(screenw, screenh)

	squarew = 20
	distance = 32 
	nx = screenw/distance
	ny = screenh/distance

	r = 0
:loop
	x = -(nx/2)
:loopx
	y = -(ny/2)
:loopy
	offset = sin(r * 1000) * 360
	phase = (120000 * (y + (ny/2)))/ny
	cr = (sin((360000*(x + (nx/2)))/nx + offset) * 127 + 127000)/1000
	cg = (sin((360000*(x + (nx/2)))/nx + phase + offset) * 127 + 127000)/1000
	cb = (sin((360000*(x + (nx/2)))/nx + 2*phase + offset) * 127 + 127000)/1000
	color(cr, cg, cb, 255)

	lw = (sin((360000*(x * nx + y))/(nx*ny) + offset) * 8)/1000 + 20
	lineWidth(lw)

	penReset()
	penForward(distance/2)
	penForward(distance*y)
	penRotate(90)
	penForward(distance*x)
	penForward(distance/2)
	penDown()
	penRotate(r)
	penForward(1)
	y = y + 1
	if y < ny/2 then goto loopy

	x = x + 1
	if x < nx/2 then goto loopx

	r = (r + 1) % 360

	if display() = 1 then goto loop

