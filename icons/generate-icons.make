
GENERATED_ICONS = \
	win32/installer_wizard_image.bmp \
	win32/appicon.ico \
	win32/xp/poedit-translation-generic.ico \
	win32/vista/poedit-translation-generic.ico \
	osx/appicon.icns \
	osx/poedit-translation-generic.icns

APPICONS_WIN32 = \
	appicon/16x16/apps/poedit.png \
	appicon/32x32/apps/poedit.png \
	appicon/48x48/apps/poedit.png \

#   FIXME: disabled 256x256 for now, because VC++ 2005's resource compiler
#          cannot handle ICO files with embedded PNG images yet
#	appicon/256x256/apps/poedit.png

APPICONS_OSX = \
	appicon/16x16/apps/poedit.png \
	appicon/32x32/apps/poedit.png \
	appicon/48x48/apps/poedit.png \
	appicon/128x128/apps/poedit.png \
	appicon/256x256/apps/poedit.png \
	appicon/512x512/apps/poedit.png


all: $(GENERATED_ICONS)

clean:
	rm -f $(GENERATED_ICONS)

win32/installer_wizard_image.bmp: appicon/48x48/apps/poedit.png
	convert $< -background white -extent 55x55-4-4 -type palette BMP3:$@

win32/appicon.ico: $(APPICONS_WIN32)
	# FIXME: should contain 8bit and 4bit versions too
	convert $(APPICONS_WIN32) -compress None $@

win32/xp/poedit-translation-generic.ico: mime-win32/xp/generic/*.png
	# FIXME: should contain 8bit and 4bit versions too
	convert $^ -compress None $@

win32/vista/poedit-translation-generic.ico: mime-win32/vista/generic/*.png
	# FIXME: should contain 8bit and 4bit versions too
	convert $^ -compress None $@


osx/appicon.icns: $(APPICONS_OSX)
	png2icns $@ $^

osx/poedit-translation-generic.icns: mime-osx/generic/*.png
	png2icns $@ $^
