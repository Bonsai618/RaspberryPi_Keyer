#!/usr/bin/env python
"""
WA2EIN Morse Code Keyboard.  1/16/2016

The program interfaces with a PIC co-processor to translate
keystrokes into morse code.

The co-processor is used to establish accurate code timing because
the Raspberry Pi timing is not deterministic.  The Raspberry Pi runs Linux
which dispatches background tasks and processes.  This upsets the intended
timing.

Connection to the co-processor is established via the seial interface at
38400 BPS.  The serial interface was chosen because of the pre-existance of
serial code on the PIC.

Keystroked are buffered by the Que class.  A background task is used to dequeue
messages and send them to the PIC.

Code in the PIC provides additional buffering.  The PIC is interrupt driven so that
the PIC can drive its output port with the required timing.  The PIC USART provides
additional hardware buffering.  Interrupts are taken with each available character.  The
USART characters are appended to a receive buffer with minimal timing overhead.

The message interface between the Raspberry Pi and the PIC processor is
a variable length message packet.  It contains a Start character, Length code,
Message type code and then a variable length message.  This allows for future
expansion.

*******************************************************************
Variable length messages start with X'FF', then a one byte message length
     followed by a message type code and then the message.
       Message types:   C = Code
                        S = Code speed
                        K = Keydown / Kepup toggle
*******************************************************************

"""


import sys
from gi.repository import Gtk
import Queue
import threading
import time
import serial
import RPi.GPIO as GPIO
import MySQLdb




'''
The user may modify the following table to add pro-signs to his liking
'''
GladeFile = 'WA2EINKeyboardsmall.glade'


CQ_Msg = 'CQ CQ CQ CQ CQ DE WA2EIN WA2EIN {'


# Database fields for logging

Stn = ''
RST = '5nn'
Rig = 'Orion'
Power = '25 W'
name = ''
QTH = ''
Notes = ''
His_notes = 'Notes'
CurrectTime = ''
conn = 0
SHIFT = 0

# My defaults
CWSpeed = '20'
MyCall = 'wa2ein'


# global definitions for Glade fields

CallEntry = 0
RSTEntry = 0
QTHEntry = 0
NameEntry = 0
NotesEntry = 0
textview1 = 0
Speed = 20



# Try to establish database connection

try:
    conn = MySQLdb.connect(host='localhost',user='pi',passwd='shazam',db='qso')
except:
    print("You dont have permission to access the qsoDB database")
    print("You must setup and get permission to access qsoDB")
    
    exit()
    
print("Connection to qsoDB is open")

    
'''  Keyboard codes.   Modify as necessary. '''
CR = 65293
CODE = {'A': '.-',     'B': '-...',   'C': '-.-.', 
        'D': '-..',    'E': '.',      'F': '..-.',
        'G': '--.',    'H': '....',   'I': '..',
        'J': '.---',   'K': '-.-',    'L': '.-..',
        'M': '--',     'N': '-.',     'O': '---',
        'P': '.--.',   'Q': '--.-',   'R': '.-.',
     	'S': '...',    'T': '-',      'U': '..-',
        'V': '...-',   'W': '.--',    'X': '-..-',
        'Y': '-.--',   'Z': '--..',
        
        '0': '-----',  '1': '.----',  '2': '..---',
        '3': '...--',  '4': '....-',  '5': '.....',
        '6': '-....',  '7': '--...',  '8': '---..',
        '9': '----.',  ' ': ' ',      '/':'-..-.',
        '.':'.-.-.-',  '?':'..--..',  ',':'--..--',
        '=':'X',       '-':' -...-',   '>':'...-.-',
        ':':'-.-. .-..','{':'.-.-.', '}':'-.- -.', 
        '!':'-.-. --.-  -.-. --.-  -.-. --.-  -.. .  .-- .- ..--- . .. -.   .-- .- ..--- . .. -.   .-.-.',
        '[':'U', ']':'D', '+':'F', '|':'B'
        }

global AutoMsg
AutoMsg = ''


# CW Buffer Queue
q = Queue.Queue()
global main_window
global about_dlg



def CheckServer():
        global conn


        try:
           
            c = conn.cursor()
            c.execute("SELECT * FROM station")

            rows = c.fetchall()
            print("SQL Server operationsl")

            
                
        except MySQLdb.Error, e:
            
            print ' %s ' %(e)
        except:
            print("Error accessing SQL server")
            exit()
            


def MyThread ():
    global AutoMsg
   
    print("MyThread started")
    
    ser = serial.Serial (
        port = '/dev/ttyAMA0',
        baudrate = 38400,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
        )
    
    print("Serial opened")
    
    
    GenMsg = ""
    
    
    while True:
        # dequeue and translate keyboard characters
        try:

            #self imposed pacing to prevent overrun 
            time.sleep(.01)
            str =(CODE[q.get().upper()])
            #print(str)
            GenMsg =str + ' '
            for i, c in enumerate(GenMsg):
                time.sleep(.01)
                ser.write(c)
                #ser.write(GenMsg)
                #print(c)
                
            if (GenMsg == 'X '):
                
                time.sleep(2)
                ser.write(' ')
                print("Reset Serial")
           
        except:
            print 'err'



