# Hello, Joel here. With this code you can control one crane with your computer's keyboard through serial (USB wire). Use arrow keys, A, Z, F and S keys.

# libraries you may need to install with pip
import pygame # https://www.pygame.org/docs/ref/joystick.html I took this code from here.
import serial # https://playground.arduino.cc/interfacing/python
import serial.tools.list_ports

# comes with Python 3.7.2
import struct # https://docs.python.org/2/library/struct.html
import threading
import time

def monitor(fan): # prints whatever arduino sends us
	while True:
		try:
			print(fan.readline().rstrip().decode())
		except:
			break

# Define some colors
BLACK=(0,0,0)
WHITE=(255,255,255)

# This is a simple class that will help us print to the screen
# It has nothing to do with the joysticks, just outputting the
# information.
class TextPrint:
	def __init__(self):
		self.reset()
		self.font = pygame.font.SysFont('Consolas', 18, bold=False, italic=False)
	def print(self, screen, textString):
		textBitmap = self.font.render(textString, True, WHITE)
		screen.blit(textBitmap, [self.x, self.y])
		self.y += self.line_height
	def reset(self):
		self.x = 10
		self.y = 10
		self.line_height = 20
	def indent(self):
		self.x += 20
	def unindent(self):
		self.x -= 20

pygame.init()
screen = pygame.display.set_mode([300, 300]) # screen size [width,height]
pygame.display.set_caption("keyboard-USB")
clock = pygame.time.Clock() # Used to manage how fast the screen updates
textPrint = TextPrint() # Get ready to print

ser=None
cat=None
say=False
old=0
wax=0
slewOld=0
trolleyOld=0
hookOld=0
buffer=bytearray(7)
settings=0b10100000

# -------- Main Program Loop -----------
done = False #Loop until the user clicks the close button.
while done==False:

	# DRAWING STEP
	# First, clear the screen to white. Don't put other drawing commands
	# above this, or they will be erased with this command.
	screen.fill(BLACK)
	textPrint.reset()

	slew=0
	trolley=0
	hook=0
	send=0

	# EVENT PROCESSING STEP
	for event in pygame.event.get(): # User did something
		if event.type == pygame.QUIT: # If user clicked close
			done=True # Flag that we are done so we exit this loop
		if event.type == pygame.KEYDOWN:
			if event.key == pygame.K_s:
				settings ^= 0b1000
			if event.key == pygame.K_h:
				settings |= 0b10000 # home
			if event.key == pygame.K_SPACE:
				settings &= ~0b100000 # stop
			if event.key == pygame.K_l:
				settings ^= 0b100 # lights on/off

	# keyboard control
	keys=pygame.key.get_pressed()
	if not keys[pygame.K_SPACE]:
		if keys[pygame.K_LEFT]:
			slew=7000 # motor steps per second
		elif keys[pygame.K_RIGHT]:
			slew=-7000
		if keys[pygame.K_UP]:
			trolley=-4000
		elif keys[pygame.K_DOWN]:
			trolley=4000
		if keys[pygame.K_a]:
			hook=7000
		elif keys[pygame.K_z]:
			hook=-7000
	
	textPrint.print(screen,"{} {} {}".format(slew,trolley,hook))
	struct.pack_into('>B',buffer,0,settings)
	struct.pack_into('>BB',buffer,1,(slew&0x3FFF)>>7,slew&0x7F)
	struct.pack_into('>BB',buffer,3,(trolley&0x3FFF)>>7,trolley&0x7F)
	struct.pack_into('>BB',buffer,5,(hook&0x3FFF)>>7,hook&0x7F)
	
	if ser is None: # auto select arduino COM port
		if cat is None:
			now=time.time()
			if now-old > 1 : # reduces CPU usage
				old=now
				for dog in serial.tools.list_ports.comports():
					print(dog)
					cat=dog.device
				if say is False:
					say=True
					if cat is None:
						print('Plug Arduino USB cable.')
		else:
			try:
				ser = serial.Serial(cat,250000) # port, baud rate
			except:
				pass
			else:
				threading.Thread(target=monitor, args=(ser,)).start()
	
	else:
		if slew!=slewOld or trolley!=trolleyOld or hook!=hookOld or 1:
			try:
				ser.write(buffer) # send speeds to Arduino
			except:
				ser=None
				cat=None
				say=False
			else:
				slewOld=slew
				trolleyOld=trolley
				hookOld=hook
				settings |= 0b100000 # stop stopping
				settings &= ~0b10000 # stop homing
				textPrint.print(screen,"USB on")
		if send:
			try:
				ser.write(bytes(struct.pack('>bb',-127,wax))) # sometimes send also settings
			except:
				ser=None
				cat=None
				say=False
			else:
				send=0
				wax &= ~2 # stop homing
				wax &= ~4 # stop stopping
				textPrint.print(screen,"Settings {}".format(wax))
	
	# ALL CODE TO DRAW SHOULD GO ABOVE THIS COMMENT
	
	# Go ahead and update the screen with what we've drawn.
	pygame.display.flip()
	
	# Limit to 20 frames per second
	clock.tick(20)
		
# Close the window and quit.
# If you forget this line, the program will 'hang'
# on exit if running from IDLE.
ser.close()
pygame.quit ()