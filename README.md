ChipMachineAS

<div align="right">
  <img src="https://img.shields.io/github/downloads/mihailod/chipmachine/total?label=Total%20Downloads" alt="Total Downloads">
</div>


**Port of ChipMachine (see the fork info) for Apple Silicon.**

![Annoying Popup](data/misc/argh.jpg)

> ChipMachine is my favorite Mac retro chiptune player and I have been using it forever. However, when opening it on my Mac after a recent macOS update, I saw this annoying popup. It caused anger in me, and I channeled that anger into this project.

---

**ONLY BUILDING ON APPLE SILICON (ARM / M CHIPS) HAS BEEN TESTED -- INSTALL AT YOUR OWN RISK.**

**BUILD SCRIPTS (FOR NOW) EXPLICITLY TARGET ONLY THE VERY LATEST macOS (26 / TAHOE) AND HAVE NOT BEEN TESTED ON OLDER VERSIONS.**

**I AM USING AI (gemini/agy, claude, codex) TO HELP ME WITH THIS INTEL -> ARM PORT. SO IT IS ALSO AN EXPERIMENT IN HOW AI PERFORMS AT THIS TASK.**

![Screenshot](data/misc/screen.png)

## Intro

*A demoscene/retro Jukebox/spotify-like  music player*

* **Intructions are dead simple:**
* **Start typing to incrementally search aggregated database**
* **UP/DOWN keys = select a song from search results**
* **ENTER key = play**
* **TAB key = help screen**
* **(Read the scrolling text for more info)**

## Binaries

Binaries for macOS (only tested on Tahoe for now) are available under *Releases*

**NOTE I understand that Tahoe is a tough requirement for some however I have no means to test on older macOS. If you care about older macOS that much feel free to fix yourself (if something is not working) and I will approve pull requests (assuming you can properly test it)**

https://github.com/mihailod/chipmachine/releases

### Running on Mac (Gatekeeper Authorization)

TESTED ON TAHOE ONLY

Because this standalone app is distributed with an ad-hoc code signature, macOS Gatekeeper will block it. This is standard behavior for open-source binaries distributed outside the official Mac App Store ecosystem.

To authorize and run the application on your Mac, follow these steps:

1. Download the latest release and unzip it in the Applications folder
2. Double-click `ChipMachineAS.app`.
3. macOS will display a prompt stating the app cannot be opened because the developer cannot be verified.
4. Click **Done** or **Cancel**.
5. Open your Mac's **System Settings**.
6. Navigate to **Privacy & Security** in the left sidebar
7. Scroll down to the **Security** section.
8. Look for the notification stating: `“ChipMachineAS” was blocked from use because it is not from an identified developer.`
9. Click the **Open Anyway** button.
10. Authenticate using your Mac's admin password or Touch ID.
11. Double-click `ChipMachineAS.app` again.
12. The final confirmation prompt will appear.
13. Click **Open**.

*Note: You only need to perform this authorization once. Subsequent launches will boot instantly.*

## Prerequisites for development (Tested on macOS 26 / Tahoe only)

* Make sure you have Homebrew installed (Apple Silicon homebrew in /opt/homebrew/ , make sure you are not using Intel legacy /usr/local tools)
* brew install git cmake ninja freetype glew glfw3 lua fftw mpg123 python ffmpeg
* (if some packages are reported missing later install then via brew and let me know -- I missed them in the line above)

## Building for Apple Silicon (ALPHA/WIP, see TOODOO.txt)

```bash
mkdir chipmachine-as && cd chipmachine-as
git clone https://github.com/mihailod/chipmachine.git
git clone https://github.com/mihailod/apone.git
git clone https://github.com/mihailod/musicplayer.git
git clone https://github.com/mihailod/vice310.git
git clone https://github.com/mihailod/98fmplayer.git
git clone https://github.com/mihailod/libpxtone.git
mkdir build && cd build
cmake ../chipmachine -GNinja -DCMAKE_BUILD_TYPE=Release
ninja
```

* Running the app from the build folder: ./chipmachine
* Packaging the app: chipmachine/package_ap.sh

## Using the application

* Type words separated by spaces for incremental search
* *ENTER* to play, *SHIFT-ENTER* to enque
* *F1* = Player screen, *F2* = Search screen
* *F5* = Play/Pause
* *F6* = Next Song (or *ENTER* from Player Screen)
* *ESC* = Clear search field
* *SHIFT-ESC* = Quit
* *F7* = Toggle Favorite
* Type _shoutcast_ to see the radio-stations

## Data Sources

### Music Collections

