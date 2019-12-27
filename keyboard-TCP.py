# Hello, Joel here. With this code you can control one crane with your computer's keyboard through ethernet. Use arrow keys, and A, Z, L, H, S, U keys.

# libraries you may need to install with pip
import pygame # pip install pygame

# comes with Python 3.7.2
import struct
import threading
import socket

# Define some colors
BLACK=(0,0,0)
WHITE=(255,255,255)

# This is a simple class that will help us print to the screen
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
pygame.display.set_caption("key-TCP")
clock = pygame.time.Clock() # Used to manage how fast the screen updates
textPrint = TextPrint() # Get ready to print

slewOld=0
trolleyOld=0
hookOld=0
buffer=bytearray(7)
accelBuffer=bytearray(7)
settings=0b10100000

IP="0.0.0.0"
slewAccel=4
trolAccel=2
hookAccel=2
slewSpeed=20
trolSpeed=10
hookSpeed=10

def readSettings():
	global IP
	global slewAccel
	global trolAccel
	global hookAccel
	global slewSpeed
	global trolSpeed
	global hookSpeed
	try:
		open("settings.txt")
	except:
		try:
			f=open("settings.txt","w")
		except:
			print('Could not create settings.txt file.')
		else:
			f.write('IP_address=192.168.10.21\n\n')
			f.write('#Acceleration in units of 10 steps/(s^2). Range 0 to 16000.\n')
			f.write('accel_slew=200\n')
			f.write('accel_trol=100\n')
			f.write('accel_hook=100\n\n')
			f.write('#Speed in units of steps/s. Range 0 to 8000.\n')
			f.write('speed_slew=2000\n')
			f.write('speed_trol=500\n')
			f.write('speed_hook=500')
			f.close()
			print('settings.txt file created.')
	try:
		f=open("settings.txt")
	except:
		print('Can not open settings.txt file.')
	else:
		for x in f:
			x=x.strip()
			if x[:11]=="IP_address=":
				IP=x[11:]
			if x[:11]=="accel_slew=":
				slewAccel=int(x[11:])
			if x[:11]=="accel_trol=":
				trolAccel=int(x[11:])
			if x[:11]=="accel_hook=":
				hookAccel=int(x[11:])
			if x[:11]=="speed_slew=":
				slewSpeed=int(x[11:])
			if x[:11]=="speed_trol=":
				trolSpeed=int(x[11:])
			if x[:11]=="speed_hook=":
				hookSpeed=int(x[11:])
		f.close()
		print('Crane IP:  {}'.format(IP))
		print('Speeds:  {}  {}  {}'.format(slewSpeed,trolSpeed,hookSpeed))
	struct.pack_into('>BB',accelBuffer,1,(slewAccel&0x3FFF)>>7,slewAccel&0x7F)
	struct.pack_into('>BB',accelBuffer,3,(trolAccel&0x3FFF)>>7,trolAccel&0x7F)
	struct.pack_into('>BB',accelBuffer,5,(hookAccel&0x3FFF)>>7,hookAccel&0x7F)
readSettings()

# Send through ethernet
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(3)
print("Connecting to {}...".format(IP))
try:
	sock.connect((IP,10000))
except:
	print("Can't connect.")
else:
	print("Connected.")

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
			if event.key == pygame.K_u:
				send=1

	# keyboard control
	keys=pygame.key.get_pressed()
	if not keys[pygame.K_SPACE]:
		if keys[pygame.K_LEFT]:
			slew=slewSpeed # motor steps per second
		elif keys[pygame.K_RIGHT]:
			slew=-slewSpeed
		if keys[pygame.K_UP]:
			trolley=-trolSpeed
		elif keys[pygame.K_DOWN]:
			trolley=trolSpeed
		if keys[pygame.K_a]:
			hook=hookSpeed
		elif keys[pygame.K_z]:
			hook=-hookSpeed
	
	textPrint.print(screen,"{} {} {}".format(slew,trolley,hook))
	struct.pack_into('>B',buffer,0,settings)
	struct.pack_into('>BB',buffer,1,(slew&0x3FFF)>>7,slew&0x7F)
	struct.pack_into('>BB',buffer,3,(trolley&0x3FFF)>>7,trolley&0x7F)
	struct.pack_into('>BB',buffer,5,(hook&0x3FFF)>>7,hook&0x7F)
	
	if slew!=slewOld or trolley!=trolleyOld or hook!=hookOld or 1:
		try:
			sock.sendall(buffer) # send speeds to Arduino
		except:
			print("Could not send speeds.")
		else:
			slewOld=slew
			trolleyOld=trolley
			hookOld=hook
			settings |= 0b100000 # stop stopping
			settings &= ~0b10000 # stop homing
			textPrint.print(screen,"TCP on")
	if send:
		readSettings()
		struct.pack_into('>B',accelBuffer,0,settings|0b1000000)
		try:
			sock.sendall(accelBuffer) # sometimes send accelerations
		except:
			print("Could not send accelerations.")
		else:
			send=0
			print('Accelerations sent.')
	
	# ALL CODE TO DRAW SHOULD GO ABOVE THIS COMMENT
	
	# Go ahead and update the screen with what we've drawn.
	pygame.display.flip()
	
	# Limit to 20 frames per second
	clock.tick(20)
		
# Close the window and quit.
# If you forget this line, the program will 'hang'
# on exit if running from IDLE.
sock.close()
pygame.quit()