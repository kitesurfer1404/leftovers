videoctrl - ein Skript, das verschiedene Videos auf Knopfdruck abspielt
=======================================================================

videoctrl ist z.B. für Museen geeignet, die interaktiv Videos auf Knopfdruck abspielen wollen. Der Code ist für den Raspberry Pi ausgelegt. Eine weitere Funktion ist das saubere Herunterfahren auf Knopfdruck.

Installation
------------

* [Raspbian](https://www.raspberrypi.org/downloads/raspbian/) runterladen und installieren
* Per raspi-config Auto-Login in die Konsole aktivieren
* sudo apt-get install fbi
* videoctrl.py in /home/pi ablegen
* Videos (1.mp4, 2.mp4, 3.mp4, ...) in /home/pi/Videos ablegen.
* "Startbild" title.jpg in /home/pi/Pictures ablegen
* /home/pi/.bashrc anpassen: folgenden Code am Dateiende einfügen:

```bash
if [ -n "$SSH_CLIENT" ] || [ -n "$SSH_TTY" ]; then
  echo "Starte das Video-Skript nicht über ssh."
else
  python ~/videoctrl.py
fi
```

* sudo reboot