* Modland - https://ftp.modland.com/
* High Voltage SID collection - https://www.hvsc.c64.org/
* Gamebase64 - http://www.gb64.com/
* AMP (Amiga Music Preservation) - http://amp.dascene.net/
* Amiga remix - http://amigaremix.com/
* RKO - http://remix.kwed.org/
* Atari ST (SNDH) - http://sndh.atari.org/
* SNES Music - http://snesmusic.org/
* Atari SAP (ASMA) - http://asma.atari.org/
* HVTC (High Voltage TED Collection) - http://plus4world.powweb.com/
* NSFE (local NES music collection)
* Sounds of Scenesat - https://scenesat.com/
* AmigaVibes - http://www.amigavibes.org/
* Demovibes - https://www.demovibes.org/

### Demo databases

* Pouet - https://www.pouet.net/
* Bitworld - http://janeway.exotica.org.uk/
* CSDb - https://csdb.dk/

### Podcasts

* Syntax Error - http://www.syntaxerror.nu/
* C64 Take Away - http://c64takeaway.com/

### Shoutcast Radio

Scenesat - https://scenesat.com/
SLAY Radio - https://www.slayradio.org/
Nectarine - https://scenestream.net/
VGM Radio - http://vgmradio.com/
NoLife-Radio - https://www.nolife-radio.com/
Rainwave - https://rainwave.cc/
The Sid Station - https://c64radio.com/
Radio PARALAX - https://www.radio-paralax.de/
CVGM Radio - https://radio.cvgm.net/

## Music Plugins (Supported formats)

### OpenMPT

Support for Amiga tracker formats

* ProTracker, ScreamTracker III, FastTracker II, Impulse Tracker, OpenMPT, ScreamTracker II, NoiseTracker, Soundtracker, Mod's Grave, UltraTracker, Composer 669 / UNIS 669, MultiTracker, OctaMed, Farandole Composer, DigiTracker, Extreme's Tracker, Velvet Studio, DSIK Format, DSMI, ASYLUM, Oktalyzer, X-Tracker, PolyTracker, Epic Megagames, MASI, MadTracker 2, DigiBooster Pro, DigiBooster, Imago Orpheus, Galaxy Sound System

Extensions: `.mod` `.xm` `.it` `.s3m` `.mptm` `.stm` `.nst` `.m15` `.stk` `.wow` `.ult` `.669` `.mtm` `.med` `.far` `.mdl` `.ams` `.dsm` `.amf` `.okt` `.dmf` `.mt2` `.dbm` `.digi` `.imf` `.j2b` `.gdm` `.umx` `.mo3` (and other formats reported by libopenmpt)

### High Technology

Support for Dreamcast and Sega Saturn music

Extensions: `.ssf` `.dsf` `.minissf` `.minidsf`

### Highly Experimental

Support for Playstation 1 & 2 music

Extensions: `.psf` `.psf2` `.minipsf` `.minipsf2`

### NDS

Support for Nintendo DS music

Extensions: `.2sf` `.mini2sf`

### Game Music Emulator

Support for various 8 bit console music

* ZX Spectrum, Amstrad CPC, Nintendo Game Boy, Sega Genesis, Mega Drive, NEC TurboGrafx-16, PC Engine, MSX Home Computer, other Z80 systems, Nintendo NES, Famicom (with VRC 6, Namco 106, and FME-7 sound), Atari systems using POKEY sound chip, Super Nintendo, Super Famicom, Sega Master System, Mark III, Sega Genesis, Mega Drive, BBC Micro

Extensions: `.spc` `.nsf` `.nsfe` `.gbs` `.ay` `.gym` `.sap` `.vgm` `.vgz` `.hes` `.kss` `.sgc` `.emul`

### SC68

Support for Atari 16 bit music

Extensions: `.sc68` `.sndh` `.snd`

### USF

Support for Nintendo 64 music

Extensions: `.usf` `.miniusf`

### StSound

Support for Atari ST music (older formats)

Extensions: `.ym` `.mix`

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

Extensions: `.a2m` `.adl` `.amd` `.bam` `.cff` `.cmf` `.d00` `.dfm` `.dmo` `.dro` `.dtm` `.hcs` `.hsp` `.imf` `.ksm` `.laa` `.lds` `.m` `.mad` `.mid` `.mkj` `.msc` `.mtk` `.rad` `.raw` `.rix` `.rol` `.as3m` `.sa2` `.sat` `.sci` `.agd` `.sdb` `.xad` `.xms` `.xsm` `.hsc` `.edl` `.mtr` `.adlib` `.sqx`

### MP3

Support for MP3 music

Extensions: `.mp3`

### Vice

Support for Commodore C64 music (.sid), including Compute's Stereo Sidplayer tunes (.mus / .str)

### Hively

Support for AHX and HVL amiga music

Extensions: `.ahx` `.hvl`

### RSN

Support for RAR packed music (primarily SNES)

Extensions: `.rsn` `.rps` `.rdc` `.rds` `.rgs` `.r64`

