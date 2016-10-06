# -*- coding: utf-8 -*-

#  Raspberry Pi videocontroller
#
#  Harm Aldick - 2016
#  https://github.com/kitesurfer1404/leftovers/videoctrl
#
#
#  LICENSE
#  
#  The MIT License (MIT)
#
#  Copyright (c) 2016  Harm Aldick 
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.
#
#
#  CHANGELOG
#
#  2016-10-06   Initial beta release


import RPi.GPIO as GPIO
import time
import subprocess

# Hier die Pins einstellen.
# Pin-Nummerierung nach https://www.raspberrypi.org/documentation/usage/gpio-plus-and-raspi2/
# Der erste Pin ist der Ausschalter. Bei Schalten gegen Masse fährt der Raspi herunter.
# Die folgenden Pins starten die Videos. Der Index in der Liste gibt dabei die Videonummer an.
# Alle Pins werden softwareseitig als Input mit Pull-Up-Widerstand konfiguriert.
pins = [26, 19, 13, 6, 5]


video_dir = "/home/pi/Videos/"           # In diesem Verzeichnis liegen die Videos
video_extension = ".mp4"                 # Diese Endung haben alle Videos
title_img = "/home/pi/Pictures/title.jpg"  # Dieses Bild wird zwischen den Videos angezeigt


# Ab hier muss eigentlich nichts mehr eingestellt werden...

fbi = subprocess  # Globale Variable für den fbi-Prozess
omx = subprocess  # Globale Variable für dem omxplayer-Prozess


GPIO.setmode(GPIO.BCM) # Alle IOs gemäß Broadcom-Chip nummerieren

# Alle Pins auf INPUT mit Pullup-Widerstand stellen
for p in pins:
  GPIO.setup(p, GPIO.IN, pull_up_down=GPIO.PUD_UP)


def shutdown():
  proc = subprocess.Popen(["sudo", "halt"])
  return

def start_fbi():
  global fbi
  fbi = subprocess.Popen(["fbi", "--noverbose", title_img])
  return

def fbi_running():
  global fbi
  try:
    ret = fbi.poll()
    #print "Prüfe fbi..."
    #print ret
    if ret is not 0:
      #print "Läuft noch"
      return True
  except Exception:
    pass
  return False


def kill_fbi():
  global fbi
  try:
    fbi.stdin.write('q')
    fbi.terminate()
    fbi.wait()
  except Exception:
    pass
  return

def start_omx( file ):
  global omx
  omx = subprocess.Popen(["omxplayer", "-b", file], stdin=subprocess.PIPE)
  return

def omx_running():
  global omx
  try:
    ret = omx.poll()
    #print "Prüfe omx..."
    #print ret
    if ret is not 0:
      #print "Läuft noch!"
      return True
  except Exception:
    pass
  return False

def kill_omx():
  global omx
  try:
    omx.stdin.write('q')
    omx.terminate()
    omx.wait()
  except Exception:
    pass
  return



start_fbi()

try:
  while True:
    button_pressed = False
    video_id = 0

    if not GPIO.input(pins[0]):
      shutdown()

    for i in range(1, len(pins)):
      if not GPIO.input(pins[i]):
        #print "Schalter %d gedrückt." % i
        button_pressed = True
        video_id = i

    if button_pressed:
      if fbi_running():
        #print "Beende fbi."
        kill_fbi()

      if omx_running():
        #print "Beende omx."
        kill_omx()

      #print "Starte Video %s%d%s" % (video_dir, video_id, video_extension)
      start_omx(video_dir + str(video_id) + video_extension)
    else:
      if not fbi_running() and not omx_running():
        #print "fbi ist aus. Starte fbi."
        start_fbi()
        
except KeyboardInterrupt:
  GPIO.cleanup()
  print "\n"
  print "Tschüss! Einen schönen Tag noch!"
