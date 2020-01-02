# Hello, Joel here. With this code you can control one crane with your computer's keyboard through ethernet or USB cable. Use arrow keys, and A, Z, L, H, S, U keys.

# libraries you may need to install with pip
import pygame # pip install pygame
import serial # pip install pyserial
import serial.tools.list_ports

# comes with Python 3.8.1
import struct
import threading
import time
import socket
from tkinter import * 

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
		with open("settings.txt") as f:
			pass
	except FileNotFoundError:
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
			print('settings.txt file created.')
		finally:
			f.close()
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
		print('Speeds:  {}  {}  {}'.format(slewSpeed,trolSpeed,hookSpeed))
	finally:
		f.close()
	struct.pack_into('>B',accelBuffer,0,settings|0b1000000)
	struct.pack_into('>BB',accelBuffer,1,(slewAccel&0x3FFF)>>7,slewAccel&0x7F)
	struct.pack_into('>BB',accelBuffer,3,(trolAccel&0x3FFF)>>7,trolAccel&0x7F)
	struct.pack_into('>BB',accelBuffer,5,(hookAccel&0x3FFF)>>7,hookAccel&0x7F)
readSettings()

ser=None
cat=None
say=False
def serStop():
	global ser
	global cat
	global say
	if ser is not None:
		ser.close()
	ser=None
	cat=None
	say=False

ikkuna=Tk()
ikkuna.title("Asetukset")
#ikkuna.geometry('250x120')
valikko=Menu(ikkuna)
#valinta=Menu(valikko,tearoff=0)
#valikko.add_cascade(label='Profiili',menu=valinta)
#valinta.add_command(label='Hidas')
#valinta.add_command(label='Nopea')
#valinta.add_command(label='Turbo')
ikkuna.config(menu=valikko)
Label(ikkuna,text='Mode:').grid(column=0,row=0)
mode=IntVar()
mode.set(1)
Radiobutton(ikkuna,text='To crane',value=1,variable=mode).grid(column=1,row=0)
Radiobutton(ikkuna,text='To relay',value=2,variable=mode).grid(column=2,row=0)
Radiobutton(ikkuna,text="I'm relay",value=3,variable=mode).grid(column=3,row=0)
Label(ikkuna,text='Output:').grid(column=0,row=1)
output=IntVar()
output.set(1)
Radiobutton(ikkuna,text='Off',value=1,variable=output).grid(column=1,row=1)
USBbutton=Radiobutton(ikkuna,text='USB',value=2,variable=output)
USBbutton.grid(column=2,row=1)
Radiobutton(ikkuna,text=IP,value=3,variable=output).grid(column=3,row=1)
IPlabel=Label(ikkuna)
IPlabel.grid(columnspan=4)

def monitorUSB(fan): # prints whatever arduino sends us
	while True:
		try:
			print('Crane: '+fan.readline().rstrip().decode())
		except:
			break

def monitorTCP():
	try:
		sockPos.connect((IP,10001))
	except:
		print("Failed to connect to IP {} port 10001.".format(IP))
		output.set(1)
	else:
		print("Connected to IP {} port 10001.".format(IP))
		while sockConnected:
			sockPos.sendall(bytes(1))
			data=sockPos.recv(32)
			print('Crane: '+data.rstrip().decode())
			time.sleep(1)
	finally:
		sockPos.close()
		print("Disconnected from port 10001.")

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
screen = pygame.display.set_mode([250, 100]) # screen size [width,height]
pygame.display.set_caption("keyboard")
clock = pygame.time.Clock() # Used to manage how fast the screen updates
textPrint = TextPrint() # Get ready to print

old=0
wax=0

sockSpd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sockSpd.settimeout(2)
sockConnected=False
sockPos = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sockPos.settimeout(2)

# -------- Main Program Loop -----------
done = False #Loop until the user clicks the close button.
while done==False:
	myIP=socket.gethostbyname(socket.gethostname())
	IPlabel.config(text='IP of this computer is '+myIP)
	ikkuna.update()

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

	if mode.get()==2:
		USBbutton.config(state=DISABLED)
		if output.get()==2:
			output.set(1)
	else:
		USBbutton.config(state='normal')

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
	
	if mode==1:
		struct.pack_into('>B',buffer,0,settings)
		struct.pack_into('>BB',buffer,1,(slew&0x3FFF)>>7,slew&0x7F)
		struct.pack_into('>BB',buffer,3,(trolley&0x3FFF)>>7,trolley&0x7F)
		struct.pack_into('>BB',buffer,5,(hook&0x3FFF)>>7,hook&0x7F)

	if output.get()==2: # send via USB
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
							print('Kytke Arduino USB johto.')
			else:
				try:
					ser = serial.Serial(cat,250000) # port, baud rate
				except:
					pass
				else:
					threading.Thread(target=monitorUSB, args=(ser,)).start()
		else: # send data to arduino
			try:
				ser.write(buffer) # send speeds
			except:
				serStop()
			else:
				settings |= 0b100000 # stop stopping
				settings &= ~0b10000 # stop homing
				textPrint.print(screen,"USB on")
			if send:
				readSettings()
				try:
					ser.write(accelBuffer) # sometimes send accelerations
				except:
					serStop()
				else:
					send=0
					print('Accelerations sent.')
	else:
		serStop()
	if output.get()==3: # send via TCP
		if not sockConnected:
			try:
				sockSpd.connect((IP,10000))
			except:
				print("Failed to connect to IP {} port 10000.".format(IP))
				output.set(1)
			else:
				print("Connected to IP {} port 10000.".format(IP))
				sockConnected=True
				threading.Thread(target=monitorTCP).start()
		else:
			try:
				sockSpd.sendall(buffer) # send speeds to Arduino
			except:
				print("Could not send speeds. Disconnected from port 10000.")
				sockSpd.close()
				sockConnected=False
			else:
				settings |= 0b100000 # stop stopping
				settings &= ~0b10000 # stop homing
				textPrint.print(screen,"TCP on")
			if send:
				readSettings()
				try:
					sockSpd.sendall(accelBuffer) # sometimes send accelerations
				except:
					print("Could not send accelerations. Disconnected from port 10000.")
					sockSpd.close()
					sockConnected=False
				else:
					send=0
					print('Accelerations sent.')
	elif sockConnected:
		print("Disconnected from port 10000.")
		sockSpd.close()
		sockConnected=False

	if settings&0b1000:
		textPrint.print(screen,'silent mode')
	if settings&0b100:
		textPrint.print(screen,'lights on')
	pygame.display.flip() # update numbers on pygame window
	clock.tick(20) # Limit to 20 frames per second

sockSpd.close()
serStop()
pygame.quit()