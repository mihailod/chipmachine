ChipMachineAS
=============

**Port of ChipMachine (see the fork info) for Apple Silicon.**

**ONLY BUILDING ON APPLE SILICON (ARM / M CHIPS) HAS BEEN TESTED -- INSTALL AT YOUR OWN RISK.**

**BUILD SCRIPTS (FOR NOW) EXPLICITLY TARGET ONLY THE VERY LATEST MAC OS (26 / TAHOE) AND HAVE NOT BEEN TESTED ON OLDER VERSIONS.**

**BINARY RELEASE TESTED ON: TAHOE, SONOMA**

**I AM USING GOOGLE GEMINI TO HELP ME WITH THIS INTEL -> ARM PORT. SO IT IS ALSO AN EXPERIMENT IN HOW AI PERFORMS AT THIS TASK.**

![Screenshot](https://raw.githubusercontent.com/mihailod/chipmachine/master/screen.png)

See the demo (turn the sound on!):
<video src="https://github.com/user-attachments/assets/66982f37-8245-4e18-9716-09f6d2f2bc3a" width="100%" autoplay loop playsinline></video>

[![ZenHub] (https://raw.githubusercontent.com/ZenHubIO/support/master/zenhub-badge.png)] (https://zenhub.io)

## Intro

*A demoscene/retro Jukebox/spotify-like  music player*

* **Intructions are dead simple:**
* **Type anything to incrementally search in entire database**
* **Hit enter to play directly**
* **TAB key = help screen**
* **(You can also read the scroll text for more info)**

## Binaries

Binaries for macOS 26 / Tahoe (for now) are available under *Releases*

https://github.com/sasq64/chipmachine/releases

### Running the Apple Silicon Build (Gatekeeper Authorization)

TESTED ON: TAHOE, SONOMA

Because this standalone Apple Silicon build is distributed with an ad-hoc code signature, macOS Gatekeeper will block it upon download. This is standard behavior for open-source binaries compiled outside the Mac App Store.

To authorize and run the application on your Mac, follow these steps:

1. Double-click `ChipMachineAS.app`. macOS will display a prompt stating the app cannot be opened because the developer cannot be verified. Click **Done** or **Cancel**.
2. Open your Mac's **System Settings**.
3. Navigate to **Privacy & Security** in the left sidebar and scroll down to the **Security** section.
4. Look for the notification stating: `“chipmachine” was blocked from use because it is not from an identified developer.`
5. Click the **Open Anyway** button.
6. Authenticate using your Mac's administrator password or Touch ID.
7. Return to the app and double-click it to launch. One final confirmation prompt will appear—click **Open**.

*Note: You only need to perform this authorization once. Subsequent launches will boot instantly.*

## Prerequisites for development (not tested yet on another clean machine)

* TESTED ON MAC OS 26 (TAHOE) ONLY AND NOT TESTED YET ON A CLEAN MACHINE
* Make sure you have Homebrew installed (Apple Silicon homebrew is in /opt/homebrew/ , make sure you are not using Intel legacy /usr/local tools)
* Download, build and install _libmpg123_ (http://sourceforge.net/projects/mpg123/files/)
* brew install git cmake ninja freetype glew glfw3 lua fftw

## Building for Apple Silicon (ALPHA/WIP, see TOODOO.txt)

	# git clone https://github.com/mihailod/chipmachine.git
	# git clone https://github.com/mihailod/apone.git
	# git clone https://github.com/mihailod/musicplayer.git
	# git clone https://github.com/mihailod/vice310.git
	# mkdir build ; cd build
	# cmake ../chipmachine -GNinja -DCMAKE_BUILD_TYPE=Release
	# ninja

## Using the application

* Type words separated by spaces for incremental search
* *ENTER* to play, *SHIFT-ENTER* to enque
* *F1* = Player screen, *F2* = Search screen
* *F5* = Play/Pause
* *F6* = Next Song (or *ENTER* from Player Screen)
* *ESC* = Clear search field
* *SHIFT-ESC* = Quit
* *F7* = Toggle Favorite

## Data Sources

### Music Collections

* Modland - http://ftp.modland.com/
* High Voltage SID collection - http://www.hvsc.c64.org/
* Amiga remix - http://amigaremix.com/
* RKO - http://remix.kwed.org/
* Atari ST - http://sndh.atari.org/
* SNES Music - http://snesmusic.org/
* Atari SAP - http://asma.atari.org/
* Sounds of Scenesat - http://sos.scenesat.com/
* AmigaVibes - http://www.amigavibes.org/
* Demovibes - http://www.demovibes.org/

### Demo databases

* Pouet - http://pouet.net/
* Bitworld - http://janeway.exotica.org.uk/
* CSDb - http://csdb.dk/

### Podcasts

* Bitjam - http://www.bitfellas.org/podcast
* Syntax Error - http://www.syntaxerror.nu/
* C64 Take Away - http://c64takeaway.com/
* Gamewave - http://gamewave.yays.co/
* This Week in Chiptune - http://thisweekinchiptune.com/
* Bitar Till Kaffet - http://www.bitartillkaffet.se/

### Shoutcast Radio

Scenesat - http://www.scenesat.com/
SLAY Radio - http://www.slayradio.org/
Nectarine - https://www.scenemusic.net/
VGM Radio - http://vgmradio.com/
NoLife-Radio - http://nolife-radio.com/
Rainwave - http://chiptune.rainwave.cc/
ChipBit - http://www.chipbit.net/
The Sid Station - http://c64radio.com/
Radio Parallax - http://www.radio-paralax.de/
CGM UKScene Radio - http://www.lmp.d2g.com/
Retro PC Game Music Streaming Radio - http://gyusyabu.ddo.jp/

## Music Plugins (Supported formats)

### OpenMPT

Support for Amiga tracker formats

* ProTracker, ScreamTracker III, FastTracker II, Impulse Tracker, OpenMPT, ScreamTracker II, NoiseTracker, Soundtracker, Mod's Grave, UltraTracker, Composer 669 / UNIS 669, MultiTracker, OctaMed, Farandole Composer, DigiTracker, Extreme's Tracker, Velvet Studio, DSIK Format, DSMI, ASYLUM, Oktalyzer, X-Tracker, PolyTracker, Epic Megagames, MASI, MadTracker 2, DigiBooster Pro, DigiBooster, Imago Orpheus, Galaxy Sound System

### High Technology

Support for Dreamcast and Sega Saturn music

### Higly Experimental

Support for Playstation 1 & 2 music

### NDS

Support for Nintendo DS music

### Game Music Emulator

Support for various 8 bit console music

* ZX Spectrum, Amstrad CPC, Nintendo Game Boy, Sega Genesis, Mega Drive, NEC TurboGrafx-16, PC Engine, MSX Home Computer, other Z80 systems, Nintendo NES, Famicom (with VRC 6, Namco 106, and FME-7 sound), Atari systems using POKEY sound chip, Super Nintendo, Super Famicom, Sega Master System, Mark III, Sega Genesis, Mega Drive, BBC Micro

### SC68

Support for Atari 16 bit music

### USF

Support for Nintendo 64 music

### StSound

Support for Atari ST music (older formats)

### ADplug

Support for retro audio format hardware simulation

* AdLib Tracker 2 by subz3ro, 
Westwood ADL File Format, 
AMUSIC Adlib Tracker by Elyssis, 
Bob's Adlib Music Format, 
BoomTracker 4.0 by CUD, 
Creative Music File Format by Creative Technology, 
EdLib by Vibrants, 
Digital-FM by R.Verhaag, 
Twin TrackPlayer by TwinTeam, 
DOSBox Raw OPL Format, 
DeFy Adlib Tracker by DeFy, 
HSC Adlib Composer by Hannes Seifert, HSC-Tracker by Electronic Rats, 
HSC Packed by Number Six / Aegis Corp., 
Apogee IMF File Format, 
Ken Silverman's Music Format, 
LucasArts AdLib Audio File Format by LucasArts, 
LOUDNESS Sound System, 
igin AdLib Music Format, 
Mlat Adlib Tracker, 
MIDI Audio File Format, 
MKJamz by M \ K Productions (preliminary), 
AdLib MSCplay, 
MPU-401 Trakker by SuBZeR0, 
Reality ADlib Tracker by Reality, 
RdosPlay RAW file format by RDOS, 
Softstar RIX OPL Music Format, 
AdLib Visual Composer by AdLib Inc., 
Screamtracker 3 by Future Crew, 
Surprise! Adlib Tracker 2 by Surprise! Productions, 
Surprise! Adlib Tracker by Surprise! Productions, 
Sierra's AdLib Audio File Format, 
SNGPlay by BUGSY of OBSESSION, 
Faust Music Creator by FAUST, 
Adlib Tracker 1.0 by TJ, 
eXotic ADlib Format by Riven the Mage, 
XMS-Tracker by MaDoKaN/E.S.G, 
eXtra Simple Music by Davey W Taylor, 

### MP3

Support for MP3 music

### Vice

Support for Commodore C64 music

### Hively

Support for AHX and HVL amiga music

### RSN

Support for RAR packed music (primarily SNES)

### Ayfly

Support for various XZ Spectrum formats

### MDX

Support for the Sharp X68000 Music Macro Language

### S98

Support for retro hardware Music

### AudioOverload

Support for Sega Saturn and Capcom Q music

### GSF

Support for Gameboy Advance music

### UADE

Support for Amiga exotic (Delitracker) formats

* ActionAmics AbyssHighestExperience ADPCM-mono AM-Composer AMOS ArtAndMagic Alcatraz-Packer ArtOfNoise-4V ArtOfNoise-8V AudioSculpture BeathovenSynthesizer BenDaglish BenDaglish-SID BladePacker ChipTracker Cinemaware CoreDesign custom CustomMade DariusZendeh DaveLowe DaveLowe-Deli DaveLoweNew DavidHanney DavidWhittaker DeltaMusic2.0 DeltaMusic1.3 Desire DIGI-Booster DigitalSonixChrome DigitalSoundStudio DynamicSynthesizer EMS EMS-6 FashionTracker FutureComposer1.3 FutureComposer1.4 Fred FredGray FutureComposer-BSI FuturePlayer ForgottenWorlds-Game GlueMon EarAche HowieDavies JochenHippel-CoSo QuadraComposer ImagesMusicSystem Infogrames InStereo InStereo2.0 JamCracker JankoMrsicFlogel JasonBrooke JasonPage JasonPage-JP JeroenTel JesperOlsen JochenHippel JochenHippel-7V Jochen-Hippel-ST KrisHatlelid Laxity LegglessMusicEditor ManiacsOfNoise MagneticFieldsPacker MajorTom Mark-Cooksey Mark-Cooksey-Old MarkII MartinWalker Maximum-Effect MCMD MED Medley MIDI-Loriciel MikeDavies MMDC Mugician MugicianII MusicAssembler MusicMaker-4V MusicMaker-8V MultiMedia-Sound NovoTradePacker NTSP-system Octa-MED Oktalyzer onEscapee PaulRobotham PaulShields PaulSummers PeterVerswyvelen PierreAdane ProfessionalSoundArtists PTK-Prowiz PumaTracker RichardJoseph RiffRaff RobHubbard RobHubbardOld Lionheart-Game SCUMM SeanConnolly SeanConran SIDMon1.0 SIDMon2.0 Silmarils SonicArranger SonicArranger-pc-all SonixMusicDriver SoundProgrammingLanguage SoundControl SoundFactory Sound-FX SoundImages SoundMaster SoundMon2.0 SoundMon2.2 SoundPlayer Special-FX Special-FX-ST SpeedyA1System SpeedySystem SteveBarrett SteveTurner SUN-Tronic Synth SynthDream SynthPack SynTracker TFMX TFMX-1.5-TFHD TFMX-7V TFMX-7V-TFHD TFMX-Pro TFMX-Pro-TFHD TFMX-ST ThomasHermann TimFollin TheMusicalEnlightenment TomyTracker Tronic UFO UltimateSoundtracker VoodooSupremeSynthesizer WallyBeben YM-2149 MusiclineEditor Soundtracker-IV Sierra-AGI DirkBialluch Quartet Quartet-PSG Quartet-ST 

### TedPlay

Support for Plus/4 music

### FFMPeg

Support for streaming audio

* AAC
* Ogg/Vorbis

## V2

Support for Farbrauisch V2 Synthesizer System modules