### Ayfly

Support for various ZX Spectrum formats

Extensions: `.ay` `.psg` `.asc` `.stc` `.psc` `.sqt` `.stp` `.stp2` `.pt1` `.pt2` `.pt3` `.ftc` `.vtx` `.vt2` `.zxs` `.st13`

### MDX

Support for the Sharp X68000 Music Macro Language

Extensions: `.mdx` (with optional `.pdx` sample banks)

### FMP (98fmplayer)

Support for NEC PC-98 FMP driver music (.opi / .ovi / .ozi)

### PxTone

Support for PxTone Collage music (.ptcop / .pttune) by Studio Pixel

### S98

Support for retro hardware Music

Extensions: `.s98`

### AudioOverload

Support for Sega Saturn and Capcom Q music

Extensions: `.ssf` `.minissf` `.qsf` `.miniqsf` `.spu`

### GSF

Support for Gameboy Advance music

Extensions: `.gsf` `.minigsf`

### UADE

Support for Amiga exotic (Delitracker) formats

* ActionAmics AbyssHighestExperience ADPCM-mono AM-Composer AMOS ArtAndMagic Alcatraz-Packer ArtOfNoise-4V ArtOfNoise-8V AudioSculpture BeathovenSynthesizer BenDaglish BenDaglish-SID BladePacker ChipTracker Cinemaware CoreDesign custom CustomMade DariusZendeh DaveLowe DaveLowe-Deli DaveLoweNew DavidHanney DavidWhittaker DeltaMusic2.0 DeltaMusic1.3 Desire DIGI-Booster DigitalSonixChrome DigitalSoundStudio DynamicSynthesizer EMS EMS-6 FashionTracker FutureComposer1.3 FutureComposer1.4 Fred FredGray FutureComposer-BSI FuturePlayer ForgottenWorlds-Game GlueMon EarAche HowieDavies JochenHippel-CoSo QuadraComposer ImagesMusicSystem Infogrames InStereo InStereo2.0 JamCracker JankoMrsicFlogel JasonBrooke JasonPage JasonPage-JP JeroenTel JesperOlsen JochenHippel JochenHippel-7V Jochen-Hippel-ST KrisHatlelid Laxity LegglessMusicEditor ManiacsOfNoise MagneticFieldsPacker MajorTom Mark-Cooksey Mark-Cooksey-Old MarkII MartinWalker Maximum-Effect MCMD MED Medley MIDI-Loriciel MikeDavies MMDC Mugician MugicianII MusicAssembler MusicMaker-4V MusicMaker-8V MultiMedia-Sound NovoTradePacker NTSP-system Octa-MED Oktalyzer onEscapee PaulRobotham PaulShields PaulSummers PeterVerswyvelen PierreAdane ProfessionalSoundArtists PTK-Prowiz PumaTracker RichardJoseph RiffRaff RobHubbard RobHubbardOld Lionheart-Game SCUMM SeanConnolly SeanConran SIDMon1.0 SIDMon2.0 Silmarils SonicArranger SonicArranger-pc-all SonixMusicDriver SoundProgrammingLanguage SoundControl SoundFactory Sound-FX SoundImages SoundMaster SoundMon2.0 SoundMon2.2 SoundPlayer Special-FX Special-FX-ST SpeedyA1System SpeedySystem SteveBarrett SteveTurner SUN-Tronic Synth SynthDream SynthPack SynTracker TFMX TFMX-1.5-TFHD TFMX-7V TFMX-7V-TFHD TFMX-Pro TFMX-Pro-TFHD TFMX-ST ThomasHermann TimFollin TheMusicalEnlightenment TomyTracker Tronic UFO UltimateSoundtracker VoodooSupremeSynthesizer WallyBeben YM-2149 MusiclineEditor Soundtracker-IV Sierra-AGI DirkBialluch Quartet Quartet-PSG Quartet-ST 

Extensions: several hundred Amiga player/packer extensions are matched as a filename prefix or suffix, e.g. `.mod` `.fc` `.fc13` `.fc14` `.hip` `.hip7` `.tfmx` `.mdat` `.ahx` `.thx` `.okt` `.med` `.mmd0` `.mmd1` `.mmd2` `.dw` `.cust` `.custom` `.sid1` `.sid2` `.ym` `.sng` `.digi` `.dss` `.jam` `.pru2` (see `UADEPlugin.cpp` for the full list)

### TedPlay

Support for Plus/4 music

Extensions: `.prg`

### FFMpeg

Support for streaming audio

* AAC
* Ogg/Vorbis

Extensions: `.m4a` `.aac` `.mp3` `.mp4`

### V2

Support for Farbrausch V2 Synthesizer System modules

Extensions: `.v2` `.v2m`
