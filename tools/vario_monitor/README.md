# VarioUSB Monitor & Flight Recorder (GUI)

Cette application de bureau Python permet de visualiser en direct les trames NMEA `$LK8EX1` émises par le dongle **VarioUSB (Seeed XIAO SAMD21 + BMP390)**, d'enregistrer des sessions de vol au format standard `.nmea` et `.csv`, et de diffuser le flux de données en temps réel vers l'application mobile Android **`VarioAppli`** (située dans `../VarioAppli`).

---

## Fonctionnalités

1. **Détection & Connexion Série :**
   - Détection automatique des ports COM sous Windows.
   - Support du débit par défaut **115200 bauds**.
   - **Mode Simulateur de Vol Intégré** : permet de générer un vol thermique dynamique (montées +2.5 m/s, transitions -1.8 m/s, variations de pression réalistes à 10 Hz) même si la carte électronique n'est pas encore branchée au PC.

2. **Tableau de Bord Télémétrique Temps Réel :**
   - **Vario (Vz)** en m/s et cm/s avec code couleur dynamique (vert en thermique, rouge en dégueulante).
   - **Pression barométrique** en hPa et Pascals.
   - **Altitude barométrique estimée** (formule standard OACI QNH) et gain relatif.
   - **Température** ambiante (°C).
   - **Fréquence du flux** (10.0 Hz) et taux d'intégrité des trames (checksum NMEA XOR).

3. **Enregistreur de Fichiers de Vol (Logger) :**
   - Boutons **Démarrer** / **Arrêter** l'enregistrement.
   - Enregistrement brut au format `.nmea` (compatible avec XCSoar, LK8000, XCTrack).
   - Export synchronisé au format `.csv` avec horodatage pour analyse dans Excel / Python / Pandas.
   - Bouton direct pour ouvrir le dossier des enregistrements (`logs/`).

4. **Pont Réseau TCP pour `VarioAppli` :**
   - Serveur TCP non bloquant multi-clients sur le port **8888**.
   - Diffuse instantanément chaque trame LK8EX1 reçue (ou simulée) vers tout client connecté.

---

## Démarrage Rapide

### Prérequis
- Python 3.10+ (installé sur le système, accessible via `py` ou `python`).
- Dépendance série : `pyserial` (déjà installée ou `py -m pip install -r requirements.txt`).

### Lancement
Vous pouvez lancer l'application de deux façons :
- **Double-clic sur :** `tools/vario_monitor/run_monitor.bat`
- **Ou en ligne de commande :**
  ```powershell
  cd tools/vario_monitor
  py main.py
  ```

---

## Comment relier les données à VarioAppli (`../VarioAppli`)

L'application Android native `VarioAppli` est conçue pour exploiter les trames `$LK8EX1` :

### 1. Sur le terrain : Connexion directe USB OTG (Smartphone)
Branchez le dongle USB Seeed XIAO directement sur votre smartphone Android à l'aide d'un adaptateur USB-C OTG. Le `VarioService` de `VarioAppli` (via `usb-serial-for-android`) ouvre automatiquement la liaison à 115200 bauds et alimente la boucle audio temps réel (`VarioAudioEngine`).

### 2. En développement / test sur PC : Pont Réseau TCP
Lorsque vous développez `VarioAppli` sur votre PC (émulateur Android Studio ou smartphone connecté au PC) :
1. Dans le moniteur Python, cochez **"Activer Serveur TCP (Port 8888)"**.
2. Sélectionnez soit le port COM de votre dongle, soit le **Simulateur Intégré**.
3. Côté Android :
   - **Émulateur Android :** peut se connecter à `10.0.2.2:8888` (IP loopback de l'hôte sous l'émulateur Android).
   - **Smartphone en WiFi :** peut se connecter à `<IP_DE_VOTRE_PC>:8888`.
   - **Via ADB :** exécutez `adb reverse tcp:8888 tcp:8888` pour mapper directement le port du téléphone vers le PC.

### 3. Validation par Rejeu de Logs
Les fichiers `.nmea` enregistrés dans `tools/vario_monitor/logs/` peuvent être importés dans les tests unitaires de `VarioAppli` (`Lk8ex1ParserTest.kt`) pour vérifier la robustesse du parseur sans allocation mémoire et le calibrage de l'audio.
