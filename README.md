# EZSYS Realtime Linux Praktikum

## 1 Vorbereitung

Ähnlich zum Yocto-Aufgabenblatt aus dem INTP-Praktikum im 3. Semester, werden wir wieder die WSL unter Windows 10 / 11 verwenden für die Entwicklung unserer Testanwendung.

```bash
mkdir C:\wsl\<name>
```

WSL prüfen:

```bash
wsl -l -v
```

---
### 1.1 Importieren des vorkonfigurierten WSL-Images

Das Image liegt unter `C:\wsl\export\yocto2024-basic-install.tar`. Der Import-Befehl der WSL ermöglicht es, ein vorkonfiguriertes Image zu laden. Dies geschieht mit dem folgenden Befehl (alle Pfade müssen absolut sein):

```bash
wsl --import yocto2024 C:\wsl\<name>\yocto2024 E:\wsl\export\yocto2024-basic-install.tar
```

Prüfen, ob `yocto2024` in WSL verfügbar ist, und als Default setzen:

```bash
wsl --set-default yocto2024
```

---
### 1.2 Installation der Entwicklungsumgebung

```bash
sudo apt update
sudo apt install -y apt-transport-https ca-certificates curl software-properties-common
```

Docker Repository hinzufügen:

```bash
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
```

```bash
echo "deb [signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
```

