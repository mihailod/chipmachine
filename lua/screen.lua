------
-- Lua settings
------
Settings = {}

TV = false
if SCREEN_WIDTH == 720 and SCREEN_HEIGHT == 576 then
	TV = true
end

GSCALE = SCREEN_HEIGHT / 576.0

-- Make margins fully proportional to window scale
scale = 3.0 * GSCALE
X0 = 15 * GSCALE
-- Search field baseline (small font, near top)
Y0 = 35 * GSCALE
-- Main title baseline (large font, pushed down by its ascender)
TITLE_Y0 = 35 + (25 * scale)

X1 = SCREEN_WIDTH - X0
Y1 = SCREEN_HEIGHT - 10

background = 0x808080

if true then
 TEXT_COLOR = 0xffe0e080
 DIGITS_COLOR = 0xff70b050
 SEARCH_COLOR = 0xffaaaaff
 FORMAT_COLOR = 0xffffffaa
 RESULT_COLOR = 0xff20c020
 SPECTRUM_COLOR0 = 0xffffffff
 SPECTRUM_COLOR1 = 0xff444444
else
 TEXT_COLOR = 0xff000000
 DIGITS_COLOR = 0xff202080
 RESULT_COLOR = 0xff202040
 SEARCH_COLOR = 0xffffaaaa
 FORMAT_COLOR = 0xffffffaa
 SPECTRUM_COLOR0 = 0xff000000
 SPECTRUM_COLOR1 = 0xff404040
 Settings.background = 0x888888
 Settings.stars = 0
end

Settings.top_left = { X0, Y0 }
Settings.down_right = { X1, Y1 }

Settings.main_title = { X0, TITLE_Y0, scale, TEXT_COLOR }
Settings.main_composer = { X0, TITLE_Y0+25*scale, scale*0.6, TEXT_COLOR }
Settings.main_format = { X0, TITLE_Y0+42*scale, scale*0.25, TEXT_COLOR }


SY = Settings.main_format[2] + 32 * GSCALE
Settings.song_field = { X0, SY, GSCALE, DIGITS_COLOR }
Settings.time_field = { X0 + 130 * GSCALE, SY, GSCALE, DIGITS_COLOR }
Settings.length_field = { X0 + 220 * GSCALE, SY, GSCALE, DIGITS_COLOR }

Settings.xinfo_field = { X0 - 4, SY + 35 * GSCALE, GSCALE * 0.75, 0xffffffff }

Settings.favicon = { X0 + 330 * GSCALE, SY - GSCALE*25, 8*8 * GSCALE, 8*6 * GSCALE }

EQ_SLOTS = 24
SPECW = SCREEN_WIDTH / EQ_SLOTS
SPECH = SPECW * 3.5

if TV then
  Settings.scroll = { Y1 - 100, GSCALE * 2.0, 4, "data/Bello.otf" }
  Settings.spectrum = { X0-40, Y1+40, 28, 80.0, SPECTRUM_COLOR0, SPECTRUM_COLOR1 }
else
  Settings.scroll = { Y1 - GSCALE * 150, GSCALE * 2.0, 4, "data/Bello.otf" }
  -- Anchor spectrum firmly to the bottom of the window
  Settings.spectrum = { X0, SCREEN_HEIGHT - 10, SPECW, SPECH, SPECTRUM_COLOR0, SPECTRUM_COLOR1 }
end

x = SCREEN_WIDTH - 300 * GSCALE
y = Settings.scroll[1] - 80 * GSCALE


NSCALE = SCREEN_PPI / 240.0;
if(NSCALE < 0.5) then NSCALE = 0.5 end

m = 15
Settings.next_field = { X1 - m, y-(28 * NSCALE), NSCALE, 0xff444477 }

scale2 = 1.2 * GSCALE
Settings.next_title = { X1 - m, y, scale2, TEXT_COLOR }
Settings.next_composer = { X1 - m, y+26*scale2, scale2*0.6, TEXT_COLOR }
Settings.next_format = { X1 - m, y+44*scale2, scale2*0.3, TEXT_COLOR }

y = Settings.scroll[1] - 70 * GSCALE

scale3 = 80.0
Settings.exit_title = { -3200, Y0, scale3, 0 }
Settings.exit_composer = { -3200, Y0+25*scale3, scale3*0.6, 0 }
Settings.exit_format = { -3200, Y0+45*scale3, scale3*0.3, 0 }

x = SCREEN_WIDTH+10
y = 340
scale4 = 1.0
Settings.enter_title = { x, y, scale4, TEXT_COLOR }
Settings.enter_composer = { x, y+25*scale4, scale4*0.6, TEXT_COLOR }
Settings.enter_format = { x, y+45*scale4, scale4*0.3, TEXT_COLOR }

-- 220
LSCALE = GSCALE * 0.85;
if(LSCALE < 0.75) then LSCALE = 0.75 end
LINE_HEIGHT = 1.5
TEXT_HEIGHT = 24 * LSCALE

Settings.search_field = { X0, Y0, LSCALE, SEARCH_COLOR }
Settings.top_status = { X0, Y0, LSCALE, FORMAT_COLOR }

Settings.result_field = { X0, Y0+TEXT_HEIGHT, LSCALE, RESULT_COLOR }
Settings.result_lines = (Y1-Y0)/(TEXT_HEIGHT*LINE_HEIGHT)

Settings.toast_field = { 0, SCREEN_HEIGHT/2 - GSCALE * 20, GSCALE * 2.0, 0x00000000 }

Settings.font = "data/Neutra.otf"
Settings.list_font = "data/Neutra.otf"
