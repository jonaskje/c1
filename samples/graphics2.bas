	w = 2560
	h = 1440
	i = 0
	graphics(w, h)
	ll = 0
	ld = 1
:loop
	
	rw0 = random((0-w/2),w/2)
	rh0 = random((0-h/2),h/2)
	color(random(0, 255), random(0, 255), random(0, 255), 255)
	
	line(rw0, rh0, rw0 + ll/5 + 2, rh0)

	i = i + 1
	if i < 10000 then goto loop

	i = 0

	ll = ll + ld
	if ll >= (20 * 5) then ld = -1
	if ll <= 0 then ld = 1	

	if display() = 1 then goto loop