```bash
sudo apt update
sudo apt install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

User zur Docker-Gruppe hinzufügen:

```bash
sudo usermod -aG docker $(whoami)
```

WSL neu starten:

```bash
sudo shutdown -h now
```

Bzw. in der Powershell:
```ps
wsl.exe --shutdown
```

Oder neue Shell mit neuen Richtlinien:

```bash
newgrp docker
```

Wenn Sie das WSL Image aus dem Praktikum verwenden, wird folgendes Kommando notwendig sein, um den Docker Dienst zu starten:

```bash
sudo update-alternatives --set iptables /usr/sbin/iptables-legacy
sudo update-alternatives --set ip6tables /usr/sbin/ip6tables-legacy
```

Docker Service manuell starten:

```bash
sudo service docker start
```

Test, ob der Service läuft:

```bash
sudo service docker status
```

---
### 1.3 Laden des Code-Templates

Das für dieses Praktikum benötigte Code-Template wird als ZIP via Moodle bereitgestellt. Alternativ kann es per Git geladen werden:

```bash
git clone https://github.com/j-peller/ezsys_rt_linux.git
```

---
### 1.4 Aufbau des Code-Templates

Wechseln Sie in das `RPi_Signal`-Verzeichnis. Es enthält `src/`, `inc/`, sowie zwei Dockerfiles (für Ubuntu & Yocto). Relevante Dateien:

- `src/main.c`
- `inc/config.h`

GPIO-Pin auf [https://pinout.xyz](https://pinout.xyz) nachschlagen. Pin-Nummer und zugehöriger Chip mit `gpioinfo` oder `gpiofind` ermitteln:

```bash
gpiofind GPIO26
```
---
### 1.5 Hardware-Aufbau

Verbinden Sie das Oszilloskop mit dem Raspberry Pi: Messprobe an den gewählten GPIO-Pin, Masse an Ground-Pin.

---
### 1.6 Docker Cross-Compile für aarch64 (Raspberry Pi 4 / 5)

```bash
docker buildx create --use
```

```bash
docker buildx build --platform linux/arm64 --build-arg BUILD_TYPE=Release --output type=local,dest=./build . -f Dockerfile
```

## 2 Tests auf Standard-Ubuntu 24.04

### 2.1 Implementieren der Signalgenerierung

Sie sollen den Programmcode zur Signalgenerierung und Zeitmessung in der Datei `main.c` implementieren. Der Rumpf der vorgesehenen Thread-Funktion ist bereits gegeben. Fügen Sie Ihren Code an den entsprechend markierten Stellen ein.

Um ein periodisches Signal an einem GPIO-Pin zu erzeugen, müssen Sie den Pin mit der gewünschten Frequenz ein- und ausschalten. Die Frequenz kann entweder

- als Kommandozeilenparameter (`-f <FREQ>`) beim Start des Programms angegeben werden oder
- statisch in der `config.h` definiert werden.

Die Frequenz wird intern in die halbe Periodendauer in Nanosekunden umgerechnet und steht Ihnen über `param->half_period_ns` innerhalb der Thread-Funktion zur Verfügung.

Beispiel:

Wenn Sie ein Signal mit einer Frequenz von 1 Hz erzeugen möchten, starten Sie das Programm folgendermaßen:

```bash
./RPISignal -f 1
```

Die resultierende halbe Periodendauer beträgt 500.000.000 ns.

Implementieren Sie zunächst einen blockierenden Toggle-Mechanismus. Dabei soll der Thread nach jedem Toggle für die Dauer der Periode aktiv warten oder schlafen. Verwenden Sie hierfür z. B. die Funktion `clock_nanosleep()` aus der `time.h`-Bibliothek oder eine andere dafür passende Funktion.

Für die Ansteuerung der GPIO-Pins wird die Bibliothek `libgpiod` verwendet. Die Initialisierung des Pins erfolgt bereits durch die bestehende Initialisierungsroutine. Ihre Aufgabe ist lediglich, den Pin auf 0 oder 1 zu setzen. Hierfür verwenden Sie folgenden Funktionsaufruf:

```c
gpiod_line_set_value(gpio_line, value);
```

- `gpio_line` erhalten Sie über `param->gpio->line`
- `value` ist der gewünschte Ausgabewert (0 oder 1)

**Testen Ihrer Implementierung:**

1. Bauen Sie das Programm:

```bash
docker buildx build --platform linux/arm64 --build-arg BUILD_TYPE=Release --output type=local,dest=./build . -f Dockerfile
```

2. Übertragen Sie das erzeugte Binary auf den Raspberry Pi:

```bash
scp build/RPISignal pi@<IP>:
```

3. Testen Sie Ihre Implementierung mit einer gewünschten Frequenz und überprüfen Sie das erzeugte Signal mit einem Oszilloskop:

```bash
./RPISignal -f <FREQ> [-d <GPIOCHIP:GPIOPIN>]
```

Beispiel:

```bash
./RPISignal -f 1
```

---
### 2.2 Implementieren der Zeitmessung

In diesem Teil sollen Sie die Zeit messen, die in Software vergeht, bis der nächste GPIO-Toggle ausgelöst wird. Ziel ist es, die tatsächliche Periode zwischen den Umschaltungen zu erfassen und diese Werte zur späteren Auswertung in einem Ringpuffer zu speichern.

**Vorgehensweise:**

- Erfassen Sie zu Beginn einer Periode einen Zeitstempel
- Nach Ablauf der Periodendauer soll der GPIO-Pin getoggled werden
- Bestimmen Sie die verstrichene Zeit seit dem letzten Toggle und speichern Sie diese im bereitgestellten Ringpuffer
- Zum Speichern der Zeitdifferenzen steht Ihnen ein Helper-Macro zur Verfügung:

```c
WRITE_TO_RINGBUFFER(rbuffer, timestamp);
```

`rbuffer` erhalten Sie über `param->rbuffer`

**Hinweise zur Zeitmessung (mögliche Ansätze):**

- Sie können die Funktion `clock_gettime()` verwenden, um Zeitstempel mit Nanosekunden-Auflösung zu erfassen.
- Achten Sie darauf, dass Ihre Zeitmessung ausschließlich die Zeit zwischen zwei GPIO-Toggles erfasst und nicht durch weitere Funktionsaufrufe oder andere Berechnungen verfälscht wird.
- Stellen Sie sicher, dass die Zeitmessung innerhalb der Thread-Funktion erfolgt.

**Testen Ihrer Implementierung:**

1. Bauen Sie das Programm:

```bash
docker buildx build --platform linux/arm64 --build-arg BUILD_TYPE=Release --output type=local,dest=./build . -f Dockerfile
```

2. Übertragen Sie das erzeugte Binary auf den Raspberry Pi:

```bash
scp build/RPISignal pi@<IP>:
```

3. Führen Sie die Anwendung aus und exportieren Sie die Zeitdifferenzen in eine Datei:

```bash
./RPISignal -f 10 -o output.csv
```

Wenn Sie die Ergebnisse während der Ausführung live visualisieren möchten:

```bash
./RPISignal -f 10 -o output.csv -g
```

**Ausführen & CSV exportieren:**\*\*

```bash
./RPISignal -f 10 -o output.csv
./RPISignal -f 10 -o output.csv -g
```

---
### 2.3 Experimentieren
Test Sie Ihre Implementierung des Signalgenerators und beobachten Sie das Laufzeitverhalten Ihres erzeugten Signals insbesondere das Verhalten ihrer Frequenz und der in Software gemessenen tatsächliche Periode. 

#### 2.3.1 Was fällt Ihnen hinsichtlich der CPU-Auslastung auf?
Starten Sie die Anwendung mit der Option -c <0,1,2,3>, um den Thread zur Signalgenerierung auf einem bestimmen CPU-Kern zu fixieren und gucken Sie sich die Auslastung mit dem Tool top an. 

Mit CPU-Fixierung und live Visualisierung:

```bash
./RPISignal -f 100 -c 1 -g
```

#### 2.3.2 Nutzen Sie den CSV-Export und erstellen Sie eine Auswertung (Excel,...)
Visualisieren Sie die Verteilung der aufgezeichneten Zeitdifferenzen mithilfe eines Histogramms oder Box-Plot Diagramms
1. Wie verhält sich der Jitter?  
2. Wie verhält sich die Periode im Durchschnitt? 
3. Welche Worst-Cases können Sie beobachten? 


---
### 2.4 Implementieren eines Polling-Basierten Ansatzes
In Ihrer ersten Implementierung haben Sie eine blockierenden Aufruf wie `clock_nanosleep()` verwendet, um die gewünschte periodendauer abzuwarten.  Ziel dieser Aufgabe ist es nun, Ihre Implementierung zu modifizieren und stattdessen einen Polling-basierten Ansatz (busy-waiting) zu realisieren. 

**Hinweis**
- Achten Sie darauf, dass Ihre Zeitmessung weiterhin nur die tatsächliche Dauer zwischen zwei GPIO-Toggles umfasst.  
- Speichern Sie unbedingt Ihren aktuellen Code, da wir diesen für spätere Tests nochmal benötigen.

---
### 2.5 Experimentieren

#### 2.5.1 Vergleichen Sie die Ergebnisse der Polling-basierten Implementierung mit denen des blockierenden Ansatzes. 
Nutzen Sie das Tool perf, um einen tieferen Einblick in das Scheduling Verhalten zu erhalten. Mit folgendem Befehl starten Sie perf zeitgleich mit der Testanwendung. Wenn Sie die Signalgenerierung beenden, erhalten Sie eine Auswertung der aufgezeichneten Events auf dem CPU-Kern 1 (hier nur das Event context-switches): 

```bash
sudo perf stat -e context-switches -C 1 ./RPISignal -f 100 -c 1
```  

- Welche perf Events könnten weitere Einblick in das Scheduling Verhalten geben? 
  - Siehe: `perf list` und recherchieren Sie im Internet 

#### 2.5.2 Vor- und Nachteile des Polling-Ansatzes
Welche Vor- und Nachteile hat ein Polling-basierter Ansatz im Vergleich zum blockierenden Ansatz?

Denken Sie dabei z. B. an Reaktionszeit, CPU-Auslastung, Energieverbrauch und typische Einsatzszenarien in Echtzeitsystemen oder energieeffizienten Anwendungen. Wann ist welcher Ansatz sinnvoller?


## 3 Realtime Linux Kernel für Ubuntu (RPi4 & 5)

###  3.1 Abhängigkeiten auf dem Host installieren

Auf dem **Host-System** (z.B. Laborrechner, Laptop oder WSL unter Windows):

```bash
sudo apt update
sudo apt install -y git bc bison flex libssl-dev make libc6-dev libncurses5-dev crossbuild-essential-arm64 u-boot-tools device-tree-compiler libelf-dev wget rsync
```

---

### 3.2 Kernel-Quellcode von Raspberry Pi herunterladen

In einem Verzeichnis Ihrer Wahl (z.B. `/home/$(whoami)`)

```bash
git clone --depth=1 https://github.com/raspberrypi/linux.git -b rpi-6.16.y
cd linux
```

> Dies lädt den aktuellsten Raspberry Pi Linux-Kernel für die Version `6.16.y` herunter.
---

### 3.3 Cross-Compile Umgebung setzen

```bash
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
```

> ️Setzt die Zielarchitektur (`arm64`) und die Toolchain-Präfixe für das Cross-Kompilieren. Diese Umgebungsvariablen gelten für den aktuellen Terminal-Tab und wird benötigt, um den Kernel für den Raspberry Pi 5 bzw. Raspberry Pi 4 zu kompilieren.

---

###  3.4: Konfiguration vorbereiten und anpassen

#### Für Raspberry Pi 4 oder Pi 5:

```bash
make bcm2711_defconfig
```

#### Dann:

```bash
make menuconfig
```

#### In `menuconfig`:

```text
General setup  --->
  (X) Fully Preemptible Kernel (Real-Time)