def SendMsg(msg):
    
    for i, c in enumerate(msg):
        time.sleep(.03)
        q.put(c)
    
            
            
# callback routines

class Handler:
    global Speed
    
    def onDeleteWindow(self, *args):
        
        Gtk.main_quit(*args)

    def Close(*args):
        about_dlg.hide()
        
    def Destroy(*args):
        # About dialog destroyed, need to rebuild it
        global about_dlg
        about_dlg.destroy()
        LoadDialog()
        about_dlg.show_all()
        about_dlg.hide()

    def About(delf, *args):
        global about_dlg
        about_dlg.show_all()
     
    def on_keypress(self,widget,event,data=None):
        global SHIFT
        # Queue keystrokes for CW generation
        print event.keyval

        # Enter Key

        # Shift Key
        if event.keyval == 65507 or event.keyval == 65508:
            SHIFT=1

        # Space key     
        if event.keyval == 65293:
            q.put(' ')
            q.put(' ')



        #F12
        if event.keyval == 65481:
            SendMsg('[[[[[')

            Set = int(Speed.get_text())
            Set = Set + 5
            Speed.set_text(str(Set))
           

        #F11 
        if event.keyval == 65480:
            SendMsg(']]]]]')
            
            Set = int(Speed.get_text())
            Set = Set - 5
            Speed.set_text(str(Set))
           


        #F1
        if event.keyval == 65470:
            SendMsg(CQ_Msg)

            
        #F2
        if event.keyval == 65471:
            Call_Msg = Stn + ' ' + Stn + ' ' + Stn + ' de ' + MyCall + ' ' + MyCall + ' {'
            SendMsg(Call_Msg)

        #F3
        if event.keyval == 65472:
            Call_Msg = Stn + ' de ' + MyCall + ' R' 
            SendMsg(Call_Msg)

        #F4
        if event.keyval == 65473:
            Msg = RST + ' '+ RST + ' va  va  pete  pete '
            SendMsg(Msg)

        #F5
        if event.keyval == 65474:
            Msg = ' Rig QRP QRP 5w  5w  ant g5rv  g5rv up 75 feet  75 feet '
            SendMsg(Msg)
            
        #F6     
        if event.keyval == 65475:
            Msg = 'de ' + MyCall
            SendMsg(Msg)

        #F9
        if event.keyval == 65478:
            Msg = 'vvv vvv vvv vvv vvv de ' + MyCall + ' {'
            SendMsg(Msg)

    
        # Normal data key pressed
        if event.keyval < 257:
            
            c = chr(event.keyval)


        # Prosign  Section
        
            # AR  - New Page
            if SHIFT == 1 and c == 'a' :
                Msg = '{'
                SendMsg(Msg)
                SHIFT=0
                c = 0

            # BT  - Page 2
            if SHIFT == 1 and c == 'b' :
                Msg = '<'
                SendMsg(Msg)
                SHIFT=0
                c = 0

                
            # CL - Close
            if SHIFT == 1 and c == 'c' :
                Msg = ':'
                SendMsg(Msg)
                SHIFT=0
                c = 0


            # KN  Invitation Named Station
            if SHIFT == 1 and c == 'k' :
                Msg = '}'
                SendMsg(Msg)
                SHIFT=0
                c = 0

            #SK - End of Contact
            if SHIFT == 1 and c == 's' :
                Msg = '>'
                SendMsg(Msg)
                SHIFT=0
                c = 0   

            
            if c > 0:
                
                q.put(c)

    def on_WPM(self,widget,event,data=None):
        global CWSpeed
        WPM = widget.get_text()


        # if CR then process WPM
        if (event.keyval == 65293):
            print("Set Speed " + WPM)

            Set = int(WPM)
            if(Set < int(CWSpeed)):
                Ct = int(CWSpeed) - Set
                CWSpeed = WPM
                while (Ct > 0):
                    q.put(']')
                    Ct = Ct - 1
            else:
                Ct = Set - int(CWSpeed)
                CWSpeed = WPM
                while (Ct > 0):
                    q.put('[')
                    Ct = Ct - 1
                
            #print(Ct)


    def on_cq(self,button):

        for i, c in enumerate(CQ_Msg):
            time.sleep(.03)
            q.put(c)

    def on_reset(self,button):
        q.put('=')
        Speed.set_text('20')
        print("Reset")
        

    def on_xch(self,button):
        global Stn
        for i, c in enumerate(Stn + " de wa2ein"):
            time.sleep(.03)
            q.put(c)
        

    def on_call_entry(self,widget,event,data=None):
        global Stn
        Stn = widget.get_text()
        #print(Stn)

    def on_call_stn(self,button):
        global MyCall
        msg = Stn + ' ' + Stn + ' ' + Stn + ' de ' + MyCall + ' ' +  MyCall + ' }'
        for i, c in enumerate(msg):
            
            time.sleep(.03)
            q.put(c)
        
        
    def on_reply(self,button):
        global MyCall
        msg = Stn + ' de ' + MyCall +' r '
        for i, c in enumerate(msg):
            
            time.sleep(.03)
            q.put(c)

    def on_report(self,button):
        global MyCall
        global RST
        #msg = Stn + ' de ' + MyCall +' r '
        msg = RST + ' ' + RST + '  va  va   pete  pete  hw ? BK'
        for i, c in enumerate(msg):
            
            time.sleep(.03)
            q.put(c)



    def on_paris(self,button):
        
        msg = 'PARIS PARIS PARIS PARIS PARIS PARIS PARIS PARIS PARIS PARIS'
        for i, c in enumerate(msg):
            
            time.sleep(.03)
            q.put(c)


    
    def on_RST(self,widget,event,data=None):
        global RST
        RST = widget.get_text()
        #print(RST)

    def on_name(self,widget,event,data=None):
        global name
        name = widget.get_text()
        #print(name)

    def on_QTH(self,widget,event,data=None):
        global QTH
        QTH = widget.get_text()
        #print(QTH)

    def on_log(self,button):
        global Stn
        global RST
        global Rig
        global Power
        global QTH
        global name
        global Notes
        global His_notes
        global conn
        global CurrentTime

        global CallEntry
        global RSTEntry
        global QTHEntry
        global NameEntry
        global NotesEntry
        

        CurrentTime = time.strftime("%c")
        print(CurrentTime)
        
        Stn = Stn.upper()
        name = name.upper()
        QTH = QTH.upper()
        Notes = NotesEntry.get_text()
        Notes = Notes.upper()

        # Clear Fields after Logging
        CallEntry.set_text("")
        RSTEntry.set_text("")
        QTHEntry.set_text("")
        NameEntry.set_text("")
        NotesEntry.set_text("")
        
        print(Stn)
        print(name)
        print(QTH)
        print(Notes)
            


        c = conn.cursor()


        # Duplicate Key is normal if Station exists.
        try:
            
            c.execute("""INSERT INTO station
            (station_call,name,QTH,notes)
            VALUES (%s,%s,%s,%s)""",
            (Stn,name,QTH,Notes))

    
            conn.commit()
            print("station table updated")
        except MySQLdb.Error, e:
            print ' %s ' %(e)

        except:
            print("station table changes rolled back") 
            conn.rollback()


        #Should not get an error.
        try:

            
            c.execute("""INSERT INTO qso
            (qso_datetime,station_call,RST,rig,power,notes)
            VALUES (%s,%s,%s,%s, %s, %s)""",
            (CurrentTime,Stn,RST,Rig,Power,His_notes))
            conn.commit()
            print("qso table updated")
        except MySQLdb.Error, e:
            print ' %s ' %(e)
        except:
            print("qso table changes rolled back") 
            conn.rollback()
        
        

        c.execute("SELECT * FROM station")

        rows = c.fetchall()

        for row in rows:
            print(row)

        #conn.close()



    
        

def LoadGladeDefinitions():
    global about_dlg
    global builder
    global CallEntry
    global RSTEntry
    global NameEntry
    global QTHEntry
    global NotesEntry
    global Speed
    
    print("Loading Glade")
    builder = Gtk.Builder()
    builder.add_from_file(GladeFile)
    builder.connect_signals(Handler())
    main_window = builder.get_object("window1")
    CallEntry = builder.get_object("Callentry1")
    RSTEntry = builder.get_object("Callentry2")
    NameEntry = builder.get_object("Callentry3")
    QTHEntry = builder.get_object("Callentry4")
    NotesEntry = builder.get_object("entry2")
    Speed = builder.get_object("WPM")
    Speed.set_text("20")
    about_dlg = builder.get_object("aboutdialog")
    main_window.show_all()
    about_dlg.show_all()
    about_dlg.hide()


def LoadDialog():
    global about_dlg
    global builder
    builder = Gtk.Builder()
    builder.add_from_file(GladeFile)
    builder.connect_signals(Handler())
    about_dlg = builder.get_object("aboutdialog")
    about_dlg.show_all()
    about_dlg.hide()

   

    
def main():
# Main Logic
    print("WA2EIN Morse Keyboard for Raspberry Pi")
    
    LoadGladeDefinitions()

    CheckServer()

#Start character dequeue and Morse translate thread.
   
    threading.Thread(target=MyThread).start()
    
    Gtk.main()
    

    print("Done")

main()      
    
         
  
  

 