```

```text
CPU Power Management --->
  CPU Frequency scaling --->
    ( ) CPU Frequency scaling deaktivieren
```

```text
Kernel Features --->
  Timer frequency --->
    (X) 1000 Hz
```

**Speichern und beenden**

---

### 3.5 Kernel, Module und Device Trees bauen

Im Verzeichnis `linux/`:

```bash
make -j$(nproc) Image modules dtbs
```

> Baut den Kernel (`Image`), Kernel-Module (`modules`) und Device Trees (`dtbs`) für den Raspberry Pi. 

----

### 3.6 Module installieren

Im Verzeichnis `linux/`:

```bash
mkdir ../modinstall
make INSTALL_MOD_PATH=../modinstall modules_install
```

> Installiert die kompilierten Module in ein temporäres Verzeichnis `../modinstall`, um sie später auf den Raspberry Pi zu übertragen.

---

### 3.7 Neuen Kernel + Module auf Raspberry Pi übertragen
```bash
rsync -av arch/arm64/boot/Image root@<ip>:/boot/firmware/vmlinuz
rsync -av arch/arm64/boot/dts/broadcom/*dtb arch/arm64/boot/dts/overlays root@<ip>:/boot/firmware/.
rsync -av --exclude='../modinstall/lib/modules/*/build' ../modinstall/lib/modules/* root@<ip>:/lib/modules/.
```

- `Image → /boot/firmware/vmlinuz`: ersetzt den Kernel.
- `*.dtb + overlays → /boot/firmware/`: aktuelle Device Trees für den Pi.
- `modules → /lib/modules/`: die neu gebauten Kernel-Module.

---

### 3.8 Neustart & Überprüfen der Kernel Version
Auf dem Raspberry Pi:

```bash
sudo reboot
```

> **Prüfe, ob der neue Kernel aktiv ist:**
>
> Der Befehl `uname -a` zeigt u. a. den Kernelnamen, die Version und das Build-Datum. Beispielausgabe:
>
> ```
> Linux ubuntu 6.16.0-rt1+ #1 SMP PREEMPT_RT Mon Jul 22 14:03:00 UTC 2024 aarch64 aarch64 aarch64 GNU/Linux
> ```
>
> - Der String `PREEMPT_RT` weist auf einen Echtzeitkernel hin.
> - Das Build-Datum hilft zu erkennen, ob es der frisch gebaute Kernel ist.

## 3 Tests auf einem Yocto-basierten Linux

### 3.1 Yocto Linux vorbereiten

Bevor Sie mit diesem Abschnitt beginnen, stellen Sie sicher, dass die Yocto-Umgebung gemäß dem Yocto-Arbeitsblatt aus dem INTP-Praktikum eingerichtet wurde (insbesondere Abschnitt 2.2).

Für die späteren Tests mit unserer Anwendung benötigen wir Tools und Bibliotheken im Yocto Image. Öffnen Sie dazu die Datei local.conf (build/conf/local.conf) in Ihrem Yocto Projekt und fügen Sie am Ende der Datei die folgenden Zeilen hinzu:

```bash
IMAGE_INSTALL:append = " openssh"
IMAGE_INSTALL:append = " libgcc"
IMAGE_INSTALL:append = " libgpiod"
IMAGE_INSTALL:append = " stress-ng"
IMAGE_INSTALL:append = " perf"
```

Die meta-raspberrypi-Layer stellt zwei verschiedene Kernel-Versionen bereit. Für unser Projekt verwenden wir die neuere Version 6.1. Dazu fügen Sie ebenfalls am Ende der local.conf die folgenden Zeilen hinzu:

```bash
PREFERRED_PROVIDER_virtual_kernel = "linux-raspberrypi"
PREFERRED_VERSION_linux_raspberrypi = "6.1%"
```

Stellen Sie außerdem sicher, dass Sie das Yocto-Image für die korrekte Hardware-Plattform erstellen. Öffnen Sie dazu die Datei local.conf und überprüfen bzw. setzen Sie den Eintrag MACHINE entsprechend Ihrer Zielplattform:

```bash
MACHINE ?= "raspberrypi5"
```

### 3.2 Yocto SDK vorbereiten

Um Ihre Anwendung für ein Yocto-basiertes Linux zu bauen, benötigen wir die Yocto-SDK. Dieses SDK enthält alle notwendigen Werkzeuge und Bibliotheken zum Cross-Kompilieren für die Zielplattform.

Wechseln Sie zunächst in Ihr Yocto-Projektverzeichnis und initialisieren Sie die Build-Umgebung:

```bash
source oe-init-build-env
```

Man wechselt automatisch in das Build-Verzeichnis. Zur Erstellung des SDKs führen Sie den folgenden bitbake-Befehl aus:

```bash
bitbake core-image-minimal -c populate_sdk
```

Nach erfolgreichem Abschluss befindet sich das SDK als Shell-Skript im folgenden Verzeichnis:

```bash
build/tmp/deploy/sdk/
```

Kopieren Sie das generierte Shell-Skript (Dateiendung .sh) in Ihren Projektordner und benennen Sie es in sdk.sh um:

```bash
cp build/tmp/deploy/sdk/poky*.sh ~/ezsys_rt_linux/sdk.sh
```

### 3.3 Bauen der Testanwendung für Yocto

Für das Bauen der Testanwendung für Yocto gibt es auch hier wieder ein vordefiniertes 
Dockerfile, das den Cross-Compile-Prozess in diesem Praktikum vereinfacht.  

Bauen Sie nun Ihre zuletzt entwickelte Signalgenerierung für den Raspberry Pi mit Yocto:

1. Bauen Sie das Programm:

```bash
sudo docker build -t rpisignal-crossbuild --output type=local,dest=./build . -f Dockerfile.yocto
```

2. Übertragen Sie das erzeugte Binary auf den Raspberry Pi:

```bash
scp build/RPISignal root@<IP>:
```

3. Auf dem Raspberry: Passen Sie die Berechtigungen des Programms vor Ausführung an:

```bash
chmod ug+x RPISignal
```

4. Auf dem Raspberry: Führen Sie die Anwendung aus und überprüfen Sie die 
Signalfrequenz am Oszilloskop:

```bash
./RPISignal -f 100 [-d <GPIOCHIP:GPIOPIN>]
```

### 3.4 Experimentieren

Führen Sie alle Tests aus Abschnitt 2 erneut unter Yocto aus und vergleichen Sie die Ergebnisse.

## 4 Linux-Kernel Optimierungen

### 4.1 Realtime Patches für den Linux Kernel

Seit 2005 gibt es für den Linux-Kernel sogenannte Realtime Patches (PREEMPT\_RT), die ursprünglich unabhängig vom offiziellen Kernel-Entwicklungszweig entwickelt wurden. Im Jahr 2021 wurden zentrale Bestandteile des PREEMPT\_RT-Patches in den Mainline-Linux-Kernel integriert.

Im Yocto-Layer für den Raspberry Pi sind diese Realtime-Patches jedoch standardmäßig nicht enthalten und müssen daher manuell eingebunden werden. Gehen Sie dazu wie folgt vor:

1. Wechseln Sie in das Hauptverzeichnis Ihrer Yocto-Installation (dort, wo sich die anderen Yocto-Layer befinden), und klonen Sie die benötigte Layer:

```bash
git clone -b yocto-rpi-rt-6.1-patch git@github.com:j-peller/ezsys_rt_linux.git meta-rpi-signalgen
```

2. Fügen Sie die neue Layer in Ihre Yocto-Umgebung ein:

```bash
bitbake-layers add-layer meta-rpi-signalgen
```

3. Überprüfen Sie, ob die Layer erfolgreich hinzugefügt wurde:

```bash
bitbake-layers show-layers
```

Um den Realtime-Patch zu aktivieren, muss die Kernel-Konfiguration entsprechend angepasst werden. Starten Sie die interaktive Konfigurationsoberfläche:

```bash
bitbake -c menuconfig virtual/kernel
```

Navigieren Sie dort zu folgendem Menüpunkt und aktivieren Sie die Option „Fully Preemptible Kernel“:

```
General Setup → Preemption Model → Fully Preemptible Kernel
```

Nachdem Sie die Konfiguration gespeichert haben, bauen Sie ein neues Yocto-Image und flashen Sie es auf die SD-Karte des Raspberry Pi:

```bash
bitbake core-image-minimal
```

### 4.2 Periodische Interrupts Deaktivieren (tickless mode)

Linux-Systeme wie Yocto nutzen standardmäßig periodische Timer-Interrupts (Ticks), um Aufgaben wie das Prozess-Scheduling oder Systemzeiterfassung durchzuführen. Diese Interrupts treten in festen Abständen auf, egal ob sie aktuell benötigt werden und beeinträchtigen damit die Echtzeitfähigkeit des Systems.

Sehen Sie sich die Gesamtanzahl der Interrupts seit Systemstart mit folgendem Befehl an:

```bash
cat /proc/interrupts
```

**Aufgabe:** Welche Interrupts treten besonders häufig auf und auf welchem CPU-Kern?

Mit der Kernel-Option `CONFIG_NO_HZ_FULL` lässt sich dieser periodische Interrupt für bestimmte CPUs vollständig deaktivieren. Dadurch können betriebssystembedingte Verzögerungen minimiert werden und echtzeitkritische Tasks auf einer dafür vorgesehenen CPU ausgeführt werden.

Navigieren Sie zum Menüpunkt „Timer tick handling“ und aktivieren Sie „Full dynticks system“:

```
General Setup → Timers subsystem → Timer tick handling → Full dynticks system
```

Abschließend muss festgelegt werden, welche CPU(s) von NO\_HZ\_FULL profitieren sollen, also keine periodischen Scheduling-Interrupts mehr erhalten, solange sie idle sind oder nur eine Userspace-Task ausführen. Um dies zu konfigurieren, fügen Sie in die `local.conf` Ihres Yocto-Builds folgende Zeile ein:

```bash
CMDLINE:append = " nohz_full=1 isolcpus=1"
```

Hiermit wird der CPU-Kern 1 von periodischen Interrupts ausgenommen und isoliert, sodass der Scheduler keine Tasks auf diesem Kern einplant. General Setup → Timers subsystem → Timer tick handling → Full dynticks system

````

In `local.conf` einfügen:

```bash
CMDLINE:append = " nohz_full=1 isolcpus=1"
````

Interrupts prüfen:

```bash
cat /proc/interrupts
```